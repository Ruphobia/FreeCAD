# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2025 FreeCAD Community                                   *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

"""
Command to create a new Part from a selected face in an Assembly.

This enables an Inventor-like workflow where you can select a face on an
existing part and immediately start designing a new part attached to that face.
"""

import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtWidgets

import UtilsAssembly
import JointObject

translate = App.Qt.translate

__title__ = "Assembly Command Create Part From Face"
__author__ = "FreeCAD Community"
__url__ = "https://www.freecad.org"


def getSelectedFace():
    """
    Get the currently selected face from the active selection.

    Returns:
        tuple: (obj, face_name, full_sub_path) or (None, None, None) if no valid face selected
               - obj: The document object containing the face
               - face_name: The face element name (e.g., "Face1")
               - full_sub_path: The full sub-element path for assembly reference
    """
    selection = Gui.Selection.getSelectionEx("*", 0)
    if not selection:
        return None, None, None

    for sel in selection:
        if not sel.SubElementNames:
            continue

        for sub_name in sel.SubElementNames:
            # Extract the face name from the sub-element path
            # sub_name might be "Link.Body.Face1" or just "Face1"
            parts = sub_name.split(".")
            element_name = parts[-1] if parts else sub_name

            # Check if it's a face
            if element_name.startswith("Face"):
                return sel.Object, element_name, sub_name

    return None, None, None


def isFacePlanar(obj, sub_name):
    """
    Check if the selected face is planar.

    Args:
        obj: The document object
        sub_name: The sub-element path to the face

    Returns:
        bool: True if the face is planar, False otherwise
    """
    try:
        # Get the actual shape through the sub-object path
        sub_obj = obj.getSubObject(sub_name, retType=1)
        if sub_obj is None:
            return False

        # Check if it's a face and if it's planar
        if hasattr(sub_obj, "Surface"):
            surface_type = sub_obj.Surface.TypeId
            # Planar surfaces are "Part::GeomPlane"
            return surface_type == "Part::GeomPlane"

        return False
    except Exception:
        return False


class CommandCreatePartFromFace:
    """Command to create a new Part from a selected face."""

    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "Geofeaturegroup",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreatePartFromFace", "New Part from Face"),
            "Accel": "N",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_CreatePartFromFace",
                "Create a new part with a sketch attached to the selected face. "
                "Select a planar face on an existing part first.",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        """Command is active when in assembly edit mode with a face selected."""
        if not UtilsAssembly.isAssemblyCommandActive():
            return False

        obj, face_name, sub_name = getSelectedFace()
        return obj is not None and face_name is not None

    def Activated(self):
        """Execute the command."""
        assembly = UtilsAssembly.activeAssembly()
        if assembly is None:
            return

        # Get the selected face
        obj, face_name, sub_name = getSelectedFace()
        if obj is None:
            QtWidgets.QMessageBox.warning(
                None,
                translate("Assembly", "No Face Selected"),
                translate("Assembly", "Please select a face on an existing part first."),
            )
            return

        # Check if face is planar
        if not isFacePlanar(obj, sub_name):
            QtWidgets.QMessageBox.warning(
                None,
                translate("Assembly", "Non-Planar Face"),
                translate(
                    "Assembly",
                    "The selected face is not planar. Please select a flat face.",
                ),
            )
            return

        # Start transaction for undo
        App.setActiveTransaction("Create Part from Face")

        try:
            self.createPartFromFace(assembly, obj, face_name, sub_name)
            App.closeActiveTransaction()
        except Exception as e:
            App.closeActiveTransaction(True)  # Abort on error
            QtWidgets.QMessageBox.critical(
                None,
                translate("Assembly", "Error"),
                translate("Assembly", f"Failed to create part: {str(e)}"),
            )

    def createPartFromFace(self, assembly, source_obj, face_name, sub_name):
        """
        Create a new Part with Body and Sketch attached to the selected face.

        Args:
            assembly: The active assembly object
            source_obj: The object containing the selected face
            face_name: The face element name (e.g., "Face1")
            sub_name: The full sub-element path
        """
        doc = assembly.Document

        # Generate a unique part name
        base_name = "Part"
        part_name = base_name
        counter = 1
        while doc.getObject(part_name):
            part_name = f"{base_name}{counter:03d}"
            counter += 1

        # Create the Part container
        part = doc.addObject("App::Part", part_name)

        # Create Body inside the Part
        body = part.newObject("PartDesign::Body", "Body")

        # Create Sketch inside the Body
        sketch = body.newObject("Sketcher::SketchObject", "Sketch")

        # Get the object that actually owns the face for attachment
        # We need to resolve through any links to get the actual geometry owner
        linked_obj, matrix = source_obj.getSubObject(sub_name, retType=2, matrix=App.Matrix())

        # Set up sketch attachment to the selected face
        # The attachment support needs to reference the face through the assembly link structure
        sketch.AttachmentSupport = [(source_obj, (sub_name,))]
        sketch.MapMode = "FlatFace"

        # Recompute to position the sketch
        doc.recompute()

        # Create a Link to the new part in the assembly
        link = assembly.newObject("App::Link", "Link_" + part_name)
        link.LinkedObject = part
        link.Label = part.Label

        # Create a Fixed joint to lock the new part to the face
        self.createFixedJoint(assembly, source_obj, sub_name, link)

        # Recompute assembly
        doc.recompute()

        # Solve the assembly to position the new part
        JointObject.solveIfAllowed(assembly)

        # Set the body as active
        Gui.ActiveDocument.ActiveView.setActiveObject("pdbody", body)

        # Open the sketch for editing
        Gui.ActiveDocument.setEdit(sketch.Name)

        App.Console.PrintMessage(
            f"Created new part '{part.Label}' with sketch attached to {face_name}\n"
        )

    def createFixedJoint(self, assembly, source_obj, sub_name, new_part_link):
        """
        Create a Fixed joint between the source face and the new part origin.

        Args:
            assembly: The assembly object
            source_obj: Object containing the source face
            sub_name: Sub-element path to the face
            new_part_link: Link to the new part
        """
        # Get or create the Joint group
        joint_group = UtilsAssembly.getJointGroup(assembly)

        # Create the Fixed joint object
        joint = joint_group.newObject("App::FeaturePython", "Fixed")
        JointObject.Joint(joint, 0)  # 0 = Fixed joint type
        JointObject.ViewProviderJoint(joint.ViewObject)

        # Set the first reference (the source face)
        # Reference format: [obj, [element_path, vertex_path]]
        joint.Reference1 = [source_obj, [sub_name, sub_name]]

        # Compute placement for Reference1
        joint.Placement1 = UtilsAssembly.findPlacement(joint, joint.Reference1, 0)

        # Set the second reference (the new part's origin)
        # Empty string means use the part's origin
        joint.Reference2 = [new_part_link, ["", ""]]
        joint.Placement2 = App.Placement()

        # Make the joint invisible (it's just for positioning)
        joint.Visibility = False

        joint.Label = f"Fixed_{new_part_link.Label}"


if App.GuiUp:
    Gui.addCommand("Assembly_CreatePartFromFace", CommandCreatePartFromFace())

# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui


class CommandInsertNewPart:
    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "Geofeaturegroup",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_InsertNewPart", "New Part"),
            "Accel": "P",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_InsertNewPart",
                "Create a new part with a sketch ready for editing.",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return App.ActiveDocument is not None

    def Activated(self):
        try:
            doc = App.ActiveDocument
            App.Console.PrintMessage("NewPart: Starting\n")

            # Generate unique part name
            counter = 1
            part_name = f"Part{counter:03d}"
            while doc.getObject(part_name):
                counter += 1
                part_name = f"Part{counter:03d}"

            # Create Part
            part = doc.addObject("App::Part", part_name)
            App.Console.PrintMessage(f"NewPart: Created {part_name}\n")

            # Create Body inside Part
            body = doc.addObject("PartDesign::Body", "Body")
            part.addObject(body)
            App.Console.PrintMessage(f"NewPart: Created Body\n")

            # Create Sketch attached to XY plane
            sketch = body.newObject("Sketcher::SketchObject", "Sketch")
            App.Console.PrintMessage(f"NewPart: Created Sketch: {sketch.Name}\n")

            # Attach sketch to the XY plane (index 3 is XY_Plane in OriginFeatures)
            xy_plane = body.Origin.OriginFeatures[3]
            App.Console.PrintMessage(f"NewPart: XY plane: {xy_plane.Name}\n")
            sketch.AttachmentSupport = (xy_plane, [""])
            sketch.MapMode = "FlatFace"

            doc.recompute()
            App.Console.PrintMessage("NewPart: Recomputed\n")

            # Set body as active
            Gui.ActiveDocument.ActiveView.setActiveObject("pdbody", body)
            App.Console.PrintMessage("NewPart: Set active body\n")

            # Open sketch for editing
            result = Gui.ActiveDocument.setEdit(sketch)
            App.Console.PrintMessage(f"NewPart: setEdit result: {result}\n")

        except Exception as e:
            import traceback
            App.Console.PrintError(f"NewPart ERROR: {e}\n")
            App.Console.PrintError(traceback.format_exc())


if App.GuiUp:
    Gui.addCommand("Assembly_InsertNewPart", CommandInsertNewPart())

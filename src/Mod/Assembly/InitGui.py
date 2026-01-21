# SPDX-License-Identifier: LGPL-2.1-or-later

import Assembly_rc


class AssemblyWorkbench(Workbench):
    "Assembly workbench"

    def __init__(self):
        self.__class__.Icon = (
            FreeCAD.getResourceDir() + "Mod/Assembly/Resources/icons/AssemblyWorkbench.svg"
        )
        self.__class__.MenuText = "Assembly"
        self.__class__.ToolTip = "Assembly workbench"

    def Initialize(self):
        from PySide.QtCore import QT_TRANSLATE_NOOP
        import CommandInsertNewPart

        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench", "Assembly"), ["Assembly_InsertNewPart"])
        self.appendMenu(QT_TRANSLATE_NOOP("Workbench", "&Assembly"), ["Assembly_InsertNewPart"])

    def Activated(self):
        pass

    def Deactivated(self):
        pass


Gui.addWorkbench(AssemblyWorkbench())

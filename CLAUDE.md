# CLAUDE.md - FreeCAD Development Guide

This document provides guidance for AI assistants (like Claude) working on the FreeCAD codebase.

## Project Overview

FreeCAD is an open-source parametric 3D CAD modeler written in C++ with Python bindings. It uses:
- **OpenCASCADE (OCCT)** - Geometry kernel for solid modeling
- **Coin3D/Pivy** - 3D scene visualization
- **Qt6/PySide6** - GUI framework
- **Python 3.11** - Scripting and extension API

**License**: LGPL 2.1 or later

## Repository Structure

```
FreeCAD/
├── src/
│   ├── Base/          # Core utilities: math, I/O, Python integration
│   ├── App/           # Application kernel (no GUI): Document, DocumentObject, Property
│   ├── Gui/           # Graphical interface: MainWindow, 3D views, dialogs
│   ├── Main/          # Entry points: MainGui.cpp, MainCmd.cpp
│   ├── Mod/           # Workbenches/modules (36 total)
│   │   ├── Part/      # Basic geometric operations, BREP
│   │   ├── PartDesign/ # Parametric solid modeling
│   │   ├── Sketcher/  # 2D constraint-based sketching
│   │   ├── TechDraw/  # Technical drawing generation
│   │   ├── Fem/       # Finite Element Analysis
│   │   ├── Assembly/  # Multi-body assembly
│   │   ├── Draft/     # 2D drafting (Python)
│   │   ├── BIM/       # Building Information Modeling (Python)
│   │   ├── CAM/       # CNC/Machining operations
│   │   └── ...        # Many more workbenches
│   ├── 3rdParty/      # Bundled dependencies
│   ├── Ext/           # Extension system
│   ├── Tools/         # Build utilities, code generation
│   └── Doc/           # Doxygen documentation sources
├── cMake/             # CMake configuration modules
├── tests/             # Automated tests
├── tools/             # Build and utility scripts
├── package/           # Packaging scripts (Fedora, Ubuntu, Windows)
├── CMakeLists.txt     # Main CMake build configuration
├── CMakePresets.json  # CMake build presets
└── pixi.toml          # Pixi/Conda environment configuration
```

## Build System

### Recommended: Using Pixi (Conda-based)

Pixi handles all dependencies automatically:

```bash
# Initialize submodules and configure (debug build)
pixi run configure

# Build
pixi run build

# Run FreeCAD
pixi run freecad

# Run tests
pixi run test
```

For release builds, use `configure-release`, `build-release`, `install-release`, `freecad-release`.

### Manual CMake Build

```bash
mkdir build && cd build

# Debug build
cmake --preset conda-linux-debug  # or conda-macos-debug, conda-windows-debug
cmake --build .

# Release build
cmake --preset conda-linux-release
cmake --build .
```

### Key CMake Options

| Option | Description |
|--------|-------------|
| `BUILD_GUI` | Build with GUI (default ON) |
| `BUILD_FEM_NETGEN` | FEM with Netgen mesh generator |
| `BUILD_BIM` | BIM workbench |
| `ENABLE_DEVELOPER_TESTS` | Enable test suite |
| `FREECAD_USE_PCL` | Point Cloud Library support |

## Architecture Concepts

### Document Model
- **App::Application** - Singleton global application instance
- **App::Document** - Container for objects, manages undo/redo and persistence
- **App::DocumentObject** - Base class for all document objects
- **Dependency Graph** - Tracks object dependencies for efficient recomputation

### Property Framework
Properties provide dynamic attributes on any object with automatic serialization:
- Access by name (Python reflection)
- Types: `PropertyInteger`, `PropertyFloat`, `PropertyString`, `PropertyLink`, etc.
- Static (C++) and dynamic (Python) property support

### Workbench Architecture
Each workbench is a directory in `src/Mod/` containing:
- `Init.py` - Module initialization
- `InitGui.py` - GUI initialization (tools, commands)
- `App/` - C++ application code (optional)
- `Gui/` - C++ GUI code (optional)
- `Tests/` - Unit tests

### Key Namespaces
- `Base::` - Foundational utilities (math, I/O, etc.)
- `App::` - Application kernel
- `Gui::` - Graphical interface
- Module-specific namespaces (e.g., `Part::`, `Sketcher::`)

## Coding Standards

### C++ Style (enforced by `.clang-format`)
- **Indentation**: 4 spaces (no tabs)
- **Line length**: 100 characters max
- **Braces**: Opening brace on new line for classes, structs, functions, namespaces
- **Pointers/References**: Left-aligned (`int* ptr`, not `int *ptr`)
- **Access modifiers**: Dedented 4 spaces from class body

```cpp
// Example style
class MyClass
{
public:
    void doSomething(int value);

private:
    int m_value;
};

void MyClass::doSomething(int value)
{
    if (value > 0) {
        m_value = value;
    }
    else {
        m_value = 0;
    }
}
```

### Python Style (enforced by `.pylintrc`)
- **Naming**: snake_case for functions, variables, modules; PascalCase for classes
- **Line length**: 100 characters max
- **Indentation**: 4 spaces
- Note: `FreeCAD` and `FreeCADGui` modules are ignored (C++ bindings)

### License Headers
All files must include SPDX license identifier:
```cpp
// SPDX-License-Identifier: LGPL-2.1-or-later
```

## File Patterns

### C++ Files
- `*.h` - Headers
- `*.cpp` - Implementation
- `*PyImp.cpp` - Python binding implementations
- `*.pyi` - Python type stubs

### Python Files
- `Init.py` - Module initialization (loaded for console mode)
- `InitGui.py` - GUI initialization (loaded only with GUI)
- `Test*.py` - Unit tests

## Testing

```bash
# Run all tests
pixi run test

# Or with CTest directly
cd build/debug
ctest

# Run specific test
ctest -R TestName
```

Tests are organized per-module: `TestPartApp.py`, `TestSketcherGui.py`, etc.

## Common Development Tasks

### Adding a New DocumentObject (C++)

1. Create header in `src/Mod/<Module>/App/`:
```cpp
#include <App/DocumentObject.h>

class MyObject : public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MyModule::MyObject);
public:
    MyObject();
    App::PropertyFloat MyProperty;
    // ...
};
```

2. Register in module's `AppInit.py` or C++ init function

### Adding a GUI Command (Python)

```python
class MyCommand:
    def GetResources(self):
        return {'Pixmap': 'my_icon',
                'MenuText': 'My Command',
                'ToolTip': 'Does something useful'}

    def Activated(self):
        # Command logic here
        pass

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

FreeCADGui.addCommand('MyModule_MyCommand', MyCommand())
```

### Creating a New Workbench (Python)

Use `src/Mod/TemplatePyMod/` as a template. Minimum files:
- `Init.py` - Register importers/exporters
- `InitGui.py` - Register workbench class with toolbars and menus

## Contribution Guidelines

From `CONTRIBUTING.md`:

1. **One PR = One Problem** - Keep changes minimal and focused
2. **Each commit must compile** on all platforms
3. **Preserve Python API compatibility** when possible
4. **No direct commits to main** - All changes via Pull Requests
5. **Clear commit messages** explaining "why" not just "what"
6. **Code style adherence** - Run clang-format before committing

### PR Requirements
- Must compile cleanly on all target platforms
- Should pass project self-tests
- Breaking API changes must be documented with migration guidance

## Useful Commands

```bash
# Format C++ code
clang-format -i src/Mod/MyModule/App/*.cpp src/Mod/MyModule/App/*.h

# Run clang-tidy
clang-tidy src/Mod/MyModule/App/MyFile.cpp

# Generate compile_commands.json (for IDE integration)
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

# Build documentation
cd build && make DevDoc
```

## Key Files Reference

| File | Purpose |
|------|---------|
| `src/App/Application.cpp` | Main application singleton |
| `src/App/Document.cpp` | Document management |
| `src/App/DocumentObject.cpp` | Base object type |
| `src/App/Property*.cpp` | Property framework |
| `src/Gui/MainWindow.cpp` | Main GUI window |
| `src/Gui/Command.cpp` | Command infrastructure |
| `cMake/FreeCAD_Helpers/` | CMake helper functions |

## Debugging Tips

- Use `FreeCAD.Console.PrintMessage()` for Python debugging
- Set `FC_DEBUG` environment variable for verbose output
- Build with Debug preset for symbols: `cmake --preset conda-linux-debug`
- Use `gdb` or `lldb` with the built executable in `build/debug/bin/`

## External Resources

- **Wiki**: https://wiki.freecad.org/
- **Forum**: https://forum.freecad.org/
- **API Documentation**: https://freecad.github.io/SourceDoc/
- **Developer Hub**: https://wiki.freecad.org/Developer_hub

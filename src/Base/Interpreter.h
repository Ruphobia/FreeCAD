// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef BASE_INTERPRETER_H
#define BASE_INTERPRETER_H

#include <FCConfig.h>
#include <string>
#include <list>
#include "Exception.h"

// Strip: Python removed - empty stubs for API compatibility

// No-op macros replacing Python helper macros
#define FC_PY_GetCallable(_pyobj, _name, _var) do {} while(0)
#define FC_PY_GetObject(_pyobj, _name, _var) do {} while(0)

namespace Base
{

/** No-op GIL locker - Python removed */
class BaseExport PyGILStateLocker
{
public:
    PyGILStateLocker() {}
    ~PyGILStateLocker() {}
    PyGILStateLocker(const PyGILStateLocker&) = delete;
    PyGILStateLocker(PyGILStateLocker&&) = delete;
    PyGILStateLocker& operator=(const PyGILStateLocker&) = delete;
    PyGILStateLocker& operator=(PyGILStateLocker&&) = delete;
};

/** No-op GIL release - Python removed */
class BaseExport PyGILStateRelease
{
public:
    PyGILStateRelease() {}
    ~PyGILStateRelease() {}
    PyGILStateRelease(const PyGILStateRelease&) = delete;
    PyGILStateRelease(PyGILStateRelease&&) = delete;
    PyGILStateRelease& operator=(const PyGILStateRelease&) = delete;
    PyGILStateRelease& operator=(PyGILStateRelease&&) = delete;
};

/** Stripped interpreter - no Python */
class BaseExport InterpreterSingleton
{
public:
    InterpreterSingleton();
    ~InterpreterSingleton();

    InterpreterSingleton(const InterpreterSingleton&) = delete;
    InterpreterSingleton(InterpreterSingleton&&) = delete;
    InterpreterSingleton& operator=(const InterpreterSingleton&) = delete;
    InterpreterSingleton& operator=(InterpreterSingleton&&) = delete;

    std::string runString(const char* psCmd);
    std::string runStringWithKey(const char* psCmd, const char* key, const char* key_initial_value = "");
    void runInteractiveString(const char* psCmd);
    void runFile(const char* pxFileName, bool local);
    void runStringArg(const char* psCom, ...);
    bool loadModule(const char* psModName);
    void addPythonPath(const char* Path);
    std::string getPythonPath();
    int cleanup(void (*func)());
    void finalize();
    void systemExit();
    std::string init(int argc, char* argv[]);
    int runCommandLine(const char* prompt);
    void replaceStdOutput();
    static InterpreterSingleton& Instance();
    static void Destruct();
    static std::string strToPython(const char* Str);
    static std::string strToPython(const std::string& Str)
    {
        return strToPython(Str.c_str());
    }

protected:
    static InterpreterSingleton* _pcSingleton;

private:
    std::string _cDebugFileName;
};

inline InterpreterSingleton& Interpreter()
{
    return InterpreterSingleton::Instance();
}

}  // namespace Base

#endif  // BASE_INTERPRETER_H

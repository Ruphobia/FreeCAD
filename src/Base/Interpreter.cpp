// SPDX-License-Identifier: LGPL-2.1-or-later
// Strip: Python removed entirely - empty stub implementations

#include "PreCompiled.h"
#include "Interpreter.h"

using namespace Base;

InterpreterSingleton* InterpreterSingleton::_pcSingleton = nullptr;

InterpreterSingleton::InterpreterSingleton() = default;
InterpreterSingleton::~InterpreterSingleton() = default;

std::string InterpreterSingleton::init(int argc, char* argv[])
{
    return {};
}

std::string InterpreterSingleton::runString(const char* psCmd)
{
    return {};
}

std::string InterpreterSingleton::runStringWithKey(const char* psCmd, const char* key, const char* key_initial_value)
{
    return {};
}

void InterpreterSingleton::runInteractiveString(const char* psCmd)
{
}

void InterpreterSingleton::runFile(const char* pxFileName, bool local)
{
}

void InterpreterSingleton::runStringArg(const char* psCom, ...)
{
}

bool InterpreterSingleton::loadModule(const char* psModName)
{
    return true;
}

void InterpreterSingleton::addPythonPath(const char* Path)
{
}

std::string InterpreterSingleton::getPythonPath()
{
    return {};
}

int InterpreterSingleton::cleanup(void (*func)())
{
    return 0;
}

void InterpreterSingleton::finalize()
{
}

void InterpreterSingleton::systemExit()
{
}

int InterpreterSingleton::runCommandLine(const char* prompt)
{
    return 0;
}

void InterpreterSingleton::replaceStdOutput()
{
}

InterpreterSingleton& InterpreterSingleton::Instance()
{
    if (!_pcSingleton) {
        _pcSingleton = new InterpreterSingleton();
    }
    return *_pcSingleton;
}

void InterpreterSingleton::Destruct()
{
    delete _pcSingleton;
    _pcSingleton = nullptr;
}

std::string InterpreterSingleton::strToPython(const char* Str)
{
    return Str ? std::string(Str) : std::string();
}

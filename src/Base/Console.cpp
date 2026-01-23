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

#include <FCConfig.h>

#if defined(FC_OS_WIN32)
# include <windows.h>
#elif defined(FC_OS_LINUX) || defined(FC_OS_MACOSX)
# include <unistd.h>
#endif
#include <cstring>

#include "Console.h"
#include <QCoreApplication>


using namespace Base;


//=========================================================================

namespace Base
{

class ConsoleEvent: public QEvent
{
public:
    ConsoleSingleton::FreeCAD_ConsoleMsgType msgtype;
    IntendedRecipient recipient;
    ContentType content;
    std::string notifier;
    std::string msg;

    ConsoleEvent(
        const ConsoleSingleton::FreeCAD_ConsoleMsgType type,
        const IntendedRecipient recipient,
        const ContentType content,
        const std::string& notifier,
        const std::string& msg
    )
        : QEvent(QEvent::User)  // NOLINT
        , msgtype(type)
        , recipient(recipient)
        , content(content)
        , notifier(notifier)
        , msg(msg)
    {}
};

class ConsoleOutput: public QObject  // clazy:exclude=missing-qobject-macro
{
public:
    static ConsoleOutput* getInstance()
    {
        if (!instance) {
            instance = new ConsoleOutput;
        }
        return instance;
    }
    static void destruct()
    {
        delete instance;
        instance = nullptr;
    }

    void customEvent(QEvent* ev) override
    {
        if (ev->type() == QEvent::User) {
            switch (const auto ce = static_cast<ConsoleEvent*>(ev); ce->msgtype) {
                case ConsoleSingleton::MsgType_Txt:
                    Console().notifyPrivate(
                        LogStyle::Message,
                        ce->recipient,
                        ce->content,
                        ce->notifier,
                        ce->msg
                    );
                    break;
                case ConsoleSingleton::MsgType_Log:
                    Console().notifyPrivate(
                        LogStyle::Log,
                        ce->recipient,
                        ce->content,
                        ce->notifier,
                        ce->msg
                    );
                    break;
                case ConsoleSingleton::MsgType_Wrn:
                    Console().notifyPrivate(
                        LogStyle::Warning,
                        ce->recipient,
                        ce->content,
                        ce->notifier,
                        ce->msg
                    );
                    break;
                case ConsoleSingleton::MsgType_Err:
                    Console().notifyPrivate(
                        LogStyle::Error,
                        ce->recipient,
                        ce->content,
                        ce->notifier,
                        ce->msg
                    );
                    break;
                case ConsoleSingleton::MsgType_Critical:
                    Console().notifyPrivate(
                        LogStyle::Critical,
                        ce->recipient,
                        ce->content,
                        ce->notifier,
                        ce->msg
                    );
                    break;
                case ConsoleSingleton::MsgType_Notification:
                    Console().notifyPrivate(
                        LogStyle::Notification,
                        ce->recipient,
                        ce->content,
                        ce->notifier,
                        ce->msg
                    );
                    break;
            }
        }
    }

private:
    static ConsoleOutput* instance;  // NOLINT
};

ConsoleOutput* ConsoleOutput::instance = nullptr;  // NOLINT

}  // namespace Base

//**************************************************************************
// Construction destruction


ConsoleSingleton::ConsoleSingleton()
#ifdef FC_DEBUG
    : _defaultLogLevel(FC_LOGLEVEL_LOG)
#else
    : _defaultLogLevel(FC_LOGLEVEL_MSG)
#endif
{}

ConsoleSingleton::~ConsoleSingleton()
{
    ConsoleOutput::destruct();
    for (ILogger* Iter : _aclObservers) {  // NOLINT
        delete Iter;
    }
}


//**************************************************************************
// methods

/**
 * \a type can be OR'ed with any of the FreeCAD_ConsoleMsgType flags to enable -- if \a b is true --
 * or to disable -- if \a b is false -- a console observer with name \a sObs.
 * The return value is an OR'ed value of all message types that have changed their state. For
 * example
 * @code
 * // switch off warnings and error messages
 * ConsoleMsgFlags ret = Base::Console().SetEnabledMsgType("myObs",
 *                       Base:ConsoleSingleton::MsgType_Wrn|Base::ConsoleSingleton::MsgType_Err,
 * false);
 * // do something without notifying observer myObs
 * ...
 * // restore the former configuration again
 * Base::Console().SetEnabledMsgType("myObs", ret, true);
 * @endcode
 * switches off warnings and error messages and restore the state before the modification.
 * If the observer \a sObs doesn't exist then nothing happens.
 */
ConsoleMsgFlags ConsoleSingleton::setEnabledMsgType(
    const char* sObs,
    const ConsoleMsgFlags type,
    const bool on
) const
{
    if (ILogger* pObs = get(sObs)) {
        ConsoleMsgFlags flags = 0;

        if (type & MsgType_Err) {
            if (pObs->bErr != on) {
                flags |= MsgType_Err;
            }
            pObs->bErr = on;
        }
        if (type & MsgType_Wrn) {
            if (pObs->bWrn != on) {
                flags |= MsgType_Wrn;
            }
            pObs->bWrn = on;
        }
        if (type & MsgType_Txt) {
            if (pObs->bMsg != on) {
                flags |= MsgType_Txt;
            }
            pObs->bMsg = on;
        }
        if (type & MsgType_Log) {
            if (pObs->bLog != on) {
                flags |= MsgType_Log;
            }
            pObs->bLog = on;
        }
        if (type & MsgType_Critical) {
            if (pObs->bCritical != on) {
                flags |= MsgType_Critical;
            }
            pObs->bCritical = on;
        }
        if (type & MsgType_Notification) {
            if (pObs->bNotification != on) {
                flags |= MsgType_Notification;
            }
            pObs->bNotification = on;
        }

        return flags;
    }

    return 0;
}

bool ConsoleSingleton::isMsgTypeEnabled(const char* sObs, const FreeCAD_ConsoleMsgType type) const
{
    if (const ILogger* pObs = get(sObs)) {
        switch (type) {
            case MsgType_Txt:
                return pObs->bMsg;
            case MsgType_Log:
                return pObs->bLog;
            case MsgType_Wrn:
                return pObs->bWrn;
            case MsgType_Err:
                return pObs->bErr;
            case MsgType_Critical:
                return pObs->bCritical;
            case MsgType_Notification:
                return pObs->bNotification;
            default:
                return false;
        }
    }

    return false;
}

void ConsoleSingleton::setConnectionMode(const ConnectionMode mode)
{
    connectionMode = mode;

    // make sure this method gets called from the main thread
    if (connectionMode == Queued) {
        ConsoleOutput::getInstance();
    }
}

//**************************************************************************
// Observer stuff

/** Attaches an Observer to Console
 *  Use this method to attach a ILogger derived class to
 *  the Console. After the observer is attached all messages will also
 *  be forwarded to it.
 *  @see ILogger
 */
void ConsoleSingleton::attachObserver(ILogger* pcObserver)
{
    // double insert !!
    assert(!_aclObservers.contains(pcObserver));

    _aclObservers.insert(pcObserver);
}

/** Detaches an Observer from Console
 *  Use this method to detach a ILogger derived class.
 *  After detaching you can destruct the Observer or reinsert it later.
 *  @see ILogger
 */
void ConsoleSingleton::detachObserver(ILogger* pcObserver)
{
    _aclObservers.erase(pcObserver);
}

void ConsoleSingleton::notifyPrivate(
    const LogStyle category,
    const IntendedRecipient recipient,
    const ContentType content,
    const std::string& notifiername,
    const std::string& msg
) const
{
    for (ILogger* Iter : _aclObservers) {
        if (Iter->isActive(category)) {
            Iter->sendLog(
                notifiername,
                msg,
                category,
                recipient,
                content
            );  // send string to the listener
        }
    }
}

void ConsoleSingleton::postEvent(
    const FreeCAD_ConsoleMsgType type,
    const IntendedRecipient recipient,
    const ContentType content,
    const std::string& notifiername,
    const std::string& msg
)
{
    QCoreApplication::postEvent(
        ConsoleOutput::getInstance(),
        new ConsoleEvent(type, recipient, content, notifiername, msg)
    );
}

ILogger* ConsoleSingleton::get(const char* Name) const
{
    const char* OName {};
    for (ILogger* Iter : _aclObservers) {
        OName = Iter->name();  // get the name
        if (OName && strcmp(OName, Name) == 0) {
            return Iter;
        }
    }
    return nullptr;
}

int* ConsoleSingleton::getLogLevel(const char* tag, const bool create)
{
    if (!tag) {
        tag = "";
    }
    if (_logLevels.contains(tag)) {
        return &_logLevels[tag];
    }
    if (!create) {
        return nullptr;
    }
    int& ret = _logLevels[tag];
    ret = -1;
    return &ret;
}

void ConsoleSingleton::refresh() const
{
    if (_bCanRefresh) {
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
}

void ConsoleSingleton::enableRefresh(const bool enable)
{
    _bCanRefresh = enable;
}

//**************************************************************************
// Singleton stuff

ConsoleSingleton* ConsoleSingleton::_pcSingleton = nullptr;

void ConsoleSingleton::Destruct()
{
    // not initialized or double destructed!
    assert(_pcSingleton);
    delete _pcSingleton;
    _pcSingleton = nullptr;
}

ConsoleSingleton& ConsoleSingleton::instance()
{
    // not initialized?
    if (!_pcSingleton) {
        _pcSingleton = new ConsoleSingleton();
    }
    return *_pcSingleton;
}

ILogger::~ILogger() = default;

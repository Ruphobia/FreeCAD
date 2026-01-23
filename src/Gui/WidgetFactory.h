/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef GUI_WIDGETFACTORY_H
#define GUI_WIDGETFACTORY_H

#include <vector>

#include <Base/Factory.h>
#include "Dialogs/DlgCustomizeImp.h"
#include "Dialogs/DlgPreferencesImp.h"
#include "PropertyPage.h"

QT_BEGIN_NAMESPACE
class QDir;
QT_END_NAMESPACE

namespace Gui
{
namespace Dialog
{
class PreferencePage;
}

/**
 * The widget factory provides methods for the dynamic creation of widgets.
 * To create these widgets once they must be registered to the factory.
 * To register them use WidgetProducer or any subclasses; to register a
 * preference page use PrefPageProducer instead.
 * \author Werner Mayer
 */
class GuiExport WidgetFactoryInst: public Base::Factory
{
public:
    static WidgetFactoryInst& instance();
    static void destruct();

    QWidget* createWidget(const char* sName, QWidget* parent = nullptr) const;
    Gui::Dialog::PreferencePage* createPreferencePage(const char* sName, QWidget* parent = nullptr) const;
    QWidget* createPrefWidget(const char* sName, QWidget* parent, const char* sPref);

private:
    static WidgetFactoryInst* _pcSingleton;

    WidgetFactoryInst() = default;
    ~WidgetFactoryInst() override = default;
};

inline WidgetFactoryInst& WidgetFactory()
{
    return WidgetFactoryInst::instance();
}

// --------------------------------------------------------------------

/**
 * The WidgetProducer class is a value-based template class that provides
 * the ability to create widgets dynamically.
 * \author Werner Mayer
 */
template<class CLASS>
class WidgetProducer: public Base::AbstractProducer
{
public:
    /**
     * Register a special type of widget to the WidgetFactoryInst.
     */
    WidgetProducer()
    {
        const char* cname = CLASS::staticMetaObject.className();
        WidgetFactoryInst::instance().AddProducer(cname, this);
    }

    ~WidgetProducer() override = default;

    /**
     * Creates an instance of the specified widget.
     */
    void* Produce() const override
    {
        return (new CLASS);
    }
};

// --------------------------------------------------------------------

/**
 * The PrefPageProducer class is a value-based template class that provides
 * the ability to create preference pages dynamically.
 * \author Werner Mayer
 */
template<class CLASS>
class PrefPageProducer: public Base::AbstractProducer
{
public:
    /**
     * Register a special type of preference page to the WidgetFactoryInst.
     */
    PrefPageProducer(const char* group)
    {
        const char* cname = CLASS::staticMetaObject.className();
        if (strcmp(cname, Gui::Dialog::PreferencePage::staticMetaObject.className()) == 0) {
            qWarning("The class '%s' lacks of Q_OBJECT macro", typeid(CLASS).name());
        }
        if (WidgetFactoryInst::instance().CanProduce(cname)) {
            qWarning("The preference page class '%s' is already registered", cname);
        }
        else {
            WidgetFactoryInst::instance().AddProducer(cname, this);
            Gui::Dialog::DlgPreferencesImp::addPage(cname, group);
        }
    }

    ~PrefPageProducer() override = default;

    /**
     * Creates an instance of the specified widget.
     */
    void* Produce() const override
    {
        return (new CLASS);
    }
};

/**
 * The PrefPageUiProducer class provides the ability to create preference pages
 * dynamically from an external UI file.
 * @author Werner Mayer
 */
class GuiExport PrefPageUiProducer: public Base::AbstractProducer
{
public:
    /**
     * Register a special type of preference page to the WidgetFactoryInst.
     */
    PrefPageUiProducer(const char* filename, const char* group);
    ~PrefPageUiProducer() override;
    /**
     * Creates an instance of the specified widget.
     */
    void* Produce() const override;

private:
    QString fn;
};

// --------------------------------------------------------------------

/**
 * The CustomPageProducer class is a value-based template class that provides
 * the ability to create custom pages dynamically.
 * \author Werner Mayer
 */
template<class CLASS>
class CustomPageProducer: public Base::AbstractProducer
{
public:
    /**
     * Register a special type of customize page to the WidgetFactoryInst.
     */
    CustomPageProducer()
    {
        const char* cname = CLASS::staticMetaObject.className();
        if (strcmp(cname, Gui::Dialog::CustomizeActionPage::staticMetaObject.className()) == 0) {
            qWarning("The class '%s' lacks of Q_OBJECT macro", typeid(CLASS).name());
        }
        if (WidgetFactoryInst::instance().CanProduce(cname)) {
            qWarning("The preference page class '%s' is already registered", cname);
        }
        else {
            WidgetFactoryInst::instance().AddProducer(cname, this);
            Gui::Dialog::DlgCustomizeImp::addPage(cname);
        }
    }

    ~CustomPageProducer() override = default;

    /**
     * Creates an instance of the specified widget.
     */
    void* Produce() const override
    {
        return (new CLASS);
    }
};

// --------------------------------------------------------------------

/**
 * The widget factory supplier class registers all kinds of
 * preference pages and widgets.
 * \author Werner Mayer
 */
class WidgetFactorySupplier
{
private:
    // Singleton
    WidgetFactorySupplier();
    static WidgetFactorySupplier* _pcSingleton;

public:
    static WidgetFactorySupplier& instance();
    static void destruct();
    friend WidgetFactorySupplier& GetWidgetFactorySupplier();
};

inline WidgetFactorySupplier& GetWidgetFactorySupplier()
{
    return WidgetFactorySupplier::instance();
}

// ----------------------------------------------------

/**
 * The ContainerDialog class acts as a container to embed any kinds of widgets that
 * do not inherit from QDialog. This class also provides an "Ok" and a "Cancel" button.
 * At most this class is used to embed widgets which are created from .ui files.
 * \author Werner Mayer
 */
class ContainerDialog: public QDialog
{
    Q_OBJECT

public:
    ContainerDialog(QWidget* templChild);
    ~ContainerDialog() override;

    QPushButton* buttonOk;     /**< The Ok button. */
    QPushButton* buttonCancel; /**< The cancel button. */

private:
    QGridLayout* MyDialogLayout;
};

}  // namespace Gui

#endif  // GUI_WIDGETFACTORY_H

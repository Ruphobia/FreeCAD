// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <cassert>

#include "BaseClass.h"

using namespace Base;

Type BaseClass::classTypeId = Base::Type::BadType;


//**************************************************************************
// Construction/Destruction

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
BaseClass::BaseClass() = default;

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
BaseClass::~BaseClass() = default;


//**************************************************************************
// separator for other implementation aspects

void BaseClass::init()
{
    assert(BaseClass::classTypeId.isBad() && "don't init() twice!");
    /* Make sure superclass gets initialized before subclass. */
    /*assert(strcmp(#_parentclass_), "inherited"));*/
    /*Type parentType(Type::fromName(#_parentclass_));*/
    /*assert(!parentType.isBad() && "you forgot init() on parentclass!");*/

    /* Set up entry in the type system. */
    BaseClass::classTypeId = Type::createType(Type::BadType, "Base::BaseClass", BaseClass::create);
}

Type BaseClass::getClassTypeId()
{
    return BaseClass::classTypeId;
}

Type BaseClass::getTypeId() const
{
    return BaseClass::classTypeId;
}


void BaseClass::initSubclass(
    Base::Type& toInit,
    const char* ClassName,
    const char* ParentName,
    Type::instantiationMethod method
)
{
    // don't init twice!
    assert(toInit.isBad());
    // get the parent class
    Base::Type parentType(Base::Type::fromName(ParentName));
    // forgot init parent!
    assert(!parentType.isBad());

    // create the new type
    toInit = Base::Type::createType(parentType, ClassName, method);
}


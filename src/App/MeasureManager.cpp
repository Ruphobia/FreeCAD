// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david@friedli-be.ch>                *
 *   Copyright (c) 2023 Wandererfan <wandererfan@gmail.com>                *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include <Base/Console.h>
#include <App/Document.h>
#include <App/Link.h>

#include "MeasureManager.h"

namespace App
{

std::vector<MeasureHandler> MeasureManager::_mMeasureHandlers;
std::vector<MeasureType*> MeasureManager::_mMeasureTypes;

MeasureManager::MeasureManager()
{
    // Constructor implementation
}


void MeasureManager::addMeasureHandler(const char* module, MeasureTypeMethod typeCb)
{
    _mMeasureHandlers.emplace_back(MeasureHandler {module, typeCb});
}

bool MeasureManager::hasMeasureHandler(const char* module)
{
    for (MeasureHandler& handler : _mMeasureHandlers) {
        if (strcmp(handler.module.c_str(), module) == 0) {
            return true;
        }
    }
    return false;
}

MeasureHandler MeasureManager::getMeasureHandler(const char* module)
{
    for (MeasureHandler handler : _mMeasureHandlers) {
        if (!strcmp(handler.module.c_str(), module)) {
            return handler;
        }
    }

    MeasureHandler empty;
    return empty;
}

MeasureHandler MeasureManager::getMeasureHandler(const App::MeasureSelectionItem& selectionItem)
{
    auto objT = selectionItem.object;

    // Resolve App::Link
    App::DocumentObject* sub = objT.getSubObject();
    if (sub->isDerivedFrom<App::Link>()) {
        auto link = static_cast<App::Link*>(sub);
        sub = link->getLinkedObject(true);
    }

    const char* className = sub->getTypeId().getName();
    std::string mod = Base::Type::getModuleName(className);

    return getMeasureHandler(mod.c_str());
}

MeasureElementType
MeasureManager::getMeasureElementType(const App::MeasureSelectionItem& selectionItem)
{
    auto handler = getMeasureHandler(selectionItem);
    if (handler.module.empty()) {
        return App::MeasureElementType::INVALID;
    }

    auto objT = selectionItem.object;
    return handler.typeCb(objT.getObject(), objT.getSubName().c_str());
}

void MeasureManager::addMeasureType(MeasureType* measureType)
{
    _mMeasureTypes.push_back(measureType);
}

void MeasureManager::addMeasureType(std::string id,
                                    std::string label,
                                    std::string measureObj,
                                    MeasureValidateMethod validatorCb,
                                    MeasurePrioritizeMethod prioritizeCb)
{
    MeasureType* mType =
        new MeasureType {id, label, measureObj, validatorCb, prioritizeCb, false};
    _mMeasureTypes.push_back(mType);
}

void MeasureManager::addMeasureType(const char* id,
                                    const char* label,
                                    const char* measureObj,
                                    MeasureValidateMethod validatorCb,
                                    MeasurePrioritizeMethod prioritizeCb)
{
    addMeasureType(std::string(id),
                   std::string(label),
                   std::string(measureObj),
                   validatorCb,
                   prioritizeCb);
}

const std::vector<MeasureType*> MeasureManager::getMeasureTypes()
{
    return _mMeasureTypes;
}


std::vector<MeasureType*> MeasureManager::getValidMeasureTypes(App::MeasureSelection selection,
                                                               std::string mode)
{
    // Store valid measure types
    std::vector<MeasureType*> validTypes;

    // Loop through measure types and check if they work with given selection
    for (App::MeasureType* mType : getMeasureTypes()) {

        if (mode != "" && mType->label != mode) {
            continue;
        }

        if (!mType->isPython) {
            // Parse c++ measure types

            if (mType->validatorCb && !mType->validatorCb(selection)) {
                continue;
            }

            // Check if the measurement type prioritizes the given selection
            if (mType->prioritizeCb && mType->prioritizeCb(selection)) {
                validTypes.insert(validTypes.begin(), mType);
            }
            else {
                validTypes.push_back(mType);
            }
        }
    }

    return validTypes;
}


}  // namespace App

# SPDX-License-Identifier: LGPL-2.1-or-later

add_library(FreeCADBase SHARED)

if(WIN32)
    add_definitions(-DFCBase)
    add_definitions(-DPYCXX_DLL)
    add_definitions(-DBOOST_DYN_LINK)
    add_definitions(-DZIPIOS_UTF8)
endif(WIN32)

if(BUILD_TRACY_FRAME_PROFILER)
    add_definitions(-DBUILD_TRACY_FRAME_PROFILER)
endif()

target_include_directories(
    FreeCADBase
    PRIVATE
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    FreeCADBase
    SYSTEM
    PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${QtCore_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${ZIPIOS_INCLUDES}
    ${ICU_INCLUDE_DIRS}
)

if(MSVC)
    set(FreeCADBase_LIBS
        ${Boost_LIBRARIES}
        ${XercesC_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${ICU_LIBRARIES}
        debug MSVCRTD.LIB
        debug MSVCPRTD.LIB
        optimized MSVCRT.LIB
        optimized MSVCPRT.LIB
        Rpcrt4.lib
        version.lib
    )

    #Universal C runtime introduced in VS 2015 (cl version 19)
    if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19"))
        list(APPEND FreeCADBase_LIBS
            debug vcruntimed.lib
            debug ucrtd.lib
            debug concrtd.lib
            optimized vcruntime.lib
            optimized ucrt.lib
            optimized concrt.lib
        )
    endif()
elseif(MINGW)
    set(FreeCADBase_LIBS
        ${XercesC_LIBRARIES}
        ${Boost_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${ICU_LIBRARIES}
        Rpcrt4.lib
    )
else(MSVC)
    set(FreeCADBase_LIBS
        ${XercesC_LIBRARIES}
        ${Boost_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${ICU_LIBRARIES}
    )
endif(MSVC)

if(BUILD_TRACY_FRAME_PROFILER)
    list(APPEND FreeCADBase_LIBS TracyClient)
endif()

list(APPEND FreeCADBase_LIBS ${QtCore_LIBRARIES})

list(APPEND FreeCADBase_LIBS libfastsignals fmt::fmt)

# Strip: Python removed

# needed for OpenSUSE
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    list(APPEND FreeCADBase_LIBS -lutil -ldl)
endif()

# Strip: Python type generation removed
generate_from_any(Parameter.xsd Parameter.inl xmlSchemeString)

# Strip: SWIG and PyCXX removed

# auto-generate resource file with all available translations
set (FreeCADBase_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Base_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${FreeCADBase_TR_QRC} ${QM_SRCS})
qt_add_resources(FreeCADBase_QRC_SRCS ${FreeCADBase_TR_QRC})

# Strip: Python .pyi files removed
SET(FreeCADBase_Pyi_SRCS)

SET(FreeCADBase_UNITAPI_SRCS
    UnitsApi.cpp
    UnitsApi.h
    UnitsConvData.h
    UnitsSchema.h
    UnitsSchema.cpp
    UnitsSchemas.cpp
    UnitsSchemas.h
    UnitsSchemasData.h
    UnitsSchemasSpecs.h
    Quantity.h
    Quantity.cpp
    Quantity.l
    Quantity.y
    Unit.h
    Unit.cpp
)
SOURCE_GROUP("Units" FILES ${FreeCADBase_UNITAPI_SRCS})

SET(FreeCADBase_CPP_SRCS
    Axis.cpp
    Base64.cpp
    BaseClass.cpp
    Builder3D.cpp
    Console.cpp
    ConsoleObserver.cpp
    Color.cpp
    CoordinateSystem.cpp
    Debugger.cpp
    DualQuaternion.cpp
    Exception.cpp
    ExceptionFactory.cpp
    Factory.cpp
    FileInfo.cpp
    FutureWatcherProgress.cpp
    Handle.cpp
    InputSource.cpp
    Interpreter.cpp
    Matrix.cpp
    Observer.cpp
    Parameter.xsd
    Parameter.cpp
    Persistence.cpp
    Placement.cpp
    ProgressIndicator.cpp
    Reader.cpp
    Rotation.cpp
    Sequencer.cpp
    ServiceProvider.cpp
    Stream.cpp
    Swap.cpp
    Tools.cpp
    Tools2D.cpp
    Tools3D.cpp
    Translate.cpp
    Type.cpp
    UniqueNameManager.cpp
    Uuid.cpp
    Vector3D.cpp
    ViewProj.cpp
    Writer.cpp
    XMLTools.cpp
    ZipHeader.cpp
)

SET(FreeCADBase_HPP_SRCS
    Axis.h
    Base64.h
    Base64Filter.h
    BaseClass.h
    Bitmask.h
    BoundBox.h
    Builder3D.h
    Console.h
    ConsoleObserver.h
    Converter.h
    Color.h
    CoordinateSystem.h
    Debugger.h
    DualNumber.h
    DualQuaternion.h
    Exception.h
    ExceptionFactory.h
    Factory.h
    FileInfo.h
    FutureWatcherProgress.h
    Handle.h
    InputSource.h
    Interpreter.h
    Matrix.h
    Observer.h
    Parameter.h
    Persistence.h
    Placement.h
    Precision.h
    ProgressIndicator.h
    Reader.h
    Rotation.h
    ServiceProvider.h
    Sequencer.h
    Stream.h
    Swap.h
    TimeInfo.h
    Tools.h
    Tools2D.h
    Tools3D.h
    Translate.h
    Type.h
    UnlimitedUnsigned.h
    UniqueNameManager.h
    Uuid.h
    Vector3D.h
    ViewProj.h
    Writer.h
    XMLTools.h
    ZipHeader.h
)

SET(FreeCADBase_SRCS
    ${FreeCADBase_QRC_SRCS}
    ${FreeCADBase_CPP_SRCS}
    ${FreeCADBase_HPP_SRCS}
    ${FreeCADBase_UNITAPI_SRCS}
    PreCompiled.h
)

IF (MSVC)
    SET(FreeCADBase_SRCS
        ${FreeCADBase_SRCS}
        StackWalker.cpp
        StackWalker.h
    )
ENDIF(MSVC)

if(FREECAD_USE_EXTERNAL_ZIPIOS)
    list(APPEND FreeCADBase_LIBS ${ZIPIOS_LIBRARY})
else()
    list(APPEND FreeCADBase_SRCS ${zipios_SRCS})
    SOURCE_GROUP("zipios" FILES ${zipios_SRCS})
endif()

if(FREECAD_USE_PCH)
    target_precompile_headers(FreeCADBase PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_sources(FreeCADBase PRIVATE ${FreeCADBase_SRCS})
target_link_libraries(FreeCADBase ${FreeCADBase_LIBS})

if (MSVC)
    target_compile_definitions(FreeCADBase PRIVATE WIN32_LEAN_AND_MEAN)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(FreeCADBase)
endif()

SET_BIN_DIR(FreeCADBase FreeCADBase)

if(WIN32)
    INSTALL(TARGETS FreeCADBase
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else(WIN32)
    INSTALL(TARGETS FreeCADBase
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif(WIN32)

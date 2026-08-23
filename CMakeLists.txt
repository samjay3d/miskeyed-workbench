cmake_minimum_required(VERSION 3.24)
project(slang_qrhi VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)

option(SLANG_QRHI_BUILD_APP "Build native standalone workbench" ON)
option(SLANG_QRHI_BUILD_PYTHON "Build Shiboken6 Python bindings" ON)
option(SLANG_QRHI_WITH_ANARI "Build the optional native ANARI backend foundation" OFF)

# Build inputs come from the environment so a clean checkout and CI build identically,
# with no machine-specific paths:
#   * Qt 6.8 dev SDK  ->  CMAKE_PREFIX_PATH
#   * Slang SDK       ->  SLANG_ROOT      (see cmake/FindSlang.cmake)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Python 3.11 REQUIRED COMPONENTS Interpreter Development.Module)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Slang REQUIRED)

if(SLANG_QRHI_WITH_ANARI)
    # ANARI stays in a separate native target so the shipped Shader Toy and Render Toy
    # dependency graph is identical when this experimental backend is disabled.
    find_package(anari CONFIG REQUIRED)

    add_library(slang_qrhi_anari_backend STATIC
        cpp/src/anari/AnariCandidates.cpp
        cpp/src/anari/AnariDeviceCatalog.cpp
        cpp/src/anari/AnariLibrary.cpp
        cpp/src/anari/AnariDeviceSession.cpp
    )
    target_include_directories(slang_qrhi_anari_backend
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cpp/include>
            $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(slang_qrhi_anari_backend PUBLIC anari::anari)

    add_executable(slang-qrhi-anari-probe app/anari_probe.cpp)
    target_link_libraries(slang-qrhi-anari-probe PRIVATE slang_qrhi_anari_backend)

    include(CTest)
    if(BUILD_TESTING)
        add_executable(test_anari_candidates tests/test_anari_candidates.cpp)
        target_link_libraries(test_anari_candidates PRIVATE slang_qrhi_anari_backend)
        add_test(NAME anari_candidates COMMAND test_anari_candidates)
    endif()
endif()

add_library(slang_qrhi_core STATIC
    cpp/src/Digest.cpp
    cpp/src/DependencyGraph.cpp
    cpp/src/ShaderParameterModel.cpp
    cpp/src/ParameterInspector.cpp
    cpp/src/SlangCompiler.cpp
    cpp/src/Qt68ShaderBridge.cpp
    cpp/src/ShaderDocument.cpp
    cpp/src/SlangRhiWidget.cpp
    cpp/src/CodeEditor.cpp
    cpp/src/ShaderHighlighter.cpp
    cpp/src/LspClient.cpp
    cpp/src/WorkbenchWindow.cpp
    # Q_OBJECT headers live in cpp/include; list them so AUTOMOC scans them (its
    # basename heuristic only checks the source dir, which would skip moc otherwise).
    cpp/include/slang_qrhi/DependencyGraph.h
    cpp/include/slang_qrhi/ShaderParameterModel.h
    cpp/include/slang_qrhi/ParameterInspector.h
    cpp/include/slang_qrhi/ShaderDocument.h
    cpp/include/slang_qrhi/SlangCompiler.h
    cpp/include/slang_qrhi/SlangRhiWidget.h
    cpp/include/slang_qrhi/CodeEditor.h
    cpp/include/slang_qrhi/ShaderHighlighter.h
    cpp/include/slang_qrhi/LspClient.h
    cpp/include/slang_qrhi/WorkbenchWindow.h
)

target_compile_definitions(slang_qrhi_core PUBLIC SLANG_QRHI_STATIC)
target_include_directories(slang_qrhi_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cpp/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(slang_qrhi_core
    PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets
    PRIVATE Qt6::GuiPrivate Slang::slang
)

if(SLANG_QRHI_BUILD_APP)
    add_executable(slang-qrhi app/main.cpp)
    target_link_libraries(slang-qrhi PRIVATE slang_qrhi_core)
    install(TARGETS slang-qrhi RUNTIME DESTINATION bin)
endif()

if(SLANG_QRHI_BUILD_PYTHON)
    include(ShibokenBindings)
    slang_qrhi_add_shiboken_module(
        TARGET _slang_qrhi
        MODULE_NAME _slang_qrhi
        TYPESYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/bindings/typesystem_slang_qrhi.xml"
        GLOBAL_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/bindings/slang_qrhi_bindings.h"
        WRAPPED_HEADERS
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/slang_qrhi/DependencyGraph.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/slang_qrhi/ShaderParameterModel.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/slang_qrhi/ShaderDocument.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/slang_qrhi/ParameterInspector.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/slang_qrhi/SlangRhiWidget.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/slang_qrhi/WorkbenchWindow.h"
        LINK_LIBRARIES slang_qrhi_core
    )

    # Ship the Slang runtime next to the extension so the wheel imports without
    # extra PATH setup (Qt/shiboken/pyside DLLs come from the PySide6 dependency).
    if(WIN32)
        find_file(SLANG_RUNTIME_DLL NAMES slang-compiler.dll slang.dll
            HINTS "${slang_DIR}/../bin" "${SLANG_ROOT}/bin" "$ENV{SLANG_ROOT}/bin")
        if(SLANG_RUNTIME_DLL)
            get_filename_component(_slang_bin_dir "${SLANG_RUNTIME_DLL}" DIRECTORY)
            file(GLOB _slang_runtime_dlls "${_slang_bin_dir}/*.dll")
            install(FILES ${_slang_runtime_dlls} DESTINATION miskeyed/workbench)
        endif()
    endif()
endif()

install(TARGETS slang_qrhi_core ARCHIVE DESTINATION lib)
install(DIRECTORY cpp/include/slang_qrhi DESTINATION include)
install(DIRECTORY shaders/ DESTINATION miskeyed/workbench/shaders)

if(SLANG_QRHI_WITH_ANARI)
    install(TARGETS slang_qrhi_anari_backend ARCHIVE DESTINATION lib)
    install(TARGETS slang-qrhi-anari-probe RUNTIME DESTINATION bin)
endif()

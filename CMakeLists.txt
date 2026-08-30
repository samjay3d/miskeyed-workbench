cmake_minimum_required(VERSION 3.24)
project(miskeyed_workbench VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)

option(SLANG_QRHI_BUILD_APP "Build native standalone workbench" ON)
option(SLANG_QRHI_BUILD_PYTHON "Build Shiboken6 Python bindings" ON)
option(MISKEYED_WORKBENCH_WITH_ANARI "Build the optional native ANARI backend foundation" OFF)

# Build inputs come from the environment so a clean checkout and CI build identically,
# with no machine-specific paths:
#   * Qt 6.8 dev SDK  ->  CMAKE_PREFIX_PATH
#   * Slang SDK       ->  SLANG_ROOT      (see cmake/FindSlang.cmake)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Python 3.11 REQUIRED COMPONENTS Interpreter Development.Module)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Slang REQUIRED)

if(MISKEYED_WORKBENCH_WITH_ANARI)
    # ANARI stays in a separate native target so the shipped Shader Toy and Render Toy
    # dependency graph is identical when this experimental backend is disabled.
    find_package(anari CONFIG REQUIRED)

    add_library(miskeyed_workbench_anari_backend STATIC
        cpp/src/workbench/anari/AnariCandidates.cpp
        cpp/src/workbench/anari/AnariDeviceCatalog.cpp
        cpp/src/workbench/anari/AnariLibrary.cpp
        cpp/src/workbench/anari/AnariDeviceSession.cpp
    )
    target_include_directories(miskeyed_workbench_anari_backend
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cpp/include>
            $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(miskeyed_workbench_anari_backend PUBLIC anari::anari)
    add_library(miskeyed::workbench::anari_backend ALIAS miskeyed_workbench_anari_backend)

    add_executable(workbench-anari-probe app/anari_probe.cpp)
    target_link_libraries(workbench-anari-probe PRIVATE miskeyed_workbench_anari_backend)

    include(CTest)
    if(BUILD_TESTING)
        add_executable(test_anari_candidates tests/test_anari_candidates.cpp)
        target_link_libraries(test_anari_candidates PRIVATE miskeyed_workbench_anari_backend)
        add_test(NAME anari_candidates COMMAND test_anari_candidates)
    endif()
endif()

add_library(miskeyed_workbench_slang_rhi STATIC
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
    cpp/include/miskeyed/workbench/slang_rhi/DependencyGraph.h
    cpp/include/miskeyed/workbench/slang_rhi/ShaderParameterModel.h
    cpp/include/miskeyed/workbench/slang_rhi/ParameterInspector.h
    cpp/include/miskeyed/workbench/slang_rhi/ShaderDocument.h
    cpp/include/miskeyed/workbench/slang_rhi/SlangCompiler.h
    cpp/include/miskeyed/workbench/slang_rhi/SlangRhiWidget.h
    cpp/include/miskeyed/workbench/slang_rhi/CodeEditor.h
    cpp/include/miskeyed/workbench/slang_rhi/ShaderHighlighter.h
    cpp/include/miskeyed/workbench/slang_rhi/LspClient.h
    cpp/include/miskeyed/workbench/slang_rhi/WorkbenchWindow.h
)

target_compile_definitions(miskeyed_workbench_slang_rhi PUBLIC MISKEYED_WORKBENCH_SLANG_RHI_STATIC)
target_include_directories(miskeyed_workbench_slang_rhi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cpp/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(miskeyed_workbench_slang_rhi
    PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets
    PRIVATE Qt6::GuiPrivate Slang::slang
)
add_library(miskeyed::workbench::slang_rhi ALIAS miskeyed_workbench_slang_rhi)

if(MISKEYED_WORKBENCH_WITH_ANARI AND BUILD_TESTING)
    # Compile both native surfaces together so later backend work cannot blur the
    # device-neutral Workbench namespace into the established renderer namespace.
    add_executable(test_native_namespace_boundaries tests/test_native_namespace_boundaries.cpp)
    target_link_libraries(test_native_namespace_boundaries
        PRIVATE
            miskeyed::workbench::slang_rhi
            miskeyed::workbench::anari_backend
    )
    add_test(NAME native_namespace_boundaries COMMAND test_native_namespace_boundaries)
endif()

if(SLANG_QRHI_BUILD_APP)
    add_executable(slang-qrhi app/main.cpp)
    target_link_libraries(slang-qrhi PRIVATE miskeyed_workbench_slang_rhi)
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
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/miskeyed/workbench/slang_rhi/DependencyGraph.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/miskeyed/workbench/slang_rhi/ShaderParameterModel.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/miskeyed/workbench/slang_rhi/ShaderDocument.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/miskeyed/workbench/slang_rhi/ParameterInspector.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/miskeyed/workbench/slang_rhi/SlangRhiWidget.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/miskeyed/workbench/slang_rhi/WorkbenchWindow.h"
        LINK_LIBRARIES miskeyed_workbench_slang_rhi
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

install(TARGETS miskeyed_workbench_slang_rhi ARCHIVE DESTINATION lib)
install(DIRECTORY cpp/include/miskeyed/workbench/slang_rhi DESTINATION include/miskeyed/workbench)
install(DIRECTORY shaders/ DESTINATION miskeyed/workbench/shaders)

if(MISKEYED_WORKBENCH_WITH_ANARI)
    install(TARGETS miskeyed_workbench_anari_backend ARCHIVE DESTINATION lib)
    install(TARGETS workbench-anari-probe RUNTIME DESTINATION bin)
    install(DIRECTORY cpp/include/miskeyed DESTINATION include)
endif()

function(workbench_add_shiboken_module)
    set(options)
    set(oneValueArgs TARGET MODULE_NAME TYPESYSTEM GLOBAL_HEADER)
    set(multiValueArgs WRAPPED_HEADERS LINK_LIBRARIES)
    cmake_parse_arguments(WB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    execute_process(COMMAND "${Python_EXECUTABLE}" -c
        "import pathlib, shiboken6_generator; print(pathlib.Path(shiboken6_generator.__file__).resolve().parent)"
        OUTPUT_VARIABLE SHIBOKEN_GENERATOR_DIR OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
    execute_process(COMMAND "${Python_EXECUTABLE}" -c
        "import pathlib, PySide6; print(pathlib.Path(PySide6.__file__).resolve().parent)"
        OUTPUT_VARIABLE PYSIDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
    execute_process(COMMAND "${Python_EXECUTABLE}" -c
        "import pathlib, shiboken6; print(pathlib.Path(shiboken6.__file__).resolve().parent)"
        OUTPUT_VARIABLE SHIBOKEN_DIR OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)

    find_program(SHIBOKEN_EXECUTABLE NAMES shiboken6 shiboken6.exe
        HINTS "${SHIBOKEN_GENERATOR_DIR}" "${SHIBOKEN_GENERATOR_DIR}/scripts")
    if(NOT SHIBOKEN_EXECUTABLE)
        message(FATAL_ERROR "shiboken6 generator executable not found. Install shiboken6_generator from Qt's official wheel index.")
    endif()

    get_property(qt_core_includes TARGET Qt6::Core PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    get_property(qt_gui_includes TARGET Qt6::Gui PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    get_property(qt_widgets_includes TARGET Qt6::Widgets PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    set(generator_includes "-I${CMAKE_CURRENT_SOURCE_DIR}/cpp/include")
    # On Unix the generator wheel carries libclang but not Clang's builtin C headers.
    # Feed it the active toolchain's resource directory explicitly; otherwise parsing
    # the C++ standard library fails at <stddef.h> before it reaches our API.
    find_program(WORKBENCH_CLANG_EXECUTABLE NAMES clang clang-18 clang-17 clang-16)
    if(WORKBENCH_CLANG_EXECUTABLE)
        execute_process(COMMAND "${WORKBENCH_CLANG_EXECUTABLE}" -print-resource-dir
            OUTPUT_VARIABLE _clang_resource_dir OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(EXISTS "${_clang_resource_dir}/include/stddef.h")
            list(APPEND generator_includes "-I${_clang_resource_dir}/include")
        endif()
    endif()
    foreach(dir IN LISTS qt_core_includes qt_gui_includes qt_widgets_includes)
        list(APPEND generator_includes "-I${dir}")
        if(APPLE AND dir MATCHES "^(.*)/[^/]+\\.framework(/Headers)?$")
            # Framework-style includes such as <QtCore/qglobal.h> are resolved by the
            # framework search root, not by adding QtCore.framework/Headers as -I.
            list(APPEND generator_includes "-F${CMAKE_MATCH_1}")
        endif()
    endforeach()
    # QShader ships in Qt's private "rhi" tree. GuiPrivate exposes those dirs only via
    # generator expressions, which execute_process cannot evaluate, so derive the
    # versioned private include dirs from the public QtGui include directory.
    foreach(dir IN LISTS qt_gui_includes)
        if(dir MATCHES "/QtGui$")
            list(APPEND generator_includes "-I${dir}/${Qt6_VERSION}" "-I${dir}/${Qt6_VERSION}/QtGui")
        elseif(APPLE AND dir MATCHES "/QtGui\\.framework/Headers$")
            list(APPEND generator_includes
                "-I${dir}/${Qt6_VERSION}" "-I${dir}/${Qt6_VERSION}/QtGui")
        endif()
    endforeach()

    set(gen_root "${CMAKE_CURRENT_BINARY_DIR}/shiboken")
    file(MAKE_DIRECTORY "${gen_root}")
    execute_process(
        COMMAND "${SHIBOKEN_EXECUTABLE}"
            --generator-set=shiboken
            --enable-parent-ctor-heuristic
            --enable-pyside-extensions
            --avoid-protected-hack
            --output-directory=${gen_root}
            --typesystem-paths=${PYSIDE_DIR}/typesystems
            ${generator_includes}
            ${WB_GLOBAL_HEADER}
            ${WB_TYPESYSTEM}
        RESULT_VARIABLE shiboken_result
        OUTPUT_VARIABLE shiboken_output
        ERROR_VARIABLE shiboken_error)
    if(NOT shiboken_result EQUAL 0)
        message(FATAL_ERROR "Shiboken generation failed:\n${shiboken_output}\n${shiboken_error}")
    endif()

    file(GLOB_RECURSE generated_sources CONFIGURE_DEPENDS "${gen_root}/${WB_MODULE_NAME}/*_wrapper.cpp")
    if(NOT generated_sources)
        message(FATAL_ERROR "Shiboken produced no wrapper sources under ${gen_root}/${WB_MODULE_NAME}")
    endif()

    if(WIN32)
        find_library(PYSIDE_LIBRARY NAMES pyside6.abi3 pyside6 HINTS "${PYSIDE_DIR}" REQUIRED)
        find_library(SHIBOKEN_LIBRARY NAMES shiboken6.abi3 shiboken6 HINTS "${SHIBOKEN_DIR}" REQUIRED)
    elseif(APPLE)
        find_file(PYSIDE_LIBRARY NAMES libpyside6.abi3.6.8.dylib
            HINTS "${PYSIDE_DIR}" REQUIRED)
        find_file(SHIBOKEN_LIBRARY NAMES libshiboken6.abi3.6.8.dylib
            HINTS "${SHIBOKEN_DIR}" REQUIRED)
    else()
        find_file(PYSIDE_LIBRARY NAMES libpyside6.abi3.so.6.8
            HINTS "${PYSIDE_DIR}" REQUIRED)
        find_file(SHIBOKEN_LIBRARY NAMES libshiboken6.abi3.so.6.8
            HINTS "${SHIBOKEN_DIR}" REQUIRED)
    endif()

    add_library(${WB_TARGET} MODULE ${generated_sources})
    set_target_properties(${WB_TARGET} PROPERTIES PREFIX "" OUTPUT_NAME "${WB_MODULE_NAME}")
    # Wheels place the Slang runtime beside the extension. Keep loader policy at this
    # packaging edge instead of teaching Python or renderer code about ELF/Mach-O.
    if(APPLE)
        set_target_properties(${WB_TARGET} PROPERTIES INSTALL_RPATH "@loader_path")
    elseif(UNIX)
        set_target_properties(${WB_TARGET} PROPERTIES INSTALL_RPATH "$ORIGIN")
    endif()
    # Python imports .pyd on Windows; a MODULE lib defaults to .dll otherwise.
    if(WIN32)
        set_target_properties(${WB_TARGET} PROPERTIES SUFFIX ".pyd")
    endif()
    # Shiboken C++ dev headers (shiboken.h, sbk*.h) ship in the shiboken6_generator
    # wheel's include/ dir; the shiboken6 runtime package only carries the import lib.
    # PySide's per-module wrapper headers (pyside6_qtcore_python.h, ...) live in
    # include/<Module> subdirs, so each linked Qt module needs its own -I.
    target_include_directories(${WB_TARGET} PRIVATE
        "${PYSIDE_DIR}/include"
        "${PYSIDE_DIR}/include/QtCore"
        "${PYSIDE_DIR}/include/QtGui"
        "${PYSIDE_DIR}/include/QtWidgets"
        "${SHIBOKEN_GENERATOR_DIR}/include"
        "${SHIBOKEN_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include")
    target_link_libraries(${WB_TARGET} PRIVATE ${WB_LINK_LIBRARIES} Qt6::Core Qt6::Gui Qt6::GuiPrivate Qt6::Widgets Python::Module "${PYSIDE_LIBRARY}" "${SHIBOKEN_LIBRARY}")
    install(TARGETS ${WB_TARGET} LIBRARY DESTINATION miskeyed/workbench RUNTIME DESTINATION miskeyed/workbench)
endfunction()

cmake_minimum_required(VERSION 3.24)
project(miskeyed_workbench_anari_probe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(anari CONFIG REQUIRED)

set(MISKEYED_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")

add_library(miskeyed_workbench_anari_backend STATIC
    "${MISKEYED_ROOT}/cpp/src/workbench/anari/AnariCandidates.cpp"
    "${MISKEYED_ROOT}/cpp/src/workbench/anari/AnariDeviceCatalog.cpp"
    "${MISKEYED_ROOT}/cpp/src/workbench/anari/AnariLibrary.cpp"
    "${MISKEYED_ROOT}/cpp/src/workbench/anari/AnariDeviceSession.cpp"
)
target_include_directories(miskeyed_workbench_anari_backend PUBLIC "${MISKEYED_ROOT}/cpp/include")
target_link_libraries(miskeyed_workbench_anari_backend PUBLIC anari::anari)

add_executable(workbench-anari-probe "${MISKEYED_ROOT}/app/anari_probe.cpp")
target_link_libraries(workbench-anari-probe PRIVATE miskeyed_workbench_anari_backend)

include(CTest)
if(BUILD_TESTING)
    add_executable(test_anari_candidates "${MISKEYED_ROOT}/tests/test_anari_candidates.cpp")
    target_link_libraries(test_anari_candidates PRIVATE miskeyed_workbench_anari_backend)
    add_test(NAME anari_candidates COMMAND test_anari_candidates)
endif()

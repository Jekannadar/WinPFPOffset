cmake_minimum_required(VERSION 3.27)
set(CMAKE_CXX_STANDARD 20)
set(VCPKG_TARGET_TRIPLET x64-windows-static)
set(VCPKG_DEFAULT_TRIPLET x64-windows-static)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(PFPOffset)
cmake_path(GET CMAKE_TOOLCHAIN_FILE PARENT_PATH rootDir)
LINK_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows-static/lib)
find_package(CGAL CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
#set(GFLAGS_NAMESPACE=gflags)
#find_package(gflags CONFIG REQUIRED)
find_package(MPFR REQUIRED)
find_package(GMP REQUIRED)
find_package(Threads REQUIRED)

# --------------------------------------------------

function (build_external_project target file_name tag)

    set(CMAKELIST_CONTENT "
        cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

        project(build_external_project)

        include(ExternalProject)
        ExternalProject_add(${target}
            GIT_REPOSITORY \"${file_name}\"
            GIT_TAG \"${tag}\"
            CMAKE_GENERATOR \"${CMAKE_GENERATOR}\"
            CMAKE_GENERATOR_PLATFORM \"${CMAKE_GENERATOR_PLATFORM}\"
            CMAKE_GENERATOR_TOOLSET \"${CMAKE_GENERATOR_TOOLSET}\"
            CMAKE_GENERATOR_INSTANCE \"${CMAKE_GENERATOR_INSTANCE}\"
            CMAKE_ARGS ${ARGN})

        add_custom_target(build_external_project)
        add_dependencies(build_external_project ${target})
    ")

    set(TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/ExternalProjects/${target}")

    file(WRITE "${TARGET_DIR}/CMakeLists.txt" "${CMAKELIST_CONTENT}")

    file(MAKE_DIRECTORY "${TARGET_DIR}" "${TARGET_DIR}/build")

    execute_process(COMMAND ${CMAKE_COMMAND}
            -G "${CMAKE_GENERATOR}"
            -A "${CMAKE_GENERATOR_PLATFORM}"
            -T "${CMAKE_GENERATOR_TOOLSET}"
            ..
            WORKING_DIRECTORY "${TARGET_DIR}/build")

    execute_process(COMMAND ${CMAKE_COMMAND}
            --build .
            --config ${CMAKE_BUILD_TYPE}
            WORKING_DIRECTORY "${TARGET_DIR}/build")

endfunction()

#----------------------------------------------------------------------------------------------------

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(THIRD_PARTY_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/ThirdParty")
cmake_path(SET CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE})

#----------------------------------------------------------------------------------------------------
set(OSQP_OPTIONS "")
list(APPEND OSQP_OPTIONS -D CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
list(APPEND OSQP_OPTIONS -D CMAKE_INSTALL_PREFIX=${THIRD_PARTY_INSTALL_DIR}/osqp)
list(APPEND OSQP_OPTIONS -D BUILD_SHARED_LIBS=OFF)
if(${CMAKE_BUILD_TYPE} MATCHES "Release")
    list(APPEND OSQP_OPTIONS -D CMAKE_BUILD_TYPE=Release)
endif ()
build_external_project(osqp https://github.com/osqp/osqp.git v0.6.3 ${OSQP_OPTIONS})

set(OSQPE_OPTIONS "")
list(APPEND OSQPE_OPTIONS -D CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
list(APPEND OSQPE_OPTIONS -D CMAKE_INSTALL_PREFIX=${THIRD_PARTY_INSTALL_DIR}/osqp-eigen)
list(APPEND OSQPE_OPTIONS -D osqp_DIR=${THIRD_PARTY_INSTALL_DIR}/osqp/lib/cmake/osqp)
list(APPEND OSQPE_OPTIONS -D Eigen3_DIR=${rootDir}/../../packages/eigen3_x64-windows-static/share/eigen3)
list(APPEND OSQPE_OPTIONS -D BUILD_SHARED_LIBS=OFF)
if(${CMAKE_BUILD_TYPE} MATCHES "Release")
    list(APPEND OSQPE_OPTIONS -D CMAKE_BUILD_TYPE=Release)
endif ()
build_external_project(osqp-eigen https://github.com/robotology/osqp-eigen.git v0.8.1 ${OSQPE_OPTIONS})

set(GFLAGS_OPTIONS "")
list(APPEND GFLAGS_OPTIONS -D CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
list(APPEND GFLAGS_OPTIONS -D CMAKE_INSTALL_PREFIX=${THIRD_PARTY_INSTALL_DIR}/gflags)
list(APPEND GFLAGS_OPTIONS -D GFLAGS_BUILD_STATIC_LIBS=ON)
list(APPEND GFLAGS_OPTIONS -D GFLAGS_BUILD_SHARED_LIBS=OFF)
list(APPEND GFLAGS_OPTIONS -D GFLAGS_INSTALL_STATIC_LIBS=ON)
list(APPEND GFLAGS_OPTIONS -D GFLAGS_INSTALL_SHARED_LIBS=OFF)
if(${CMAKE_BUILD_TYPE} MATCHES "Release")
    list(APPEND GFLAGS_OPTIONS -D CMAKE_BUILD_TYPE=Release)
endif ()
build_external_project(gflags https://github.com/gflags/gflags.git v2.2.2 ${GFLAGS_OPTIONS})


include_directories(vcglib/)

find_package(osqp REQUIRED PATHS "${THIRD_PARTY_INSTALL_DIR}/osqp" NO_DEFAULT_PATH)
find_package(OsqpEigen REQUIRED PATHS "${THIRD_PARTY_INSTALL_DIR}/osqp-eigen" NO_DEFAULT_PATH)
find_package(gflags REQUIRED PATHS "${THIRD_PARTY_INSTALL_DIR}/gflags" NO_DEFAULT_PATH)

include_directories(${Boost_INCLUDE_DIRS})


if(${CMAKE_BUILD_TYPE} MATCHES "Release")
    message(STATUS "Release版本")
    set(BuildType "Release")
    set(CMAKE_C_FLAGS "$ENV{CFLAGS} /EHsc /GL /GS-")
    set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} /EHsc /GL /GS-")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
else()
    message(STATUS "Debug版本")
    set(BuildType "Debug")
    set(CMAKE_C_FLAGS "$ENV{CFLAGS} /EHsc /bigobj")
    set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} /EHsc /bigobj")
endif()

file (GLOB CppFile
        MeshKernel/*
        Eigen/*
)
add_executable(PFPOffset main.cpp DSU.cpp remeshing.cpp ${CppFile} )
if(APPLE)
    target_link_libraries(PFPOffset
            /opt/homebrew/lib/libgmp.dylib
            -lpthread
            /opt/homebrew/lib/libmpfr.dylib
            ${Boost_LIBRARY_DIRS}
            OsqpEigen::OsqpEigen
            /opt/homebrew/lib/libgflags.dylib
    )
else()
    target_link_libraries(PFPOffset
            gmp
            Threads::Threads
            ${MPFR_LIBRARIES}
            ${Boost_LIBRARY_DIRS}
            OsqpEigen::OsqpEigen
            gflags
    )
endif()

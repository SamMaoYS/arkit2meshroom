cmake_minimum_required(VERSION 3.11)

if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project(converter)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fext-numeric-literals -pthread -DBOOST_LOG_DYN_LINK -fopenmp")
set(CMAKE_CXX_STANDARD 11)

find_package(OpenMP REQUIRED)

# prefix setup
set(DEPENDENCIES_DIR "" CACHE PATH "/local/multiscan/multiscan/dependencies")
set(MESHROOM_ROOT "${DEPENDENCIES_DIR}/meshroom")
set(ALICEVISION_INSTALL_DIRS "${MESHROOM_ROOT}/install")

# include ALICEVISION
set(ALICEVISION_INCLUDE_DIRS "${ALICEVISION_INSTALL_DIRS}/include")
set(ALICEVISION_LIBRARY_DIRS "${ALICEVISION_INSTALL_DIRS}/lib")
include_directories(${ALICEVISION_INCLUDE_DIRS})
link_directories(${ALICEVISION_LIBRARY_DIRS})

#include Eigen3
include_directories("${ALICEVISION_INSTALL_DIRS}/include/eigen3")

# include OpenEXR
include_directories("${ALICEVISION_INSTALL_DIRS}/include/OpenEXR")

# include OpenCV
include_directories("${ALICEVISION_INSTALL_DIRS}/include/opencv4")

# # include tbb
# include_directories("${ALICEVISION_INSTALL_DIRS}/include/tbb")

#include GEOGRAM
set(GEOGRAM_INCLUDE_DIRS "${ALICEVISION_INCLUDE_DIRS}/geogram1")
include_directories("${GEOGRAM_INCLUDE_DIRS}")

# include MeshIO from Instant-Meshes with some modification
set(MESHIO_DIR "meshio")
include_directories("${MESHIO_DIR}")

# include Boost
set(BOOST_ROOT "${ALICEVISION_INSTALL_DIRS}")
find_package(Boost 1.70 REQUIRED)

# include RapidJson
set(RAPIDJSON_INSTALL_DIRS "${DEPENDENCIES_DIR}/rapidjson/install")
set(RAPIDJSON_INCLUDE_DIRS "${RAPIDJSON_INSTALL_DIRS}/include")
include_directories(${RAPIDJSON_INCLUDE_DIRS})

# include this project
file(GLOB SRC "*.cpp" "*.h" "*.hpp")

add_executable(converter ${SRC}
        ${MESHIO_DIR}/normal.h ${MESHIO_DIR}/normal.cpp
        ${MESHIO_DIR}/meshio.h ${MESHIO_DIR}/meshio.cpp
        ${MESHIO_DIR}/common.h
        ${MESHIO_DIR}/rply.c
        )

file(GLOB ALICEVISION_LIBS "${ALICEVISION_LIBRARY_DIRS}/lib*.so")
target_link_libraries(converter PUBLIC ${ALICEVISION_LIBS} OpenMP::OpenMP_CXX stdc++fs)

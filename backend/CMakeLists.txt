cmake_minimum_required(VERSION 3.7)

project(duduserver)

# set project framework
INCLUDE_DIRECTORIES(
  include
)
AUX_SOURCE_DIRECTORY(src DIR_SRCS)
SET(EXECUTABLE_OUTPUT_PATH ../bin)

set(CMAKE_CXX_STANDARD 11)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Boost COMPONENTS system filesystem REQUIRED)
find_package(Threads)
include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
find_package(Metis REQUIRED)
include_directories(${METIS_INCLUDE_DIR})

add_executable(${PROJECT_NAME} ${DIR_SRCS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} Threads::Threads ${METIS_LIBRARY})

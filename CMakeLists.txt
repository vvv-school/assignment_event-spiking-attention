# requires minimum cmake version
cmake_minimum_required(VERSION 3.5)

# produce the cmake var PROJECT_NAME
project(event-spiking-attention)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

# mandatory use of these packages
find_package(YARP 3.1.101 REQUIRED COMPONENTS OS)
find_package(ICUBcontrib REQUIRED)
find_package(eventdriven REQUIRED)

# extend the current search path used by cmake to load helpers
list(APPEND CMAKE_MODULE_PATH ${ICUBCONTRIB_MODULE_PATH})

# helpers defining certain macros
include(ICUBcontribHelpers)
include(ICUBcontribOptions)

# specify to install in $ICUBcontrib_DIR/bin
icubcontrib_set_default_prefix()

# includes  @ ICUBCONTIRB_INSTALL_PREFIX/include/
include_directories(${eventdriven_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/event-spiking-attention.h ${CMAKE_SOURCE_DIR}/src/event-spiking-attention.cpp)

# libraries @ ICUBCONTRIB_INSTALL_PREFIX/lib/libeventdriven.a
target_link_libraries(${PROJECT_NAME} ${YARP_LIBRARIES} eventdriven)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

icubcontrib_add_uninstall_target()

file(GLOB scripts ${CMAKE_SOURCE_DIR}/app/scripts/*.xml)
yarp_install(FILES ${scripts} DESTINATION ${ICUBCONTRIB_APPLICATIONS_INSTALL_DIR})


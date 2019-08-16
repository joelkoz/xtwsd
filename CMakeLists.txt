cmake_minimum_required(VERSION 2.8.9)
project(XTideService)
set (XTideService_VERSION_MAJOR 0)
set (XTideService_VERSION_MINOR 1)
set (CMAKE_CXX_STANDARD 11)

#Bring the headers into the project
include_directories(/usr/local/include)


set ( PROJECT_LINK_LIBS libtcd.a libxtide.a librestbed.a )
set ( TEST_LINK_LIBS libtcd.a libxtide.a)
 
#Gather all source files:
file(GLOB SOURCES "src/*.cpp")
 
add_executable(xtwsd ${SOURCES})
target_link_libraries(xtwsd ${PROJECT_LINK_LIBS} )

list(REMOVE_ITEM SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)
file (GLOB TESTSRC "tests/*.cpp")
list(APPEND TESTSRC ${SOURCES})
add_executable(test-xtwsd ${TESTSRC})
target_link_libraries(test-xtwsd ${TEST_LINK_LIBS} )

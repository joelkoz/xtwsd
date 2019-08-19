cmake_minimum_required(VERSION 2.8.9)
project(XTideService)
set (XTideService_VERSION_MAJOR 0)
set (XTideService_VERSION_MINOR 2)
set (CMAKE_CXX_STANDARD 11)

#Bring the headers into the project
include_directories(/usr/local/include)
include_directories(/usr/local/opt/openssl/include)

OPTION (BUILD_SERVER "Build xtwsd server" ON)
OPTION (BUILD_CLIENT "Build nos2xt client" ON)
OPTION (BUILD_TESTS "Build unit test suite" OFF)



IF (BUILD_SERVER)
  set ( SERVER_LINK_LIBS libtcd.a libxtide.a libserved.a libboost_system.a)
  file(GLOB SERVER_SOURCES "src/*.cpp")
  add_executable(xtwsd ${SERVER_SOURCES})
  target_link_libraries(xtwsd ${SERVER_LINK_LIBS} )
ENDIF (BUILD_SERVER)



IF (BUILD_CLIENT)
  set ( CLIENT_LINK_LIBS libboost_system.a libssl.a libcrypto.a)
  file(GLOB CLIENT_SOURCES "src/util/*.cpp")
  add_executable(nos2xt ${CLIENT_SOURCES})
  target_link_libraries(nos2xt ${CLIENT_LINK_LIBS} )
ENDIF (BUILD_CLIENT)



IF (BUILD_TESTS)
  set ( TEST_LINK_LIBS libtcd.a libxtide.a)
  file(GLOB TEST_SOURCES "src/*.cpp")
  list(REMOVE_ITEM TEST_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)
  file (GLOB TESTSRC "tests/*.cpp")
  list(APPEND TESTSRC ${TEST_SOURCES})
  add_executable(test-xtwsd ${TESTSRC})
  target_link_libraries(test-xtwsd ${TEST_LINK_LIBS} )
ENDIF (BUILD_TESTS)

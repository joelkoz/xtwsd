cmake_minimum_required(VERSION 3.2)
project(xtwsd)
set (xtwsd_VERSION_MAJOR 0)
set (xtwsd_VERSION_MINOR 2)
set (CMAKE_CXX_STANDARD 11)


OPTION (BUILD_SERVER "Build xtwsd server" ON)
OPTION (BUILD_CLIENT "Build nos2xt client" ON)
OPTION (BUILD_TESTS "Build unit test suite" OFF)

include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

# libtcd from XTide
ExternalProject_Add(project_libtcd
  URL https://flaterco.com/files/xtide/libtcd-2.2.7-r2.tar.bz2
  URL_MD5 5dbdc144c0e00886252ef880fc765d2e
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libtcd-2.2.7
  CONFIGURE_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/libtcd-2.2.7/src/project_libtcd/configure"
  BUILD_COMMAND make
  INSTALL_COMMAND ""
)

ExternalProject_Get_Property(project_libtcd SOURCE_DIR)

ExternalProject_Add_Step(project_libtcd configure 
    WORKING_DIRECTORY ${SOURCE_DIR}
)

ExternalProject_Add_Step(project_libtcd build
    WORKING_DIRECTORY ${SOURCE_DIR}
)

ExternalProject_Get_Property(project_libtcd BINARY_DIR)
add_library(libtcd STATIC IMPORTED)
set_property(TARGET libtcd PROPERTY IMPORTED_LOCATION ${BINARY_DIR}/.libs/libtcd.a)
INCLUDE_DIRECTORIES("${BINARY_DIR}")




# Dependencies for project_xtide
FIND_PACKAGE(PNG 1.4.12 REQUIRED)
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIRS})



# libxtide main project
ExternalProject_Add(project_xtide
  URL https://flaterco.com/files/xtide/xtide-2.15.2.tar.bz2
  URL_MD5 3d4bd4ef4152809deff9a3b48330fe9d
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/xtide-2.15.2
  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/xtide-2.15.2/src/project_xtide/configure --without-x
  BUILD_COMMAND make
  INSTALL_COMMAND ""
)

ExternalProject_Get_Property(project_xtide SOURCE_DIR)
INCLUDE_DIRECTORIES("${SOURCE_DIR}")

ExternalProject_Add_Step(project_xtide configure 
    WORKING_DIRECTORY ${SOURCE_DIR}
)

ExternalProject_Add_Step(project_xtide build
    WORKING_DIRECTORY ${SOURCE_DIR}
)

ExternalProject_Get_Property(project_xtide BINARY_DIR)
add_library(libxtide STATIC IMPORTED)
set_property(TARGET libxtide PROPERTY IMPORTED_LOCATION ${BINARY_DIR}/.libs/libxtide.a)
INCLUDE_DIRECTORIES("${BINARY_DIR}/libxtide")





# Boost
set(Boost_USE_STATIC_LIBS ON)
FIND_PACKAGE(Boost 1.70.0 COMPONENTS "system" REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})


# nlohmann_json
add_subdirectory(dependencies/nlohmann_json)
add_subdirectory(dependencies/fifo_map)
INCLUDE_DIRECTORIES(dependencies/fifo_map/fifo_map/src)

FIND_PACKAGE(Threads)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
message("Include dirs used in for compiling:")
foreach(dir ${dirs})
  message(STATUS "include_dir='${dir}'")
endforeach()

IF (BUILD_SERVER)
  SET(SERVED_BUILD_SHARED OFF CACHE BOOL "Don't build served shared library")
  SET(SERVED_BUILD_STATIC ON CACHE BOOL "Build served static library")
  SET(SERVED_BUILD_TESTS OFF CACHE BOOL "Don't build served tests")
  SET(SERVED_BUILD_EXAMPLES OFF CACHE BOOL "Don't build served examples")
  add_subdirectory(dependencies/served)
  INCLUDE_DIRECTORIES(dependencies/served/served/src)
  file(GLOB SERVER_SOURCES "src/*.cpp")
  add_executable(xtwsd ${SERVER_SOURCES})
  target_link_libraries(xtwsd libtcd libxtide served ${CMAKE_THREAD_LIBS_INIT} ${Boost_SYSTEM_LIBRARY} ${PNG_LIBRARIES} nlohmann_json::nlohmann_json)
  install(TARGETS xtwsd DESTINATION bin)
ENDIF (BUILD_SERVER)



IF (BUILD_CLIENT)
  # openssl
  FIND_PACKAGE(OpenSSL REQUIRED)
  INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})

  file(GLOB CLIENT_SOURCES "src/util/*.cpp")
  add_executable(nos2xt ${CLIENT_SOURCES})
  target_link_libraries(nos2xt ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${Boost_SYSTEM_LIBRARY} nlohmann_json::nlohmann_json)
  install(TARGETS nos2xt DESTINATION bin)
ENDIF (BUILD_CLIENT)



IF (BUILD_TESTS)
  set ( TEST_LINK_LIBS libtcd libxtide)
  file(GLOB TEST_SOURCES "src/*.cpp")
  list(REMOVE_ITEM TEST_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)
  file (GLOB TESTSRC "tests/*.cpp")
  list(APPEND TESTSRC ${TEST_SOURCES})
  add_executable(test-xtwsd ${TESTSRC})
  target_link_libraries(test-xtwsd ${TEST_LINK_LIBS} )
ENDIF (BUILD_TESTS)

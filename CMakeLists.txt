cmake_minimum_required(VERSION 3.7)
project(SimpleRenderEngine CXX)

if ("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(WARNING
            "Building from within the SimpleRenderEngine source tree is not recommended.\n"
            "Hint: Run these commands\n"
            "$ rm -rf CMakeCache.txt CMakeFiles\n"
            "$ mkdir -p ../sre_build\n"
            "$ cd ../sre_build\n"
            "$ cmake ${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

SET(CMAKE_PREFIX_PATH "/var/cache/pacman/pkg/mingw64/lib;/var/cache/pacman/pkg/ming64/lib/cmake/SDL2;/var/cache/pacman/pkg/mingw64/include/SDL2;/mingw64/lib/cmake/SDL2")

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

IF (MSVC)
    SET(SDL2_DIR "${CMAKE_BINARY_DIR}/sre_libs/SDL2-2.0.12/lib/x86" CACHE STRING "Directory path to SDL2.lib")
    SET(SDL2_LIBRARY "${CMAKE_BINARY_DIR}/sre_libs/SDL2-2.0.12/lib/x86/SDL2.lib" CACHE STRING "File path to SDL2.lib")
    SET(SDL2_INCLUDE_DIR "${CMAKE_BINARY_DIR}/sre_libs/SDL2-2.0.12/include" CACHE STRING "Directory path to SDL2 header files")
    SET(SDL2MAIN_LIBRARY "${CMAKE_BINARY_DIR}/sre_libs/SDL2-2.0.12/lib/x86/SDL2main.lib" CACHE STRING "File path to SDL2main.lib")
ENDIF (MSVC)

find_package(SDL2 REQUIRED)
set(SRE_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})

enable_testing()

#########################################################
# FIND OPENGL
#########################################################
SET(OpenGL_GL_PREFERENCE GLVND)
IF(APPLE)
    add_definitions(-DGL_SILENCE_DEPRECATION)
    list(APPEND SRE_INCLUDE_DIRS /System/Library/Frameworks)
    FIND_LIBRARY(OpenGL_LIBRARY OpenGL )
    MARK_AS_ADVANCED (OpenGL_LIBRARY)
    SET(EXTRA_LIBS ${OpenGL_LIBRARY})
ELSE (APPLE)
    # find_package(OpenGL REQUIRED GLOBAL) # GLOBAL supported in CMake 3.24+
    find_package(OpenGL REQUIRED)
    list(APPEND SRE_INCLUDE_DIRS ${OPENGL_INCLUDE_DIR})
    IF(MSVC)
        SET(GLEW_INCLUDE_DIR "${CMAKE_BINARY_DIR}/sre_libs/glew-2.1.0_source/include" CACHE STRING "Directory path to glew header files")
        SET(GLEW_LIBRARY "${CMAKE_BINARY_DIR}/sre_libs/glew-2.1.0/lib/Release/Win32/glew32s.lib" CACHE STRING "File path to glew32s.lib")
        SET(GLEW_SOURCE "${CMAKE_BINARY_DIR}/sre_libs/glew-2.1.0_source/src" CACHE STRING "Directory path to glew source files")
    ENDIF(MSVC)
    # set(GLEW_USE_STATIC_LIBS ON) # Set to link statically (also remove local outdated "FindGLEW.cmake", or IF(!MSVC)-out the contents)
    # find_package(GLEW REQUIRED GLOBAL) # GLOBAL export supported in CMake 3.24+
    find_package(GLEW REQUIRED)
    list(APPEND SRE_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
    # SET(EXTRA_LIBS OpenGL::GL GLEW::glew_s) # Set to link statically
    # SET(EXTRA_LIBS ${OPENGL_LIBRARY} ${GLEW_LIBRARY}) # Old usage
    SET(EXTRA_LIBS OpenGL::GL ${GLEW_LIBRARY})
ENDIF (APPLE)

IF (MSVC)
    SET(SDL2_IMAGE_LIBRARY "${CMAKE_BINARY_DIR}/sre_libs/SDL2_image-2.0.5/lib/x86/SDL2_image.lib" CACHE STRING "File path to SDL2_image.lib")
    SET(SDL2_IMAGE_INCLUDE_DIR "${CMAKE_BINARY_DIR}/sre_libs/SDL2_image-2.0.5/include" CACHE STRING "Directory path to SDL2 header files")
ENDIF(MSVC)
find_package(SDL2_image REQUIRED)
list(APPEND SRE_INCLUDE_DIRS ${SDL2_IMAGE_INCLUDE_DIRS})

option(USE_OPENVR "Enable OpenVR" OFF)

set(OPENVR_LIB)
IF (USE_OPENVR)
    find_package(OpenVR REQUIRED)
    list(APPEND SRE_INCLUDE_DIRS ${OPENVR_INCLUDE_DIRS})
	IF(UNIX)
		add_definitions(-DSRE_OPENVR)
	ELSE(UNIX)	
		add_definitions(/DSRE_OPENVR)
	ENDIF(UNIX)
    IF(APPLE)
        if(${OPENVR_PLATFORM} MATCHES "osx32")
            set(OPENVR_LIB "${OPENVR_ROOT_DIR}/lib/${OPENVR_PLATFORM}/libopenvr_api.dylib")
        else()
            set(OPENVR_LIB "${OPENVR_ROOT_DIR}/lib/${OPENVR_PLATFORM}/OpenVR.framework")
        endif()
        MESSAGE("${OPENVR_LIB}")
    ELSE(APPLE)
        set(OPENVR_LIB "${OPENVR_ROOT_DIR}/lib/${OPENVR_PLATFORM}/openvr_api.lib")
    ENDIF(APPLE)
ENDIF(USE_OPENVR)

add_subdirectory(submodules)
set(SRE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include ${EXTRA_INCLUDE} ${SRE_INCLUDE_DIRS} CACHE PATH "" FORCE)
include_directories(${SRE_INCLUDE})

add_subdirectory(src)
SET(USE_SRE_EXAMPLES ON CACHE BOOL "Enable examples for SRE")
if (USE_SRE_EXAMPLES)
    add_subdirectory(examples)
endif(USE_SRE_EXAMPLES)

SET(USE_SRE_TEST_AND_UTILS OFF CACHE BOOL "Enable Tests and Utils for SRE")
IF (USE_SRE_TEST_AND_UTILS)
    include(SreTest)
    add_subdirectory(submodules/googletest)
    add_subdirectory(unit_tests)
    add_subdirectory(regression_tests)
    add_subdirectory(utils)
ENDIF(USE_SRE_TEST_AND_UTILS)
set(SRE_LIBRARIES SRE ${EXTRA_LIBS} ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARIES} ${OPENVR_LIB} CACHE PATH "" FORCE)

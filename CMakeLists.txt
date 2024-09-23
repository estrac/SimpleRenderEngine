cmake_minimum_required(VERSION 3.7)
project(SimpleRenderEngine CXX)

# Setup CMake and CMake Utilities =============================================

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
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

if (NOT MSVC AND NOT XCODE)
    # The CMAKE_BUILD_TYPE variable is ignored by CMake for MSVC and XCODE
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type: Debug or Release")
endif ()

if (CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Semicolon separated list of supported configuration types" FORCE)
endif()

if (WIN32 AND NOT MSVC)
    set(CMAKE_CODEBLOCKS_EXECUTABLE "C:/'Program Files'/CodeBlocks/codeblocks.exe" CACHE FILEPATH "Location of codeblocks executable" FORCE)
endif ()

IF (MSVC)
    SET(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libraries" "${CMAKE_CURRENT_SOURCE_DIR}/libraries/glew-2.1.0" CACHE STRING "Set semi-colon-seperated list of directories where to search for SDL2 and GLEW libraries")
ELSE ()
    SET(CMAKE_PREFIX_PATH "/usr;/usr/bin;/usr/local;/sw;/opt/local;/opt/csw;/opt;/var/cache/pacman/pkg/mingw64/lib;/var/cache/pacman/pkg/ming64/lib/cmake/SDL2;/var/cache/pacman/pkg/mingw64/include/SDL2;/mingw64/lib/cmake/SDL2" CACHE STRING "Set semi-colon-seperated list of directories where to search for SDL2 and GLEW libraries")
ENDIF ()

# option(BUILD_SHARED_LIBS "Build with shared libraries" OFF) # This does not appear to have any effect in MSVC

# Below sets MSVC to use static MultiThreaded libraries (with or without Debug
# symbols, depending on the configuration). It is ignored for non-MSVC builds.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# TODO: https://alexreinking.com/blog states it is bad practice to modify these
# flags (also modified in SreUtils "build_sre_exe" and unit_tests/CMakeLists.txt)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLEW_STATIC" CACHE INTERNAL "")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # Note that CMAKE_CXX_FLAGS_RELEASE and CMAKE_CXX_FLAGS_DEBUG are
    # available to customize the Debug and Release configurations
    # TODO: Gradually ramp warnings up to /W3
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /W0 /wd4996 /EHsc" CACHE INTERNAL "")
endif ()

include(SreUtils)

# Find SDL ====================================================================
find_package(SDL2 REQUIRED)
if (WIN32)
    # TODO: Check if SDL2-static will work on Windows with home-built SDL2 lib
    #       If it does, then this if statement can be removed
    SET(SDL_LIBS SDL2::SDL2)
else ()
    SET(SDL_LIBS SDL2::SDL2-static)
endif ()

# Find OpenGL =================================================================
SET(OpenGL_GL_PREFERENCE GLVND)
IF (APPLE)
    add_definitions(-DGL_SILENCE_DEPRECATION)
    list(APPEND SRE_INCLUDE_DIRS /System/Library/Frameworks)
    FIND_LIBRARY(OpenGL_LIBRARY OpenGL )
    MARK_AS_ADVANCED (OpenGL_LIBRARY)
    SET(OPENGL_LIBS ${OpenGL_LIBRARY})
ELSE (APPLE)
    if (MSVC)
        set(GLEW_USE_STATIC_LIBS ON)
    endif (MSVC)
    find_package(OpenGL REQUIRED)
    # Unfortunately none of these lines work to link GLEW statically -- get
    # unresolved symbol glXGetProcAddressARB
    #set(GLEW_USE_STATIC_LIBS ON)
    #SET(OPENGL_LIBS OpenGL::OpenGL OpenGL::GLX GLEW::glew_s)
    #SET(OPENGL_LIBS OpenGL::OpenGL OpenGL::EGL GLEW::glew_s)
    #SET(OPENGL_LIBS OpenGL::GL GLEW::glew_s)
    find_package(GLEW REQUIRED)
    SET(OPENGL_LIBS OpenGL::GL GLEW::GLEW)
ENDIF (APPLE)

# Find OpenVR =================================================================
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

# Setup libraries =============================================================
set(SRE_LIBRARIES ${SDL_LIBS} ${OPENGL_LIBS} ${OPENVR_LIB} CACHE PATH "" FORCE)

# Build submodules ============================================================
add_subdirectory(submodules) # This subdirectory sets ${EXTRA_INCLUDE}

# Setup include directories ===================================================
set(SRE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include ${EXTRA_INCLUDE} ${SRE_INCLUDE_DIRS} CACHE PATH "" FORCE)
include_directories(${SRE_INCLUDE})

# Build SRE library ===========================================================
add_subdirectory(src)

# Build examples ==============================================================
option(USE_SRE_EXAMPLES "Enable examples for SRE" ON)
if (USE_SRE_EXAMPLES)
    add_subdirectory(examples)
endif (USE_SRE_EXAMPLES)

# Build tests =================================================================
option(USE_SRE_TEST_AND_UTILS "Enable Tests and Utils for SRE" OFF)
IF (USE_SRE_TEST_AND_UTILS)
    enable_testing()
    include(GoogleTest)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory(submodules/googletest)
    add_subdirectory(unit_tests)
    add_subdirectory(regression_tests)
    add_subdirectory(utils)
    if (MSVC)
        # Create target to run tests from Visual Studio
        add_custom_target(
            Run_Tests
            COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> --force-new-ctest-process 
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        )
    endif ()
ENDIF (USE_SRE_TEST_AND_UTILS)

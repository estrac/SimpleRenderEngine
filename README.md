[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/mortennobel/SimpleRenderEngine/master/LICENSE)
[![Build Status](https://travis-ci.org/mortennobel/SimpleRenderEngine.svg?branch=master)](https://travis-ci.org/mortennobel/SimpleRenderEngine)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/86403818b8b54161a6fef03248c0b828)](https://www.codacy.com/app/mortennobel/SimpleRenderEngine?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=mortennobel/SimpleRenderEngine&amp;utm_campaign=Badge_Grade)

# SimpleRenderEngine

This is an update to Morten Nobel-Jørgensen's SimpleRenderEngine (sre). The intent of the updates is to enhance the capabilities and to make the SRE interface easier to use (e.g. not as dependent on advanced C++11/14 features like Lambdas) and with less exposure of OpenGL internals (e.g. by abstracting away direct use of transformation matrices).

Two new cameras (derived from the basic virtual camera) have been added:
1) A basic "First-Person Surveyor (FPS)" (e.g. a Minecraft-like) camera (demonstrated in 'examples/18_FPS-camera.cpp')
2) A basic "Flight" camera (demonstrated in 'examples/19_Flight-camera.cpp')

The goal of SimpleRenderEngine is to provide easy way to get started with graphics programming in 2D or 3D without a deep knowledge about the low-level graphics APIs like OpenGL, DirectX and Vulkan.
 
SimpleRenderEngine currently depends on Simple Direct Layer 2.x (SDL2), SDL2-image, OpenGL Mathematics (GLM), and OpenGL 3.3 (or higher), Dear ImGui and runs on Windows, macOS, and Linux.
 
sre provides:
 * First-Person Surveyor (e.g. Minecraft-like) virtual camera
 * Flight virtual camera
 * Basic virtual camera (perspective and orthographic)
 * Texture support (JPEG, PNG)
 * Cube map support
 * Mesh support (with custom vertex attributes)
 * Shaders (PBR, Blinn-Phong, unlit, alpha blending, and custom shaders)
 * Enforces efficient use of OpenGL
 * Forward rendering
 * Full C++14 support
 * Support for 2D or 3D rendering
 * GUI rendering (using Dear ImGui)
 * Emscripten support (allows cross compiling to HTML 5 + WebGL)
 * VR support (OpenVR)
 * Bump mapping
 * Shadowmap

To keep sre as simple and flexible as possible the following features are not a part of sre:
 * Scenegraphs
 * Deferred rendering
 * Dynamic particle systems

## Build Instructions

### Ubuntu Linux

#### Update Ubuntu

Use the Ubuntu 'Software Updater' utility to ensure Ubuntu is fully up-to-date. Then take these steps:

 * sudo apt full-upgrade
 * sudo apt update
 * sudo apt autoremove

#### Install compilers and build tools (if not installed already)

 * sudo apt install g++
 * sudo apt install git
 * sudo apt install make
 * sudo apt install cmake
 * sudo apt install cmake-curses-gui (this is optional)

#### Install libraries (if not installed already)

 * sudo apt install libsdl2-dev
 * sudo apt install libsdl2-image-dev
 * sudo apt install libglew-dev

### Windows

#### Install MSYS2 and Tools
 * Go to the MSYS2 site and follow instructions to install MSYS2 and mingw64:

        https://www.msys2.org/

 * Open 'MSYS2 MinGW64 64-bit' command line from 'MSYS2 64bit' folder in the
   Windows Start Menu and then run:

        pacman -S --needed base-devel mingw-w64-x86_64-toolchain
        pacman -S mingw-w64-x86_64-glew  # To install GLEW (see "OpenGL Notes")
        pacman -S mingw-w64-x86_64-cmake # To install CMake
        pacman -S mingw-w64-x86_64-qt6   # To install CMake gui
        # Note that not updating qt leads to weird errors like "error while loading shared libs"

    * The cmake executables are:

        /mingw64/bin/cmake.exe
        /mingw64/bin/cmake-gui.exe

 * Install SDL2

        pacman -S mingw-w64-x86_64-SDL2
        pacman -S mingw-w64-x86_64-SDL2_image

 * Notes on using the MSYS2 package manager
    * To see which packages are needed to make a package work properly, use

        packman -Qi package_name [e.g. mingw-w64-x86_64-cmake]

    * To search for a package available in MSYS2, use the command

        pacman -Ss package_name

    * To search for an already-installed package, use the command

        pacman -Qs package_name [or part of a package name]

### Install sre
 
 * cd [directory-to-install-into] 
 * git clone --recurse-submodules https://github.com/estrac/SimpleRenderEngine.git sre (where 'sre' is the installation directory that will be created for sre). Note that if you have forked the repository you should replace 'estrac' with your username.
 * 'cd sre'
 * If you have forked the repository, run the following to stay in-sync:
 * 'git remote add upstream https://github.com/estrac/SimpleRenderEngine.git'
 * 'pwd' (Note: use this to get the full-path-of-install-directory used below)
 * 'git checkout'
 * 'mkdir -p ../sreBuild ('-p' flag gracefully handles a pre-existing directory)'
 * 'cd ../sreBuild'
 * 'cmake [full-path-of-install-directory]'
 * or for developers: 'cmake [install-dir] -DCMAKE_BUILD_TYPE=Debug -DINSTALL_GTEST=ON -DUSE_SRE_TEST_AND_UTILS:BOOL=ON'
 * 'make'
 * 'cd examples'
 * './SRE-Example-00_hello-engine'
 * Try other examples

## Examples
 
Examples (in the examples folder).

[![Matcap](https://mortennobel.github.io/SimpleRenderEngine/examples/07_matcap.png)](https://mortennobel.github.io/SimpleRenderEngine/examples/07_matcap.html)[![Picking](https://mortennobel.github.io/SimpleRenderEngine/examples/09_picking.png)](https://mortennobel.github.io/SimpleRenderEngine/examples/09_picking.html)[![Skybox](https://mortennobel.github.io/SimpleRenderEngine/examples/10_skybox-example.png)](https://mortennobel.github.io/SimpleRenderEngine/examples/10_skybox-example.html)[![Render to texture](https://mortennobel.github.io/SimpleRenderEngine/examples/12_render-to-texture.png)](https://mortennobel.github.io/SimpleRenderEngine/examples/12_render-to-texture.html)[![Cloth_Simulation](https://mortennobel.github.io/SimpleRenderEngine/examples/15_cloth_simulation.png)](https://mortennobel.github.io/SimpleRenderEngine/examples/15_cloth_simulation.html)[![Shadows](https://mortennobel.github.io/SimpleRenderEngine/examples/16_shadows.png)](https://mortennobel.github.io/SimpleRenderEngine/examples/16_shadows.html)[![GLSL Editor](https://mortennobel.github.io/SimpleRenderEngine/examples/glsl_editor.png)](https://github.com/mortennobel/sre_glsl_editor)[![Platformer](https://mortennobel.github.io/SimpleRenderEngine/examples/platformer.png)](https://github.com/mortennobel/SimpleRenderEngineProject/tree/master/project/platformer)[![Particle system](https://mortennobel.github.io/SimpleRenderEngine/examples/particle-system.png)](https://github.com/mortennobel/SimpleRenderEngineProject/tree/master/project/particle_system)[![ImGUI integration](https://mortennobel.github.io/SimpleRenderEngine/examples/gui.png)](https://github.com/mortennobel/SimpleRenderEngineProject/tree/master/project/gui)

## Documentation

API documentation is defined in header files, usage is shown in the example files.
 
## Other resources
 
 * https://www.libsdl.org Simple Direct Layer 2.x 
 * https://www.libsdl.org/projects/SDL_image/ Simple Direct Layer Image 2.x
 * http://glm.g-truc.net/ OpenGL Mathematics (bundled as submodule)
 * https://www.opengl.org/registry/ OpenGL Registry
 * https://github.com/ocornut/imgui ImGui 1.60 (submodule)
 * https://github.com/BalazsJako/ImGuiColorTextEdit ImGuiColorTextEdit (bundled as submodule)
 * https://github.com/kazuho/picojson PicoJSON (bundled as submodule)

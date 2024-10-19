## SRE Libraries Folder/Directory

This folder/directory contains binary libraries for SDL2 and GLEW for various platforms and situations.

The SDL and Glew directories without the "-EGL" label allow for static builds with Microsoft Visual Studio (MSVC).

The Glew directory with the "-EGL" exenstion is an OpenGL-EGL build of Glew needed to support the `SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland")` call to support wayland true fractional scaling on Linux (to use "Wayland" versus "XWayland").

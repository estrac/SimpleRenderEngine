SET(scr_files 00_hello-engine 01_hello-engine-raw 02_spheres 03_spinning-cube 04_spinning-primitives-tex 05_obj-viewer 06_custom-mesh-layout 07_matcap 08_gui 09_picking 10_skybox-example 11_sprite-example 12_render-to-texture  13_benchmark64k 14_spinning-primitives-openvr 15_cloth_simulation  16_shadows 17_deferred-shading 18_FPS-camera 19_Flight-camera)
#SET(scr_files 00_hello-engine) # For first example only
#SET(scr_files 18_FPS-camera 19_Flight-camera) # For two new camera examples only
#SET(scr_files 18_FPS-camera) # Custom

# Create custom build targets
FOREACH(scr_file ${scr_files})
    add_executable(SRE-Example-${scr_file} ${scr_file}.cpp)
    target_link_libraries(SRE-Example-${scr_file} SRE ${EXTRA_LIBS} ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARIES} ${OPENVR_LIB})
ENDFOREACH()

file(COPY ../examples_data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
IF (MSVC)
    # Create list of DLL files and copy to where execuables are
    SET(SDL2_dll "${CMAKE_BINARY_DIR}/sre_libs/SDL2-2.0.12/lib/x86/SDL2.dll" CACHE STRING "File path to SDL2.dll")
    SET(SDL2_image_lib_path "${CMAKE_BINARY_DIR}/sre_libs/SDL2_image-2.0.5/lib/x86" CACHE PATH "Directory path to SDL2_image.dll")
    SET(DLLFileList "${SDL2_image_lib_path}/SDL2_image.dll" "${SDL2_image_lib_path}/libjpeg-9.dll" "${SDL2_image_lib_path}/libpng16-16.dll" "${SDL2_image_lib_path}/libtiff-5.dll" "${SDL2_image_lib_path}/libwebp-7.dll" "${SDL2_image_lib_path}/zlib1.dll"  ${SDL2_dll})
    IF (USE_OPENVR)	
        SET(DLLFileList "${DLLFileList}" "${OPENVR_ROOT_DIR}/bin/${OPENVR_PLATFORM}/openvr_api.dll")
    ENDIF (USE_OPENVR)
    file(COPY ${DLLFileList} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
ENDIF(MSVC)

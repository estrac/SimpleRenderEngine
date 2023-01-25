include(SreUtils)
SET(src_files 00_hello-engine 01_hello-engine-raw 02_spheres 03_spinning-cube 04_spinning-primitives-tex 05_obj-viewer 06_custom-mesh-layout 07_matcap 08_gui 09_picking 10_skybox-example 11_sprite-example 12_render-to-texture  13_benchmark64k 14_spinning-primitives-openvr 15_cloth_simulation  16_shadows 17_deferred-shading 18_FPS-camera 19_Flight-camera)
#SET(src_files 00_hello-engine) # For first example only
#SET(src_files 18_FPS-camera 19_Flight-camera) # For two new camera examples only
#SET(src_files 18_FPS-camera) # Custom

# Create custom build targets
FOREACH(src_file ${src_files})
    build_sre_exe(${src_file})
ENDFOREACH()

file(COPY ../examples_data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

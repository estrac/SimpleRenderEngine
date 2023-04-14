set(test_name "imgui-demo-test")
set(test_width "800")
set(test_height "600")
set(pixel_threshold "0.0")
set(pixel_tolerance "0")
set(save_diff_images TRUE)

build_sre_exe(${test_name})
add_sre_test(${test_name} ${test_width} ${test_height} ${pixel_threshold} ${pixel_tolerance} ${save_diff_images})

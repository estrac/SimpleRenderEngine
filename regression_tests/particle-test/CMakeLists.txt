set(test_name "particle-test")
set(test_width "800")
set(test_height "600")
set(pixel_error_tolerance "0.0")
set(percent_error_allowed "0.0")
set(save_diff_images TRUE)

build_sre_exe(${test_name})
add_sre_test(${test_name} ${pixel_error_tolerance} ${percent_error_allowed} ${save_diff_images} ${test_width} ${test_height})

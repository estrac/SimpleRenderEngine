# Add image comparison tests to ${test_name} ====================================
# This funcion assumes:
#   - the "gold results" to compare to are located in directory 'gold_results'
#   - the "gold results" files have the same names as the test files
#   - the image comparison software is ${PROJECT_BINARY_DIR}/bin/imgcmp
#   - PNG images are being compared
function(add_image_tests test_name tolerance percent_error save_diff_images)
    set(gold_results_dir_name "gold_results") 
    set(dir ${CMAKE_CURRENT_BINARY_DIR})
    file(GLOB image_files_list "${gold_results_dir_name}/*.png")
    foreach(image_file_path ${image_files_list})
        get_filename_component(image_filename ${image_file_path} NAME)
        get_filename_component(image_name ${image_file_path} NAME_WLE)
        set(sub_test_name ${test_name}_${image_name})
        if (save_diff_images)
            set(diff_file_string "-o" "diff_${image_filename}")
        else ()
            set(diff_file_string "")
        endif ()
        add_test(NAME regression:${sub_test_name}
                 COMMAND ${PROJECT_BINARY_DIR}/bin/imgcmp -v ${diff_file_string} -t ${tolerance} -e ${percent_error}% ${dir}/${image_filename} ${dir}/${gold_results_dir_name}/${image_filename}
                 )
        set_tests_properties(regression:${sub_test_name}
                             PROPERTIES FIXTURES_REQUIRED ${test_name}
                             )
    endforeach()
endfunction()

# Add an SRE test in the current directory ======================================
# This funcion assumes:
#   - the user interface events file is called 'test.ui_events'
#   - the test's executable name is ${test_name}
function(add_sre_test test_name width height tolerance percent_error save_diff_images)
    set(dir ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY . DESTINATION ${dir} PATTERN "${test_name}.cpp" EXCLUDE)
    if (EXISTS "${dir}/test.ui_events")
        add_test(NAME regression:${test_name}
                 COMMAND ${test_name} -p test.ui_events -c -x ${width} -y ${height}
                 )
        set_tests_properties(regression:${test_name}
                             PROPERTIES FIXTURES_SETUP ${test_name}
                             )
    else ()
        add_test(NAME interactive:${test_name} COMMAND ${test_name} -x ${width} -y ${height})
    endif ()
    add_image_tests(${test_name} ${tolerance} ${percent_error} ${save_diff_images})
endfunction()

# Build a test executable # TODO: Rename this to build_sre_exe
# This function assumes:
#   - the name of the source file is ${test_name}.cpp
function(build_sre_test test_name)
    add_executable(${test_name} ${test_name}.cpp)
    if (MSVC)
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:LIBCMT /ignore:4099" PARENT_SCOPE)
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:LIBCMT" PARENT_SCOPE)
        set(EXTRA_LIBRARIES "winmm" "setupapi" "version")
    endif ()
    target_link_libraries(${test_name} SRE ${EXTRA_LIBRARIES})
endfunction()

# Call "add_subdirectory(...) on all subdirectories in the current directory
function(add_all_subdirectories)
    file(GLOB list_of_directory_items "*")
    foreach(item_path ${list_of_directory_items})
        if (IS_DIRECTORY ${item_path})
            get_filename_component(item_name ${item_path} NAME)
            add_subdirectory(${item_name})
        endif ()
    endforeach()
endfunction()

# Add image comparison tests to ${test_name} ====================================
# This funcion assumes:
#   - the "gold results" to compare to are located in directory 'gold_results'
#   - the "gold results" files have the same names as the test files
#   - the image comparison software is ${PROJECT_BINARY_DIR}/bin/imgcmp
#   - PNG images are being compared
function(add_image_tests test_name test_type working_dir threshold tolerance save_diff_images)
    set(gold_results_dir_name "gold_results") 
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
        add_test(NAME ${test_type}:${sub_test_name}
                 COMMAND ${PROJECT_BINARY_DIR}/bin/imgcmp -v ${diff_file_string} -t ${threshold} -e ${tolerance} ${image_filename} ${gold_results_dir_name}/${image_filename}
                 WORKING_DIRECTORY ${working_dir}
                 )
        set_tests_properties(${test_type}:${sub_test_name}
                            PROPERTIES FIXTURES_REQUIRED ${test_type}:${test_name}
                            )
    endforeach()
endfunction()

# Add an SRE test in the current directory ======================================
# This funcion assumes:
#   - the user interface events file is called 'test.ui_events'
#   - the test's executable name is ${test_name}
function(add_sre_test test_name width height threshold tolerance save_diff_images)
    set(test_type "regression")
    set(working_dir ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY . DESTINATION ${working_dir} PATTERN "${test_name}.cpp" EXCLUDE)
    if (EXISTS "${working_dir}/test.ui_events")
        add_test(NAME regression:${test_name}
                 COMMAND ${test_name}
                 WORKING_DIRECTORY ${working_dir}
                 )
        set_tests_properties(regression:${test_name}
                             PROPERTIES FIXTURES_SETUP ${test_type}:${test_name}
                             )
    else ()
        add_test(NAME interactive:${test_name}
                 COMMAND ${test_name}
                 WORKING_DIRECTORY ${working_dir}
                 )
    endif ()
    add_image_tests(${test_name} ${test_type} ${working_dir} ${threshold} ${tolerance} ${save_diff_images})
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

# Build an executable using the SRE library from a single cpp file
# This function assumes:
#   - the name of the single source file is ${exe_name}.cpp
function(build_sre_exe exe_name)
    add_executable(${exe_name} ${exe_name}.cpp)
    if (MSVC)
        # MSVC moves the .exe into a subdirectory -- move it to the root build directory,
        # where all the needed files are located. Specify Release/Debug-specific linker flags
        # Both of these are fragile (they break for other $<CONFIG> names)
        # TODO: Investigate using the INSTALL feature to accomplish both of these more robustly
        set_target_properties(${exe_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR})
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:LIBCMT /ignore:4099" PARENT_SCOPE)
        set_target_properties(${exe_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR})
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:LIBCMT" PARENT_SCOPE)
        set(EXTRA_LIBRARIES "winmm" "setupapi" "version")
    endif ()
    target_link_libraries(${exe_name} SRE ${EXTRA_LIBRARIES})
endfunction()

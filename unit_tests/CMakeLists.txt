# List of single-file tests

include_directories(SYSTEM ${gtest_SOURCE_DIR} ${gtest_SOURCE_DIR}/include)

file(GLOB_RECURSE test_cases *.cpp)
foreach(case_file ${test_cases})
    get_filename_component(case_name ${case_file} NAME_WE)
    set (case_name test_${case_name})
    add_executable(${case_name} ${case_file})
    if (MSVC)
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:LIBCMT /ignore:4099")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:LIBCMT")
        set(EXTRA_LIBRARIES "winmm" "setupapi" "version")
    endif ()
    target_link_libraries(${case_name} 
                          SRE ${EXTRA_LIBRARIES}
                          gtest_main
                          )
    add_test(NAME unit:${case_name}
             WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/unit_tests/
             COMMAND ${case_name}
             )
endforeach()

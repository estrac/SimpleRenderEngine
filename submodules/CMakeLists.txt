add_subdirectory(imgcmp)

set(EXTRA_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiColorTextEdit ${CMAKE_CURRENT_SOURCE_DIR}/imgui ${CMAKE_CURRENT_SOURCE_DIR}/glm ${CMAKE_CURRENT_SOURCE_DIR}/picojson ${CMAKE_CURRENT_SOURCE_DIR}/imgcmp/vendor/utils ${CMAKE_CURRENT_SOURCE_DIR}/imgui-filebrowser CACHE PATH "" FORCE)

file(GLOB EXTRA_SOURCE_FILES_
    "ImGuiColorTextEdit/TextEditor.cpp"
    "imgui/imgui.cpp"
    "imgui/imgui_draw.cpp"
    "imgui/imgui_widgets.cpp"
    "imgui/imgui_tables.cpp"
)

if (NOT EXTRA_SOURCE_FILES_)
    message( FATAL_ERROR "Cannot find submodules used in SimpleRenderEngine. Run 'git submodule update --init  --recursive' or checkout full project using 'git clone --recurse-submodules <url>'")
endif()

set(EXTRA_SOURCE_FILES ${EXTRA_SOURCE_FILES_} CACHE PATH "" FORCE)

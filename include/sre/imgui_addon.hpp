#ifndef IMGUI_ADDON_H_
#define IMGUI_ADDON_H_

#include <iostream>
#include <imgui.h>
#include <imgui_internal.h>


namespace ImGui {

//====================== ImGui Add-on Functions ================================

extern bool ShowMessage(const std::string& message,
                        const std::string& title = "Error",
                        // Last two arguments can be used to create a modal
                        // "process dialog" without buttons by passing showOk =
                        // false, and passing a "show" bool to close it 
                        const bool& showOk = true,
                        bool* show = nullptr);

extern void ToggleButton(const char* str_id, bool* selected, ImVec2 size);


// It would be good to commit the revised ImGui::RadioButton function below to
// the Dear ImGui repository.  If so, remove the capital 'T' from name (it
// should replace the shortcut that is there).

IMGUI_API template<typename T> bool RadioButtonT(const char* label, T* v,  T      v_button); // shortcut to handle above pattern when value is an arbtrary type T

// Template Function Definitions
template<typename T> bool RadioButtonT(const char* label, T* v, T v_button)
{
    const bool pressed = RadioButton(label, *v == v_button);
    if (pressed)
        *v = v_button;
    return pressed;
}

} // namespace ImGui

#endif // IMGUI_ADDON_H_

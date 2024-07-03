#ifndef IMGUI_ADDON_H_
#define IMGUI_ADDON_H_

#include <string_view>
#include <imgui.h>
#include <imgui_internal.h>
#include <glm/glm.hpp>

namespace sre {class Texture;}

namespace ImGui {

//========= ImGui functions for DPI scaling support ============================
// Transform coordinates that are scaled by the font size to pixel coordinates
ImVec2 GetFontDimensions();
ImVec2 ScaleByFont(const ImVec2& fontScaledCoord);
float ScaleByFontHeight(const float& fontScaledYCoord);
float ScaleByFontWidth(const float& fontScaledXCoord);
// Transform pixel coordinates to coordinates that are scaled by the font size
ImVec2 GetFontScale(const ImVec2& pixelCoord);

//========= ImGui Add-on Types =================================================
enum MouseButton {MouseButton_Left, MouseButton_Right, MouseButton_Middle};
enum YesNoButton {YesButton, NoButton, YesNoNotAnswered};

//========= ImGui Add-on Functions =============================================
void OpenPopup(std::string_view title); // Call this once before PopupModal
bool PopupModal(std::string_view name, std::string_view message,
                // Last two arguments can be used to create a modal "process
                // dialog" without buttons by passing showOk = false, and
                // passing a "show" bool to close it 
                            const bool& showOk = true, const bool& show = true);
void PopupModalWide(std::string_view name, std::string_view message);
ImGui::YesNoButton PopupYesNoModal(std::string_view name,
                                                     std::string_view question);
bool IsAnyPopupModalActive();
void TextCentered(std::string_view text); // Display centered text in ImGui
// Toggle button with border equal to ImGui::GetStyle().SeparatorTextBorderSize;
bool ToggleButton(std::string_view str_id, bool* selected, ImVec2 size = {0,0});

// It would be good to commit the revised ImGui::RadioButton function below to
// the Dear ImGui repository.  If so, remove the capital 'T' from name (it
// should replace the shortcut that is there) and remove the corresponding code
// in imgui_widgets.cpp, where bool ImGui::RadioButton is defined (this function
// will be superceded by the template definition and obviate its need).
IMGUI_API template<typename T> bool RadioButtonT(const char* label, T* v,  T      v_button); // shortcut to handle above pattern when value is an arbtrary type T

// Template Function Definitions
template<typename T> bool RadioButtonT(const char* label, T* v, T v_button)
{
    const bool pressed = RadioButton(label, *v == v_button);
    if (pressed)
        *v = v_button;
    return pressed;
}

// Helper functions
void RenderTexture(sre::Texture* texture, glm::vec2 size, const glm::vec2& uv0 = glm::vec2(0,0), const glm::vec2& uv1 = glm::vec2(1,1), const glm::vec4& tint_col = glm::vec4(1,1,1,1), const glm::vec4& border_col = glm::vec4(0,0,0,0));
extern char* AllocateString(const char* s);
extern char* AllocateString(const size_t length);

} // namespace ImGui

#endif // IMGUI_ADDON_H_

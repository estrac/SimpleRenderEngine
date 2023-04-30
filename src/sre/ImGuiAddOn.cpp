#include <string.h>
#include <imgui.h>
#include <sre/ImGuiAddOn.hpp>
#include <sre/Texture.hpp>

namespace ImGui {

//=========================== ImGui Functions ==================================

// This function initializes a modal popup to be shown. It should only be
// called once for each modal window shown. It must be called from within code
// that can render ImGui
void
OpenPopup(std::string_view name) {
    ImGui::OpenPopup(name.data());
}

// This function shows a message and returns true if acknowledged
// It must be called from within code that can render ImGui
// Before calling this function, OpenPopup(name.data()) should be called once
bool
PopupModal(std::string_view name, std::string_view message, 
           // Last two arguments are for a modal "process dialog" without
           // buttons, which needs a bool to be closed.
           // TODO: A seperate function for a popup without an OK button should
           // be made -- it would keep things cleaner
           const bool& showOk, const bool& show)
{
    bool acknowledged = false;
    // Always center this window when appearing
    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f,
                                           ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(name.data(), NULL,
                                           ImGuiWindowFlags_AlwaysAutoResize)) {
        // Dialog is only created if OpenPopup(name.data()) was called once
        int height = ImGui::GetFrameHeight();
        if (!showOk) ImGui::Dummy(ImVec2(height, height));
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 365);
        ImGui::Text("%s", message.data());
        if (showOk) {
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                acknowledged = true;
                ImGui::CloseCurrentPopup();
            }
        } else {
            ImGui::Dummy(ImVec2(height, height));
            if (!show) {
                acknowledged = true;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
    return acknowledged;
}

void
ToggleButton(std::string_view str_id, bool* selected, ImVec2 size)
{
    // Initialize and store variables used
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGuiStyle& imGuiStyle = ImGui::GetStyle();
    ImVec4* colors = imGuiStyle.Colors;
    float prevFrameRounding = imGuiStyle.FrameRounding;
    imGuiStyle.FrameRounding = 0.0;
    float prevFrameBorderSize = imGuiStyle.FrameBorderSize;
    imGuiStyle.FrameBorderSize = 1.0;
    ImVec2 pIn = ImGui::GetCursorScreenPos();
    ImVec2 p = pIn;
    if (size.y == 0) {
        size.y = ImGui::GetFrameHeight();
    }
    float thick  = 0.1f*size.y;
    float width = size.x + 2.0f*thick;
    float height = size.y + 2.0f*thick;

    // Add rectangle border on top of button
    draw_list->AddRectFilled(ImVec2(p.x, p.y),
                ImVec2(p.x + width, p.y + thick),
                ImGui::GetColorU32(*selected ? colors[ImGuiCol_ButtonActive]
                : colors[ImGuiCol_Button]), ImGui::GetStyle().FrameRounding);
    // Add rectangle border on bottom of button
    draw_list->AddRectFilled(ImVec2(p.x, p.y + height),
                ImVec2(p.x + width, p.y + height - thick),
                ImGui::GetColorU32(*selected ? colors[ImGuiCol_ButtonActive]
                : colors[ImGuiCol_Button]), ImGui::GetStyle().FrameRounding);
    // Add rectangle border on left side of button
    draw_list->AddRectFilled(ImVec2(p.x, p.y + thick),
                ImVec2(p.x + thick, p.y + height - thick),
                ImGui::GetColorU32(*selected ? colors[ImGuiCol_ButtonActive]
                : colors[ImGuiCol_Button]), ImGui::GetStyle().FrameRounding);
    // Add rectangle border on right side of button
    draw_list->AddRectFilled(ImVec2(p.x + width - thick, p.y + thick),
                ImVec2(p.x + width, p.y + height - thick),
                ImGui::GetColorU32(*selected ? colors[ImGuiCol_ButtonActive]
                : colors[ImGuiCol_Button]), ImGui::GetStyle().FrameRounding);

    // Add button to the center of the border
    p = {p.x + thick, p.y + thick};
    ImGui::SetCursorScreenPos(p);
    if (ImGui::Button(str_id.data(), ImVec2(size.x, size.y))) {
        *selected = !*selected;
    }
    // Return style properties to their previous values
    imGuiStyle.FrameBorderSize = prevFrameBorderSize;
    imGuiStyle.FrameRounding = prevFrameRounding;

    // Advance ImGui cursor according to actual size of full toggle button
    ImGui::SetCursorScreenPos(pIn);
    ImGui::Dummy(ImVec2(width, height));
}

void
RenderTexture(sre::Texture* texture,glm::vec2 size, const  glm::vec2& uv0, const glm::vec2& uv1, const glm::vec4& tint_col, const glm::vec4& border_col)
{
    ImGui::Image(reinterpret_cast<ImTextureID>(texture->textureId), ImVec2(size.  x, size.y),{uv0.x,uv0.y},{uv1.x,uv1.y},{tint_col.x,tint_col.y,tint_col.z,tint_col.w},{border_col.x,border_col.y,border_col.z,border_col.w});
}

} // namespace ImGui

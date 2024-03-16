#include <string.h>
#include <imgui.h>
#include <sre/ImGuiAddOn.hpp>
#include <sre/Texture.hpp>

static const float ImGui_Default_Font_Size = 13.0f; // ImGui's "ProggyClean" font

// Return the screen dots-per-inch (DPI) scaling factor
float
DpiScaling()
{
    IM_ASSERT(GImGui != NULL);
    return ImGui::GetFontSize() / ImGui_Default_Font_Size;
}

// Scale x and y by screen dots-per-inch (DPI) scaling factor
ImVec2
DpiVec2(float x, float y)
{
    IM_ASSERT(GImGui != NULL);
    float dpiScaling = DpiScaling();
    return ImVec2(dpiScaling*x, dpiScaling*y);
}


namespace ImGui {

//=========================== ImGui Functions ==================================

// Initialize a modal popup to be shown. It should only be called once for each
// modal window shown. It must be called from within code that can render ImGui
void
OpenPopup(std::string_view name)
{
    ImGui::OpenPopup(name.data());
}

// Show a message and returns true if acknowledged. Must be called from within
// code that can render ImGui. OpenPopup(name.data()) should be called once
// before calling this function.
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

// Show a message asking a question with a "yes" and a "no" button and returns
// an ImGui::YesNoButton enumerated type. It must be called from within code
// that can render ImGui. Before calling this function, OpenPopup(name.data())
// should be called once
ImGui::YesNoButton
PopupYesNoModal(std::string_view name, std::string_view question)
{ 
    ImGui::YesNoButton yesNoStatus = ImGui::YesNoNotAnswered;
    // Always center this window when appearing
    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f,
                                           ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(name.data(), NULL,
                                           ImGuiWindowFlags_AlwaysAutoResize)) {
        // Dialog is only created if OpenPopup(name.data()) was called once
        int height = ImGui::GetFrameHeight();
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 365);
        ImGui::Text("%s", question.data());
        if (ImGui::Button("Yes", ImVec2(60, 0))) {
            yesNoStatus = ImGui::YesButton;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(60, 0))) {
            yesNoStatus = ImGui::NoButton;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return yesNoStatus;
}

void
TextCentered(std::string_view text)
{
    auto windowWidth = ImGui::GetWindowSize().x;
    auto textWidth   = ImGui::CalcTextSize(text.data()).x;
    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::Text("%s", text.data());
}

void
ToggleButton(std::string_view str_id, bool* selected, ImVec2 size)
{
    // Initialize and store variables used
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    float prevFrameRounding = style.FrameRounding;
    style.FrameRounding = 0.0;
    float prevFrameBorderSize = style.FrameBorderSize;
    style.FrameBorderSize = 1.0;
    ImVec4 prevButtonCol = style.Colors[ImGuiCol_Button];
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
    if (*selected) {
        // Slightly darken a selected button
        style.Colors[ImGuiCol_Button] = ImColor(prevButtonCol.x, prevButtonCol.y,
                                          prevButtonCol.z, prevButtonCol.w-0.1f);
    }
    p = {p.x + thick, p.y + thick};
    ImGui::SetCursorScreenPos(p);
    if (ImGui::Button(str_id.data(), ImVec2(size.x, size.y))) {
        *selected = !*selected;
    }
    // Return style properties to their previous values
    style.FrameBorderSize = prevFrameBorderSize;
    style.FrameRounding = prevFrameRounding;
    style.Colors[ImGuiCol_Button] = prevButtonCol;

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

#include <string.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <sre/ImGuiAddOn.hpp>
#include <sre/Texture.hpp>

//=========================== ImGui Functions ==================================
namespace ImGui {

ImVec2
GetFontDimensions()
{
    IM_ASSERT(GImGui != NULL);
    ImVec2 fontDims = ImGui::CalcTextSize("EEEE");
    fontDims.x /= 4.0f; // Only divide the width by four (number of characters)
    return fontDims;
    // I ran the test below. ImGui::CalcTextSize returns smaller average char
    // width with more characters. 1-3 characters returns a width of exactly 7,
    // 4 characters returns a value of 6.75, and 100 characters returns a value
    // of 6.73 (all for Hack-Regular, size 13)
    //
    //ImVec2 fontDims = ImGui::CalcTextSize("eeeeeeeeeeee");
    //fontDims.x /= 12.0f;
    //std::cout << "fontDims4 = " << fontDims.x << ", " << fontDims.y
    //          << std::endl;
}

// Transform coordinates that are scaled by the font size to pixel coordinates
ImVec2
ScaleByFont(const ImVec2& fontScaledCoord)
{
    ImVec2 fontDims = GetFontDimensions();
    // Uncomment below to print out font width and height when program starts
    //
    //static bool firstFunctionCall = true;
    //if (firstFunctionCall) {
    //    std::cout << "fontDims = " << fontDims.x << ", " << fontDims.y
    //              << std::endl;
    //    firstFunctionCall = false;
    //}
    // ImGui works with pixels, so round to integers. If ImGui changes, revisit
    return ImVec2(round(fontDims.x * fontScaledCoord.x),
                  round(fontDims.y * fontScaledCoord.y));
}

float
ScaleByFontHeight(const float& fontScaledYCoord)
{
    return ScaleByFont({0.0, fontScaledYCoord}).y;
}

float ScaleByFontWidth(const float& fontScaledXCoord)
{
    return ScaleByFont({fontScaledXCoord, 0.0}).x;
}

// Transform pixel coordinates to coordinates that are scaled by the font size
ImVec2
GetFontScale(const ImVec2& pixelCoord)
{
    ImVec2 fontDims = GetFontDimensions();
    return ImVec2(pixelCoord.x/fontDims.x, pixelCoord.y/fontDims.y);
}

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
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ScaleByFontWidth(54));
        ImGui::Text("%s", message.data());
        if (showOk) {
            if (ImGui::Button("OK", ScaleByFont({17.0, 0}))) {
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
        ImGui::SetWindowFocus();
        ImGui::EndPopup();
    }
    return acknowledged;
}

void
PopupModalWide(std::string_view name, std::string_view message)
{
    // Always center this window when appearing
    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f,
                                           ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(name.data(), NULL,
                                           ImGuiWindowFlags_AlwaysAutoResize)) {
        // Dialog is only created if OpenPopup(name.data()) was called once
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ScaleByFontWidth(80));
        ImGui::Text("%s", message.data());
        if (ImGui::Button("OK", ScaleByFont({17.0, 0}))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetWindowFocus();
        ImGui::EndPopup();
    }
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
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ScaleByFontWidth(54));
        ImGui::Text("%s", question.data());
        if (ImGui::Button("Yes", ScaleByFont({8.58, 0.0}))) {
            yesNoStatus = ImGui::YesButton;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ScaleByFont({8.56, 0.0}))) {
            yesNoStatus = ImGui::NoButton;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetWindowFocus();
        ImGui::EndPopup();
    }
    return yesNoStatus;
}

bool
IsAnyPopupModalActive()
{
    // The following function is from imgui_internal
    return (ImGui::GetTopMostPopupModal() != NULL);
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
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, ScaleByFontWidth(0.15));
    ImVec2 pIn = ImGui::GetCursorScreenPos();
    ImVec2 p = pIn;
    if (size.y == 0) size.y = ImGui::GetFrameHeight();
    // ImGui works with pixels, so round to integers. If ImGui changes, revisit
    int thick  = round(0.1f*size.y);
    int width = round(size.x) + 2*thick;
    int height = round(size.y) + 2*thick;

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

    // If the button has been selected then darken it
    bool pushedStyleColor = false;
    if (*selected) {
        ImVec4 pale = colors[ImGuiCol_Button];
        ImVec4 darkened = ImColor(pale.x, pale.y, pale.z, pale.w - 0.4f);
        ImGui::PushStyleColor(ImGuiCol_Button, darkened); 
        pushedStyleColor = true;
    }
    // Add the (slightly-darkened if selected) button to the center of the border
    p = {p.x + thick, p.y + thick};
    ImGui::SetCursorScreenPos(p);
    if (ImGui::Button(str_id.data(), ImVec2(size.x, size.y))) {
        *selected = !*selected;
    }

    // Return style properties to their previous values
    ImGui::PopStyleVar(2); // FrameRounding and FrameBorderSize
    if (pushedStyleColor) ImGui::PopStyleColor();

    // Advance ImGui cursor according to actual size of full toggle button
    ImGui::SetCursorScreenPos(pIn);
    ImGui::Dummy(ImVec2(width, height));
}

void
RenderTexture(sre::Texture* texture,glm::vec2 size, const  glm::vec2& uv0, const glm::vec2& uv1, const glm::vec4& tint_col, const glm::vec4& border_col)
{
    ImGui::Image(reinterpret_cast<ImTextureID>(texture->textureId), ImVec2(size.  x, size.y),{uv0.x,uv0.y},{uv1.x,uv1.y},{tint_col.x,tint_col.y,tint_col.z,tint_col.w},{border_col.x,border_col.y,border_col.z,border_col.w});
}

char*
AllocateString(const char* s)
{
    if (s == nullptr) return nullptr;

    size_t len = strlen(s) + 1;
    void* buf = malloc(len);
    IM_ASSERT(buf);
    // C std memcpy copies len characters from s to buf.
    return (char*)memcpy(buf, (const void*)s, len);
    /*  Below is a more intuitive way to write the same code...
    char* newString = (char*)malloc(sizeof(char)*len);
    strcpy(newString, s);
    return newString;*/
}

char*
AllocateString(const size_t length)
{
    if (length <= 0) return nullptr;

    void* buf = malloc(length);
    IM_ASSERT(buf);
    return (char*)buf;
}

} // namespace ImGui

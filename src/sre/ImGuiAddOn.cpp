#include <string.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "sre/ImGuiAddOn.hpp"
#include "sre/Texture.hpp"

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

bool
ToggleButton(std::string_view str_id, bool* selected, ImVec2 size)
{
    // size represents a "standard" button plus an enclosing border of thickness
    // ImGui::GetStyle().SeparatorTextBorderSize

    // Initialize and store variables used
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, ScaleByFontWidth(0.15));
    ImVec2 pIn = ImGui::GetCursorScreenPos();
    ImVec2 p = pIn;
    // ImGui works with pixels, so round to integers. If ImGui changes, revisit
    size = {round(size.x), round(size.y)};
    // ImGui style SeparatorTextBorderSize is the preset size that looks best
    float border = style.SeparatorTextBorderSize;
    if (size.x == 0 && size.y == 0) { // Caller did not specify size, use default
        size = ImGui::CalcTextSize(str_id.data()) + style.FramePadding*2
               + ImVec2(border, border)*2;
    }
    ImVec2 innerSize = size - ImVec2(border, border)*2; // Button inside of border

    //colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

    // Add rectangle border on top of button
    draw_list->AddRectFilled(ImVec2(p.x, p.y),
                ImVec2(p.x + size.x, p.y + border),
                ImGui::GetColorU32(*selected ? colors[ImGuiCol_Border]
                : colors[ImGuiCol_Button]), ImGui::GetStyle().FrameRounding);
    // Add rectangle border on bottom of button
    draw_list->AddRectFilled(ImVec2(p.x, p.y + size.y),
                ImVec2(p.x + size.x, p.y + size.y - border),
                ImGui::GetColorU32(*selected ? colors[ImGuiCol_Border]
                : colors[ImGuiCol_Button]), ImGui::GetStyle().FrameRounding);
    // Add rectangle border on left side of button
    draw_list->AddRectFilled(ImVec2(p.x, p.y + border),
                ImVec2(p.x + border, p.y + size.y - border),
                ImGui::GetColorU32(*selected ? colors[ImGuiCol_Border]
                : colors[ImGuiCol_Button]), ImGui::GetStyle().FrameRounding);
    // Add rectangle border on right side of button
    draw_list->AddRectFilled(ImVec2(p.x + size.x - border, p.y + border),
                ImVec2(p.x + size.x, p.y + size.y - border),
                ImGui::GetColorU32(*selected ? colors[ImGuiCol_Border]
                : colors[ImGuiCol_Button]), ImGui::GetStyle().FrameRounding);

    // If the button has been selected then darken it
    bool pushedStyleColor = false;
    if (*selected) {
        ImVec4 bttn = colors[ImGuiCol_Button];
        ImVec4 darkenedButton = ImColor(bttn.x, bttn.y, bttn.z, bttn.w - 0.4f);
        ImGui::PushStyleColor(ImGuiCol_Button, darkenedButton);
        ImVec4 hovr = colors[ImGuiCol_ButtonHovered];
        ImVec4 darkenedHover = ImColor(hovr.x, hovr.y, hovr.z, hovr.w - 0.85f);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, darkenedHover);
        pushedStyleColor = true;
    }
    // Add the (slightly-darkened if selected) button to the center of the border
    bool toggleButtonClicked = false;
    p = {p.x + border, p.y + border};
    ImGui::SetCursorScreenPos(p);
    if (ImGui::Button(str_id.data(), ImVec2(innerSize.x,innerSize.y))) {
        *selected = !*selected;
        toggleButtonClicked = true;
    }

    // Return style properties to their previous values
    ImGui::PopStyleVar(2); // FrameRounding and FrameBorderSize
    if (pushedStyleColor) ImGui::PopStyleColor(2);

    // Advance ImGui cursor according to size of full toggle button
    ImGui::SetCursorScreenPos(pIn);
    ImGui::Dummy(ImVec2(size.x, size.y));

    return toggleButtonClicked;
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

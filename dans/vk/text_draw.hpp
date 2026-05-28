// dans/vk/text_draw.hpp
//
#pragma once

#include "dans/vk/types.hpp"
// StdLib
#include <string>
#include <string_view>
//

namespace dans::vk
{

// World-space text. The baseline of the first glyph sits at `position` in the
// coordinate system used by the active camera's view-projection matrix.
struct TextDrawConfig
{
    Vec2 position{};
    std::string_view text{};
    Color color{Color::white};
    f32 size_scale{1.0f};
};

// Screen-space text. Position is in framebuffer pixels with the origin at the
// top-left corner. Unaffected by the camera.
struct TextScreenConfig
{
    Vec2 position{};
    std::string_view text{};
    Color color{Color::white};
    f32 size_scale{1.0f};
};

struct TextDrawCommand
{
    Vec2 position{};
    std::string text{};
    Color color{Color::white};
    f32 size_scale{1.0f};
};

}  // namespace dans::vk

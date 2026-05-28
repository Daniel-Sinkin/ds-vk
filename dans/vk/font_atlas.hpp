// dans/vk/font_atlas.hpp
//
#pragma once

#include "dans/vk/types.hpp"
// StdLib
#include <filesystem>
#include <vector>
//

namespace dans::vk
{

struct GlyphMetrics
{
    u16 atlas_x{};
    u16 atlas_y{};
    u16 atlas_w{};
    u16 atlas_h{};
    f32 offset_x{};
    f32 offset_y{};
    f32 advance{};
};

struct FontMetrics
{
    f32 ascent{};
    f32 descent{};
    f32 line_gap{};
    f32 pixel_size{};
};

struct FontBakeConfig
{
    std::filesystem::path ttf_path{};
    f32 pixel_size{16.0f};
    u32 first_codepoint{32u};
    u32 codepoint_count{96u};
    u32 atlas_width{512u};
    u32 atlas_height{512u};
};

struct BakedFont
{
    FontMetrics metrics{};
    std::vector<GlyphMetrics> glyphs{};
    std::vector<u8> pixels{};
    u32 first_codepoint{};
    u32 atlas_width{};
    u32 atlas_height{};
};

[[nodiscard]] auto bake_font(const FontBakeConfig& config) -> BakedFont;

[[nodiscard]] auto glyph_for(const BakedFont& font, u32 codepoint) noexcept
    -> const GlyphMetrics*;

[[nodiscard]] auto line_advance(const FontMetrics& metrics) noexcept -> f32;

}  // namespace dans::vk

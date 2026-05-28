// app/main_2d.cpp
//
#include "dans/vk/math.hpp"
#include "dans/vk/runtime.hpp"
#include "dans/vk/shape_draw.hpp"
#include "dans/vk/text_draw.hpp"
#include "dans/vk/types.hpp"
// StdLib
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <format>
#include <iostream>
#include <numbers>
#include <string>
#include <string_view>
#include <vector>
//

namespace
{

struct TensorNode
{
    dans::vk::Vec2 position{};
    dans::vk::i32 rank{};
    std::string label{};
};

struct Bond
{
    dans::vk::usize a{};
    dans::vk::usize b{};
};

[[nodiscard]] auto color_for_rank(dans::vk::i32 rank) noexcept -> dans::vk::Color
{
    using dans::vk::Color;
    switch (rank)
    {
        case 1:
            return Color{0.36f, 0.62f, 0.86f, 1.0f};
        case 2:
            return Color{0.92f, 0.50f, 0.26f, 1.0f};
        case 3:
            return Color{0.40f, 0.78f, 0.55f, 1.0f};
        case 4:
            return Color{0.86f, 0.55f, 0.86f, 1.0f};
        default:
            return Color{0.55f, 0.60f, 0.70f, 1.0f};
    }
}

[[nodiscard]] auto node_radius(dans::vk::i32 rank) noexcept -> dans::vk::f32
{
    return 26.0f + 3.0f * static_cast<dans::vk::f32>(std::clamp(rank, 0, 4));
}

class TwoDApp
{
  public:
    auto setup(dans::vk::Runtime& runtime) -> void
    {
        runtime_ = &runtime;

        const auto spacing = 140.0f;
        for (auto i = 0; i < 8; ++i)
        {
            nodes_.push_back(
                TensorNode{
                    .position = {static_cast<dans::vk::f32>(i) * spacing, 0.0f},
                    .rank = (i == 0 or i == 7) ? 1 : 2,
                    .label = std::format("T{}", i + 1),
                }
            );
        }
        for (auto i = 0; i < 4; ++i)
        {
            nodes_.push_back(
                TensorNode{
                    .position
                    = {(static_cast<dans::vk::f32>(i) + 1.0f) * spacing - 0.5f * spacing, 170.0f},
                    .rank = 3,
                    .label = std::format("M{}", i + 1),
                }
            );
        }

        for (auto i = 0u; i + 1u < 8u; ++i)
        {
            bonds_.push_back(Bond{.a = i, .b = i + 1u});
        }
        bonds_.push_back(Bond{.a = 1u, .b = 8u});
        bonds_.push_back(Bond{.a = 3u, .b = 9u});
        bonds_.push_back(Bond{.a = 4u, .b = 10u});
        bonds_.push_back(Bond{.a = 6u, .b = 11u});

        const auto pivot = dans::vk::Vec2{(7.0f * spacing) * 0.5f, 85.0f};
        runtime.set_camera_2d(pivot, 1.0f);

#if defined(DANS_VK_DEFAULT_FONT_PATH)
        runtime.load_font(
            {
                .ttf_path = DANS_VK_DEFAULT_FONT_PATH,
                .pixel_size = 26.0f,
            }
        );
#endif
    }

    auto update(dans::vk::FrameContext& frame, const dans::vk::f32 dt) -> void
    {
        elapsed_ += dt;
        const auto& input = frame.input;
        const auto mouse_world = runtime_->screen_to_world_2d(input.mouse_px);

        if (not input.mouse_captured_by_ui)
        {
            hovered_ = hit_test(mouse_world);
        }
        else
        {
            hovered_ = -1;
        }

        if (input.left_click.occurred and not input.mouse_captured_by_ui)
        {
            if (hovered_ >= 0)
            {
                if (input.left_click.modifiers.shift)
                {
                    toggle_selected(hovered_);
                }
                else if (not is_selected(hovered_))
                {
                    selected_.clear();
                    selected_.push_back(static_cast<dans::vk::usize>(hovered_));
                }
                dragging_ = true;
                drag_anchor_world_ = mouse_world;
                drag_initial_positions_.clear();
                drag_initial_positions_.reserve(selected_.size());
                for (const auto idx : selected_)
                {
                    drag_initial_positions_.push_back(nodes_[idx].position);
                }
            }
            else if (not input.left_click.modifiers.shift)
            {
                selected_.clear();
            }
        }

        if (dragging_ and not input.left_button_down)
        {
            dragging_ = false;
        }
        if (dragging_)
        {
            const auto delta = mouse_world - drag_anchor_world_;
            for (auto i = 0u; i < selected_.size(); ++i)
            {
                nodes_[selected_[i]].position = drag_initial_positions_[i] + delta;
            }
        }

        draw_grid(frame);
        draw_abstraction(frame);
        draw_bonds(frame);
        draw_free_legs(frame);
        draw_nodes(frame);
        draw_hud(frame, mouse_world);
        draw_radial_menu(frame);
    }

  private:
    dans::vk::Runtime* runtime_{};
    std::vector<TensorNode> nodes_{};
    std::vector<Bond> bonds_{};
    std::vector<dans::vk::usize> selected_{};
    dans::vk::i32 hovered_{-1};
    bool dragging_{};
    dans::vk::Vec2 drag_anchor_world_{};
    std::vector<dans::vk::Vec2> drag_initial_positions_{};
    dans::vk::f32 elapsed_{};

    [[nodiscard]] auto hit_test(const dans::vk::Vec2 world_pos) const -> dans::vk::i32
    {
        for (auto i = static_cast<dans::vk::i32>(nodes_.size()) - 1; i >= 0; --i)
        {
            const auto& n = nodes_[static_cast<dans::vk::usize>(i)];
            const auto r = node_radius(n.rank);
            if (std::abs(world_pos.x - n.position.x) <= r
                and std::abs(world_pos.y - n.position.y) <= r)
            {
                return i;
            }
        }
        return -1;
    }

    [[nodiscard]] auto is_selected(const dans::vk::i32 idx) const -> bool
    {
        return std::find(selected_.begin(), selected_.end(), static_cast<dans::vk::usize>(idx))
               != selected_.end();
    }

    auto toggle_selected(const dans::vk::i32 idx) -> void
    {
        const auto target = static_cast<dans::vk::usize>(idx);
        const auto it = std::find(selected_.begin(), selected_.end(), target);
        if (it != selected_.end())
        {
            selected_.erase(it);
        }
        else
        {
            selected_.push_back(target);
        }
    }

    auto draw_grid(dans::vk::FrameContext& frame) -> void
    {
        const auto pivot = runtime_->camera_2d_pivot();
        const auto zoom = runtime_->camera_2d_zoom();
        const auto extent_w = static_cast<dans::vk::f32>(std::max(1u, frame.extent.width));
        const auto extent_h = static_cast<dans::vk::f32>(std::max(1u, frame.extent.height));
        const auto half_w = extent_w * 0.5f * zoom;
        const auto half_h = extent_h * 0.5f * zoom;

        const auto base_spacing = 50.0f;
        const auto spacing = base_spacing;
        const auto dot_radius = std::max(1.0f, 1.4f * zoom);

        const auto x_start
            = static_cast<dans::vk::i32>(std::floor((pivot.x - half_w) / spacing)) - 1;
        const auto x_end = static_cast<dans::vk::i32>(std::ceil((pivot.x + half_w) / spacing)) + 1;
        const auto y_start
            = static_cast<dans::vk::i32>(std::floor((pivot.y - half_h) / spacing)) - 1;
        const auto y_end = static_cast<dans::vk::i32>(std::ceil((pivot.y + half_h) / spacing)) + 1;

        const auto grid_color = dans::vk::Color{1.0f, 1.0f, 1.0f, 0.10f};
        for (auto xi = x_start; xi <= x_end; ++xi)
        {
            for (auto yi = y_start; yi <= y_end; ++yi)
            {
                frame.draw.circle(
                    {
                        .center
                        = {static_cast<dans::vk::f32>(xi) * spacing, static_cast<dans::vk::f32>(yi) * spacing},
                        .radius = dot_radius,
                        .fill_color = grid_color,
                    }
                );
            }
        }
    }

    auto draw_abstraction(dans::vk::FrameContext& frame) -> void
    {
        if (nodes_.size() < 8u)
        {
            return;
        }
        auto min_x = nodes_[0].position.x;
        auto max_x = min_x;
        auto min_y = nodes_[0].position.y;
        auto max_y = min_y;
        for (auto i = 0u; i < 8u; ++i)
        {
            min_x = std::min(min_x, nodes_[i].position.x);
            max_x = std::max(max_x, nodes_[i].position.x);
            min_y = std::min(min_y, nodes_[i].position.y);
            max_y = std::max(max_y, nodes_[i].position.y);
        }
        const auto pad = 70.0f;
        frame.draw.rect(
            {
                .position = {min_x - pad, min_y - pad},
                .size = {(max_x - min_x) + 2.0f * pad, (max_y - min_y) + 2.0f * pad},
                .fill_color = dans::vk::Color{0.40f, 0.55f, 0.80f, 0.05f},
                .stroke_color = dans::vk::Color{0.55f, 0.70f, 0.95f, 0.55f},
                .stroke_width = 1.5f,
                .corner_radius = 28.0f,
            }
        );
        frame.draw.text(
            {
                .position = {min_x - pad + 14.0f, min_y - pad + 24.0f},
                .text = "MPS chain (8)",
                .color = dans::vk::Color{0.78f, 0.88f, 1.0f, 0.85f},
                .size_scale = 0.55f,
            }
        );
    }

    auto draw_bonds(dans::vk::FrameContext& frame) -> void
    {
        for (auto bi = 0u; bi < bonds_.size(); ++bi)
        {
            const auto& bond = bonds_[bi];
            if (bond.a >= nodes_.size() or bond.b >= nodes_.size())
            {
                continue;
            }
            const auto& a = nodes_[bond.a];
            const auto& b = nodes_[bond.b];
            const auto mid = 0.5f * (a.position + b.position);
            const dans::vk::Vec2 along{b.position.x - a.position.x, b.position.y - a.position.y};
            const auto length = std::sqrt(along.x * along.x + along.y * along.y);
            const auto perp_norm
                = (length > 1e-4f)
                      ? dans::vk::Vec2{-along.y / length, along.x / length}
                      : dans::vk::Vec2{0.0f, 1.0f};
            const auto bulge = mid + perp_norm * 16.0f;

            const auto is_branch = (bi >= 7u);
            const auto color = is_branch
                                   ? dans::vk::Color{0.50f, 0.85f, 0.55f, 0.95f}
                                   : dans::vk::Color{0.48f, 0.74f, 1.0f, 0.95f};
            frame.draw.bezier(
                {
                    .start = a.position,
                    .control = bulge,
                    .end = b.position,
                    .color = color,
                    .thickness = 3.0f,
                    .dash_on = is_branch ? 8.0f : 0.0f,
                    .dash_off = is_branch ? 6.0f : 0.0f,
                    .segments = 28u,
                }
            );
        }
    }

    auto draw_free_legs(dans::vk::FrameContext& frame) -> void
    {
        for (auto i = 0u; i < nodes_.size() and i < 8u; ++i)
        {
            const auto& n = nodes_[i];
            const auto r = node_radius(n.rank);
            const dans::vk::Vec2 leg_dir{0.0f, 1.0f};
            const auto leg_start
                = dans::vk::Vec2{n.position.x + leg_dir.x * r, n.position.y + leg_dir.y * r};
            const auto leg_end = dans::vk::Vec2{
                leg_start.x + leg_dir.x * 28.0f,
                leg_start.y + leg_dir.y * 28.0f,
            };
            frame.draw.line_2d(
                {
                    .start = leg_start,
                    .end = leg_end,
                    .color = dans::vk::Color{0.95f, 0.46f, 0.42f, 0.95f},
                    .thickness = 2.5f,
                }
            );
            frame.draw.circle(
                {
                    .center = leg_end,
                    .radius = 3.6f,
                    .fill_color = dans::vk::Color{0.95f, 0.46f, 0.42f, 1.0f},
                }
            );
            frame.draw.text(
                {
                    .position = {leg_end.x - 12.0f, leg_end.y + 18.0f},
                    .text = "phys",
                    .color = dans::vk::Color{0.96f, 0.55f, 0.52f, 1.0f},
                    .size_scale = 0.46f,
                }
            );
        }
    }

    auto draw_nodes(dans::vk::FrameContext& frame) -> void
    {
        for (auto i = 0u; i < nodes_.size(); ++i)
        {
            const auto& n = nodes_[i];
            const auto r = node_radius(n.rank);
            const auto idx_i32 = static_cast<dans::vk::i32>(i);
            const auto selected = is_selected(idx_i32);
            const auto hovered = (hovered_ == idx_i32);

            const auto fill = color_for_rank(n.rank);
            auto stroke = dans::vk::Color{0.05f, 0.08f, 0.12f, 0.75f};
            auto stroke_w = 1.6f;
            if (selected)
            {
                stroke = dans::vk::Color{1.0f, 1.0f, 1.0f, 1.0f};
                stroke_w = 3.0f;
            }
            else if (hovered)
            {
                stroke = dans::vk::Color{1.0f, 1.0f, 1.0f, 0.65f};
                stroke_w = 2.2f;
            }

            frame.draw.rect(
                {
                    .position = {n.position.x - r, n.position.y - r},
                    .size = {2.0f * r, 2.0f * r},
                    .fill_color = fill,
                    .stroke_color = stroke,
                    .stroke_width = stroke_w,
                    .bevel_size = r * 0.34f,
                }
            );

            frame.draw.text(
                {
                    .position = {n.position.x - 11.0f, n.position.y + 5.0f},
                    .text = n.label,
                    .color = dans::vk::Color{1.0f, 1.0f, 1.0f, 1.0f},
                    .size_scale = 0.55f,
                }
            );
        }
    }

    auto draw_hud(dans::vk::FrameContext& frame, const dans::vk::Vec2 mouse_world) -> void
    {
        frame.draw.text_screen(
            {
                .position = {24.0f, 36.0f},
                .text = "dans_vk 2D viz (tensor network mock)",
                .color = dans::vk::Color::white,
            }
        );

        const auto info = std::format(
            "selected: {}  hovered: {}  zoom: {:.2f}  mouse_world: ({:.1f}, {:.1f})",
            selected_.size(),
            hovered_,
            static_cast<dans::vk::f64>(runtime_->camera_2d_zoom()),
            static_cast<dans::vk::f64>(mouse_world.x),
            static_cast<dans::vk::f64>(mouse_world.y)
        );
        frame.draw.text_screen(
            {
                .position = {24.0f, 70.0f},
                .text = info,
                .color = dans::vk::Color{0.80f, 0.88f, 0.98f, 1.0f},
                .size_scale = 0.62f,
            }
        );
        frame.draw.text_screen(
            {
                .position = {24.0f, 96.0f},
                .text = "left-click select  shift+click multi  drag to move  RMB/MMB pan  wheel zoom",
                .color = dans::vk::Color{0.62f, 0.74f, 0.90f, 1.0f},
                .size_scale = 0.52f,
            }
        );
    }

    auto draw_radial_menu(dans::vk::FrameContext& frame) -> void
    {
        const auto extent_w = static_cast<dans::vk::f32>(std::max(1u, frame.extent.width));
        const auto extent_h = static_cast<dans::vk::f32>(std::max(1u, frame.extent.height));
        const auto cx = extent_w - 110.0f;
        const auto cy = extent_h - 110.0f;

        const auto r_in = 32.0f;
        const auto r_out = 78.0f;
        constexpr auto k_sectors = 6;
        constexpr auto k_two_pi = 2.0f * std::numbers::pi_v<dans::vk::f32>;

        for (auto i = 0; i < k_sectors; ++i)
        {
            const auto a0 = k_two_pi * static_cast<dans::vk::f32>(i) / k_sectors
                            - k_two_pi / (2.0f * static_cast<dans::vk::f32>(k_sectors));
            const auto a1 = a0 + k_two_pi / static_cast<dans::vk::f32>(k_sectors);
            const auto hue = static_cast<dans::vk::f32>(i) / static_cast<dans::vk::f32>(k_sectors);
            const dans::vk::Color fill{
                0.35f + 0.35f * std::cos(hue * k_two_pi),
                0.35f + 0.35f * std::cos(hue * k_two_pi + 2.0f),
                0.40f + 0.35f * std::cos(hue * k_two_pi + 4.0f),
                0.82f,
            };
            frame.draw.sector(
                {
                    .center = {cx, cy},
                    .outer_radius = r_out,
                    .inner_radius = r_in,
                    .start_angle = a0,
                    .end_angle = a1,
                    .fill_color = fill,
                    .stroke_color = dans::vk::Color{0.05f, 0.08f, 0.12f, 0.7f},
                    .stroke_width = 1.0f,
                    .screen_space = true,
                }
            );
        }
        frame.draw.circle(
            {
                .center = {cx, cy},
                .radius = r_in * 0.65f,
                .fill_color = dans::vk::Color{0.12f, 0.15f, 0.20f, 0.95f},
                .stroke_color = dans::vk::Color{0.55f, 0.65f, 0.80f, 0.85f},
                .stroke_width = 1.5f,
                .screen_space = true,
            }
        );
        frame.draw.text_screen(
            {
                .position = {cx - 22.0f, cy + 5.0f},
                .text = "menu",
                .color = dans::vk::Color::white,
                .size_scale = 0.55f,
            }
        );
    }
};

[[nodiscard]] auto parse_u32(const char* text, const dans::vk::u32 fallback) noexcept
    -> dans::vk::u32
{
    char* end = nullptr;
    const auto value = std::strtoul(text, &end, 10);
    if (end == text)
    {
        return fallback;
    }
    return static_cast<dans::vk::u32>(value);
}

auto print_usage(const char* executable) -> void
{
    std::cout << "usage: " << executable
              << " [--smoke-frames N] [--screenshot PATH] [--hide-ui]\n";
}

}  // namespace

auto main(int argc, char** argv) -> int
{
    try
    {
        dans::vk::RuntimeConfig config{
            .window_title = "dans_vk 2D viz",
            .render_mode = dans::vk::RenderMode::two_d,
        };
        for (auto i = 1; i < argc; ++i)
        {
            const std::string_view arg{argv[i]};
            if (arg == "--help")
            {
                print_usage(argv[0]);
                return 0;
            }
            if (arg == "--smoke-frames" and i + 1 < argc)
            {
                config.smoke_frames = parse_u32(argv[++i], 0u);
            }
            else if (arg == "--screenshot" and i + 1 < argc)
            {
                config.screenshot_path = argv[++i];
            }
            else if (arg == "--hide-ui")
            {
                config.hide_ui = true;
            }
            else
            {
                std::cerr << "unknown or incomplete argument: " << arg << '\n';
                print_usage(argv[0]);
                return 2;
            }
        }

        TwoDApp app{};
        dans::vk::Runtime runtime{std::move(config)};
        return runtime.run_prototype(app);
    }
    catch (const std::exception& error)
    {
        std::cerr << "fatal: " << error.what() << '\n';
        return 1;
    }
}

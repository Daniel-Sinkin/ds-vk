#include "ds_vk/math.hpp"
#include "ds_vk/mesh.hpp"
#include "ds_vk/plugins/viz.hpp"
#include "ds_vk/runtime.hpp"
#include "ds_vk/types.hpp"

#include <cstdlib>
#include <exception>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <string_view>

namespace
{
class BasicApp
{
  public:
    auto setup(ds_vk::Runtime& runtime) -> void
    {
        floor_ = runtime.upload_mesh(ds_vk::make_quad(8.0f, ds_vk::Color::white));
        cube_ = runtime.upload_mesh(ds_vk::make_cube(1.0f, ds_vk::Color::white));
        sphere_ = runtime.upload_mesh(ds_vk::make_uv_sphere({.radius = 1.0f}));
        runtime.camera({
            .pivot = 0.6f * ds_vk::k_axis_z,
            .distance = 6.0f,
            .yaw = glm::radians(42.0f),
            .pitch = glm::radians(26.0f),
        });
    }

    auto update(ds_vk::FrameContext& frame, ds_vk::f32 dt_seconds) -> void
    {
        elapsed_seconds_ += dt_seconds;
        frame.draw.set_environment({
            .background_color = ds_vk::Color{0.05f, 0.065f, 0.08f, 1.0f},
            .background_top_color = ds_vk::Color{0.24f, 0.31f, 0.36f, 1.0f},
            .gradient_background = true,
        });
        frame.draw.directional_light({
            .direction = glm::normalize(ds_vk::Vec3{-0.35f, -0.45f, -0.90f}),
            .intensity = 2.4f,
            .shadow = {.enabled = true},
        });
        frame.draw.radial_light({
            .position = {1.5f, -2.0f, 2.4f},
            .color = ds_vk::Color{0.55f, 0.75f, 1.0f, 1.0f},
            .intensity = 8.0f,
            .range = 5.5f,
        });

        frame.draw.draw_mesh({
            .mesh = floor_,
            .material = {.base_color = ds_vk::Color{0.30f, 0.34f, 0.31f, 1.0f}, .roughness = 0.9f},
            .mask = {.shadow_producer = false},
        });
        frame.draw.draw_mesh({
            .mesh = cube_,
            .transform = {
                .translation = {-1.1f, 0.0f, 0.65f},
                .rotation = glm::angleAxis(elapsed_seconds_ * 0.65f, ds_vk::k_axis_z),
            },
            .material = {.base_color = ds_vk::Color{0.92f, 0.46f, 0.22f, 1.0f}, .roughness = 0.52f},
        });
        frame.draw.draw_mesh({
            .mesh = sphere_,
            .transform = {.translation = {1.2f, 0.35f, 0.9f}, .scale = ds_vk::Vec3{0.9f}},
            .material = {
                .base_color = ds_vk::Color{0.18f, 0.62f, 0.85f, 1.0f},
                .metallic = 0.0f,
                .roughness = 0.38f,
            },
        });

        frame.draw.debug_arrow({.origin = {}, .vector = ds_vk::k_axis_x, .color = ds_vk::Color::red});
        frame.draw.debug_arrow({.origin = {}, .vector = ds_vk::k_axis_y, .color = ds_vk::Color::green});
        frame.draw.debug_arrow({.origin = {}, .vector = ds_vk::k_axis_z, .color = ds_vk::Color::blue});
    }

  private:
    ds_vk::MeshHandle floor_{};
    ds_vk::MeshHandle cube_{};
    ds_vk::MeshHandle sphere_{};
    ds_vk::f32 elapsed_seconds_{};
};

auto parse_u32(const char* text, ds_vk::u32 fallback) noexcept -> ds_vk::u32
{
    char* end = nullptr;
    const auto value = std::strtoul(text, &end, 10);
    if (end == text)
    {
        return fallback;
    }
    return static_cast<ds_vk::u32>(value);
}

auto print_usage(const char* executable) -> void
{
    std::cout << "usage: " << executable
              << " [--smoke-frames N] [--screenshot PATH] [--hide-ui]"
                 " [--transparent-screenshot]\n";
}
}  // namespace

auto main(int argc, char** argv) -> int
{
    try
    {
        ds_vk::RuntimeConfig config{.window_title = "ds_vk Basic App"};
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
            else if (arg == "--transparent-screenshot")
            {
                config.transparent_screenshot = true;
            }
            else
            {
                std::cerr << "unknown or incomplete argument: " << arg << '\n';
                print_usage(argv[0]);
                return 2;
            }
        }

        BasicApp app{};
        ds_vk::Runtime runtime{std::move(config)};
        return runtime.run_prototype(app);
    }
    catch (const std::exception& error)
    {
        std::cerr << "fatal: " << error.what() << '\n';
        return 1;
    }
}

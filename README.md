# ds-vk

Personal Vulkan helper/runtime for C++ visualization experiments.

This is not a game engine or a cross-API abstraction. It is a small Vulkan runtime
and helper library intended to be pulled directly into project repos, usually as a
Git submodule under `external/ds-vk`.

## Use As A Dependency

```sh
git submodule add git@github.com:Daniel-Sinkin/ds-vk.git external/ds-vk
```

```cmake
add_subdirectory(external/ds-vk)
add_executable(my_app app/main.cpp)
target_link_libraries(my_app PRIVATE ds_vk ds_vk_viz ds_vk_picker)
```

When this repo is included with `add_subdirectory`, examples and tests are off by
default. The reusable targets are:

- `ds_vk_core` / `ds_vk::core` for CPU-side math, geometry, mesh, camera, glTF helpers
- `ds_vk` / `ds_vk::runtime` for the Vulkan runtime
- `ds_vk_picker`, `ds_vk_viz`, and `ds_vk_manipulator` for optional helper plugins

The Vulkan runtime still expects system Vulkan SDK support, including `glslc`.

## Local Assets

Redistribution-sensitive or bulky assets belong in `local/assets/`. That directory
is intentionally ignored by Git. The framework does not require checked-in assets
to build; project repos can define their own `DS_VK_ASSET_DIR` pointing at their
own ignored local asset cache.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

If Vulkan and `glslc` are available, the top-level build also produces
`ds_vk_basic_app`:

```sh
./run.sh
./run.sh --smoke-frames 20 --hide-ui --screenshot run/basic.png
```

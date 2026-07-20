#pragma once

#include "Math.hpp"

#include <memory>

struct Color {
    float red;
    float green;
    float blue;
    float alpha;
};

struct RgbColor {
    float red;
    float green;
    float blue;
};

struct DirectionalLight {
    resonance::math::Vector3 direction;
    RgbColor color;
};

struct AmbientLight {
    RgbColor color;
};

struct SceneLighting {
    DirectionalLight directional;
    AmbientLight ambient;
};

class GpuWindow {
  public:
    GpuWindow();
    ~GpuWindow();

    GpuWindow(const GpuWindow &) = delete;
    GpuWindow &operator=(const GpuWindow &) = delete;

    void render(Color clear_color, const SceneLighting &lighting);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

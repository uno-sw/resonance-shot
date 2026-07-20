#pragma once

#include <memory>

struct Color {
    float red;
    float green;
    float blue;
    float alpha;
};

class GpuWindow {
  public:
    GpuWindow();
    ~GpuWindow();

    GpuWindow(const GpuWindow &) = delete;
    GpuWindow &operator=(const GpuWindow &) = delete;

    void render(Color clear_color);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#include "GpuWindow.hpp"

#include <SDL3/SDL.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

[[noreturn]] void sdl_fail(const std::string &message) {
    throw std::runtime_error(message + ": " + SDL_GetError());
}

struct WindowDeleter {
    void operator()(SDL_Window *window) const noexcept { SDL_DestroyWindow(window); }
};

using Window = std::unique_ptr<SDL_Window, WindowDeleter>;

struct GpuDeviceDeleter {
    void operator()(SDL_GPUDevice *device) const noexcept { SDL_DestroyGPUDevice(device); }
};

using GpuDevice = std::unique_ptr<SDL_GPUDevice, GpuDeviceDeleter>;

GpuDevice create_gpu_device() {
    GpuDevice device{SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL, true, "metal")};
    if (device == nullptr) {
        sdl_fail("could not create Metal GPU device");
    }
    if (std::string_view(SDL_GetGPUDeviceDriver(device.get())) != "metal") {
        throw std::runtime_error("SDL selected a non-Metal GPU backend");
    }
    return device;
}

Window create_window() {
    Window window{SDL_CreateWindow("Resonance Shot", 1280, 720, 0)};
    if (window == nullptr) {
        sdl_fail("could not create window");
    }
    return window;
}

class ClaimedGpuWindow {
  public:
    ClaimedGpuWindow(SDL_GPUDevice *device, SDL_Window *window) : device_(device), window_(window) {
        if (!SDL_ClaimWindowForGPUDevice(device_, window_)) {
            sdl_fail("could not claim GPU window");
        }
    }

    ClaimedGpuWindow(const ClaimedGpuWindow &) = delete;
    ClaimedGpuWindow &operator=(const ClaimedGpuWindow &) = delete;

    ~ClaimedGpuWindow() {
        SDL_WaitForGPUIdle(device_);
        SDL_ReleaseWindowFromGPUDevice(device_, window_);
    }

  private:
    SDL_GPUDevice *device_;
    SDL_Window *window_;
};

} // namespace

class GpuWindow::Impl {
  public:
    Impl()
        : device_(create_gpu_device()), window_(create_window()),
          claim_(device_.get(), window_.get()) {}

    void render(Color clear_color) {
        SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(device_.get());
        if (commands == nullptr) {
            sdl_fail("could not acquire GPU command buffer");
        }

        SDL_GPUTexture *swapchain_texture = nullptr;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands, window_.get(), &swapchain_texture,
                                                   nullptr, nullptr)) {
            const std::string error = SDL_GetError();
            SDL_CancelGPUCommandBuffer(commands);
            throw std::runtime_error("could not acquire GPU swapchain texture: " + error);
        }

        if (swapchain_texture == nullptr) {
            SDL_CancelGPUCommandBuffer(commands);
            return;
        }

        SDL_GPUColorTargetInfo color_target{};
        color_target.texture = swapchain_texture;
        color_target.clear_color = {
            .r = clear_color.red,
            .g = clear_color.green,
            .b = clear_color.blue,
            .a = clear_color.alpha,
        };
        color_target.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass *render_pass =
            SDL_BeginGPURenderPass(commands, &color_target, 1, nullptr);
        if (render_pass == nullptr) {
            const std::string error = SDL_GetError();
            SDL_SubmitGPUCommandBuffer(commands);
            throw std::runtime_error("could not begin GPU render pass: " + error);
        }

        SDL_EndGPURenderPass(render_pass);

        if (!SDL_SubmitGPUCommandBuffer(commands)) {
            sdl_fail("could not submit GPU command buffer");
        }
    }

  private:
    GpuDevice device_;
    Window window_;
    ClaimedGpuWindow claim_;
};

GpuWindow::GpuWindow() : impl_(std::make_unique<Impl>()) {}

GpuWindow::~GpuWindow() = default;

void GpuWindow::render(Color clear_color) { impl_->render(clear_color); }

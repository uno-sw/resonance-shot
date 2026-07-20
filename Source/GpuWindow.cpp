#include "GpuWindow.hpp"
#include "FloorShader.hpp"
#include "Math.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

struct alignas(16) CameraUniforms {
    std::array<float, 16> view_projection;
};

static_assert(sizeof(CameraUniforms) == 64);
static_assert(alignof(CameraUniforms) == 16);

CameraUniforms create_camera_uniforms(float aspect_ratio) {
    constexpr float pi = 3.14159265358979323846F;
    const resonance::math::Matrix4 view = resonance::math::look_at(
        {.x = 0.0F, .y = 6.0F, .z = 8.0F}, {.x = 0.0F, .y = 1.2F, .z = -4.0F},
        {.x = 0.0F, .y = 1.0F, .z = 0.0F});
    const resonance::math::Matrix4 projection =
        resonance::math::perspective(pi / 3.0F, aspect_ratio, 0.1F, 100.0F);
    return {.view_projection = resonance::math::multiply(projection, view).elements};
}

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

struct ShaderDeleter {
    SDL_GPUDevice *device;

    void operator()(SDL_GPUShader *shader) const noexcept { SDL_ReleaseGPUShader(device, shader); }
};

using GpuShader = std::unique_ptr<SDL_GPUShader, ShaderDeleter>;

struct GraphicsPipelineDeleter {
    SDL_GPUDevice *device;

    void operator()(SDL_GPUGraphicsPipeline *pipeline) const noexcept {
        SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    }
};

using GraphicsPipeline = std::unique_ptr<SDL_GPUGraphicsPipeline, GraphicsPipelineDeleter>;

struct TextureDeleter {
    SDL_GPUDevice *device;

    void operator()(SDL_GPUTexture *texture) const noexcept {
        SDL_ReleaseGPUTexture(device, texture);
    }
};

using GpuTexture = std::unique_ptr<SDL_GPUTexture, TextureDeleter>;

constexpr SDL_GPUTextureFormat depth_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
constexpr std::uint32_t capsule_vertex_count = 9U * 24U * 6U;

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

GpuShader create_shader(SDL_GPUDevice *device, SDL_GPUShaderStage stage, const char *entrypoint) {
    const SDL_GPUShaderCreateInfo create_info{
        .code_size = floor_shader_source.size(),
        .code = reinterpret_cast<const std::uint8_t *>(floor_shader_source.data()),
        .entrypoint = entrypoint,
        .format = SDL_GPU_SHADERFORMAT_MSL,
        .stage = stage,
        .num_uniform_buffers = stage == SDL_GPU_SHADERSTAGE_VERTEX ? 1U : 0U,
    };

    GpuShader shader{SDL_CreateGPUShader(device, &create_info), ShaderDeleter{device}};
    if (shader == nullptr) {
        sdl_fail(std::string("could not create ") + entrypoint + " shader");
    }
    return shader;
}

GraphicsPipeline create_graphics_pipeline(SDL_GPUDevice *device, SDL_Window *window,
                                          const char *vertex_entrypoint,
                                          const char *fragment_entrypoint) {
    GpuShader vertex_shader = create_shader(device, SDL_GPU_SHADERSTAGE_VERTEX, vertex_entrypoint);
    GpuShader fragment_shader =
        create_shader(device, SDL_GPU_SHADERSTAGE_FRAGMENT, fragment_entrypoint);

    const SDL_GPUColorTargetDescription color_target{
        .format = SDL_GetGPUSwapchainTextureFormat(device, window),
    };
    const SDL_GPUGraphicsPipelineCreateInfo create_info{
        .vertex_shader = vertex_shader.get(),
        .fragment_shader = fragment_shader.get(),
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .depth_stencil_state =
            {
                .compare_op = SDL_GPU_COMPAREOP_LESS,
                .enable_depth_test = true,
                .enable_depth_write = true,
            },
        .target_info =
            {
                .color_target_descriptions = &color_target,
                .num_color_targets = 1,
                .depth_stencil_format = depth_format,
                .has_depth_stencil_target = true,
            },
    };

    GraphicsPipeline pipeline{SDL_CreateGPUGraphicsPipeline(device, &create_info),
                              GraphicsPipelineDeleter{device}};

    if (pipeline == nullptr) {
        sdl_fail(std::string("could not create ") + vertex_entrypoint + " graphics pipeline");
    }
    return pipeline;
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
          claim_(device_.get(), window_.get()),
          floor_pipeline_(create_graphics_pipeline(device_.get(), window_.get(), "floor_vertex",
                                                   "floor_fragment")),
          capsule_pipeline_(create_graphics_pipeline(device_.get(), window_.get(), "capsule_vertex",
                                                     "capsule_fragment")),
          depth_texture_(nullptr, TextureDeleter{device_.get()}),
          camera_uniforms_(create_camera_uniforms(16.0F / 9.0F)) {
        if (!SDL_GPUTextureSupportsFormat(device_.get(), depth_format, SDL_GPU_TEXTURETYPE_2D,
                                          SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)) {
            throw std::runtime_error("Metal GPU does not support the required D32 depth format");
        }
    }

    void render(Color clear_color) {
        SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(device_.get());
        if (commands == nullptr) {
            sdl_fail("could not acquire GPU command buffer");
        }

        SDL_GPUTexture *swapchain_texture = nullptr;
        std::uint32_t swapchain_width = 0;
        std::uint32_t swapchain_height = 0;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands, window_.get(), &swapchain_texture,
                                                   &swapchain_width, &swapchain_height)) {
            const std::string error = SDL_GetError();
            SDL_CancelGPUCommandBuffer(commands);
            throw std::runtime_error("could not acquire GPU swapchain texture: " + error);
        }

        if (swapchain_texture == nullptr) {
            SDL_CancelGPUCommandBuffer(commands);
            return;
        }

        ensure_depth_texture(swapchain_width, swapchain_height);

        SDL_PushGPUVertexUniformData(commands, 0, &camera_uniforms_, sizeof(camera_uniforms_));

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

        SDL_GPUDepthStencilTargetInfo depth_target{};
        depth_target.texture = depth_texture_.get();
        depth_target.clear_depth = 1.0F;
        depth_target.load_op = SDL_GPU_LOADOP_CLEAR;
        depth_target.store_op = SDL_GPU_STOREOP_DONT_CARE;
        depth_target.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depth_target.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

        SDL_GPURenderPass *render_pass =
            SDL_BeginGPURenderPass(commands, &color_target, 1, &depth_target);
        if (render_pass == nullptr) {
            const std::string error = SDL_GetError();
            SDL_SubmitGPUCommandBuffer(commands);
            throw std::runtime_error("could not begin GPU render pass: " + error);
        }

        SDL_BindGPUGraphicsPipeline(render_pass, floor_pipeline_.get());
        SDL_DrawGPUPrimitives(render_pass, 6, 1, 0, 0);
        SDL_BindGPUGraphicsPipeline(render_pass, capsule_pipeline_.get());
        SDL_DrawGPUPrimitives(render_pass, capsule_vertex_count, 1, 0, 0);
        SDL_EndGPURenderPass(render_pass);

        if (!SDL_SubmitGPUCommandBuffer(commands)) {
            sdl_fail("could not submit GPU command buffer");
        }
    }

  private:
    void ensure_depth_texture(std::uint32_t width, std::uint32_t height) {
        if (depth_texture_ != nullptr && width == depth_width_ && height == depth_height_) {
            return;
        }

        const SDL_GPUTextureCreateInfo create_info{
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = depth_format,
            .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
            .width = width,
            .height = height,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
        };
        GpuTexture texture{SDL_CreateGPUTexture(device_.get(), &create_info),
                           TextureDeleter{device_.get()}};
        if (texture == nullptr) {
            sdl_fail("could not create depth texture");
        }

        depth_texture_ = std::move(texture);
        depth_width_ = width;
        depth_height_ = height;
        camera_uniforms_ =
            create_camera_uniforms(static_cast<float>(width) / static_cast<float>(height));
    }

    GpuDevice device_;
    Window window_;
    ClaimedGpuWindow claim_;
    GraphicsPipeline floor_pipeline_;
    GraphicsPipeline capsule_pipeline_;
    GpuTexture depth_texture_;
    std::uint32_t depth_width_ = 0;
    std::uint32_t depth_height_ = 0;
    CameraUniforms camera_uniforms_;
};

GpuWindow::GpuWindow() : impl_(std::make_unique<Impl>()) {}

GpuWindow::~GpuWindow() = default;

void GpuWindow::render(Color clear_color) { impl_->render(clear_color); }

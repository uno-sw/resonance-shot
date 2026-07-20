#include "GpuWindow.hpp"
#include "FloorShader.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

struct Vector3 {
    float x;
    float y;
    float z;
};

struct Matrix4 {
    std::array<float, 16> elements{};

    float &at(std::size_t column, std::size_t row) { return elements[column * 4 + row]; }
    float at(std::size_t column, std::size_t row) const { return elements[column * 4 + row]; }
};

struct alignas(16) CameraUniforms {
    std::array<float, 16> view_projection;
};

static_assert(sizeof(CameraUniforms) == 64);
static_assert(alignof(CameraUniforms) == 16);

float dot(Vector3 left, Vector3 right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vector3 cross(Vector3 left, Vector3 right) {
    return {
        .x = left.y * right.z - left.z * right.y,
        .y = left.z * right.x - left.x * right.z,
        .z = left.x * right.y - left.y * right.x,
    };
}

Vector3 normalize(Vector3 vector) {
    const float inverse_length = 1.0F / std::sqrt(dot(vector, vector));
    return {
        .x = vector.x * inverse_length,
        .y = vector.y * inverse_length,
        .z = vector.z * inverse_length,
    };
}

Matrix4 multiply(const Matrix4 &left, const Matrix4 &right) {
    Matrix4 result;
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t index = 0; index < 4; ++index) {
                result.at(column, row) += left.at(index, row) * right.at(column, index);
            }
        }
    }
    return result;
}

Matrix4 look_at(Vector3 eye, Vector3 target, Vector3 up) {
    const Vector3 forward = normalize({
        .x = target.x - eye.x,
        .y = target.y - eye.y,
        .z = target.z - eye.z,
    });
    const Vector3 right = normalize(cross(forward, up));
    const Vector3 camera_up = cross(right, forward);

    Matrix4 result;
    result.at(0, 0) = right.x;
    result.at(0, 1) = camera_up.x;
    result.at(0, 2) = -forward.x;
    result.at(1, 0) = right.y;
    result.at(1, 1) = camera_up.y;
    result.at(1, 2) = -forward.y;
    result.at(2, 0) = right.z;
    result.at(2, 1) = camera_up.z;
    result.at(2, 2) = -forward.z;
    result.at(3, 0) = -dot(right, eye);
    result.at(3, 1) = -dot(camera_up, eye);
    result.at(3, 2) = dot(forward, eye);
    result.at(3, 3) = 1.0F;
    return result;
}

Matrix4 perspective(float vertical_field_of_view, float aspect_ratio, float near_plane,
                    float far_plane) {
    const float vertical_scale = 1.0F / std::tan(vertical_field_of_view * 0.5F);

    Matrix4 result;
    result.at(0, 0) = vertical_scale / aspect_ratio;
    result.at(1, 1) = vertical_scale;
    result.at(2, 2) = far_plane / (near_plane - far_plane);
    result.at(2, 3) = -1.0F;
    result.at(3, 2) = near_plane * far_plane / (near_plane - far_plane);
    return result;
}

CameraUniforms create_camera_uniforms() {
    constexpr float pi = 3.14159265358979323846F;
    const Matrix4 view =
        look_at({.x = 0.0F, .y = 6.0F, .z = 8.0F}, {.x = 0.0F, .y = 0.0F, .z = -4.0F},
                {.x = 0.0F, .y = 1.0F, .z = 0.0F});
    const Matrix4 projection = perspective(pi / 3.0F, 16.0F / 9.0F, 0.1F, 100.0F);
    return {.view_projection = multiply(projection, view).elements};
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

GraphicsPipeline create_graphics_pipeline(SDL_GPUDevice *device, SDL_Window *window) {
    GpuShader vertex_shader = create_shader(device, SDL_GPU_SHADERSTAGE_VERTEX, "floor_vertex");
    GpuShader fragment_shader =
        create_shader(device, SDL_GPU_SHADERSTAGE_FRAGMENT, "floor_fragment");

    const SDL_GPUColorTargetDescription color_target{
        .format = SDL_GetGPUSwapchainTextureFormat(device, window),
    };
    const SDL_GPUGraphicsPipelineCreateInfo create_info{
        .vertex_shader = vertex_shader.get(),
        .fragment_shader = fragment_shader.get(),
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .target_info =
            {
                .color_target_descriptions = &color_target,
                .num_color_targets = 1,
            },
    };

    GraphicsPipeline pipeline{SDL_CreateGPUGraphicsPipeline(device, &create_info),
                              GraphicsPipelineDeleter{device}};

    if (pipeline == nullptr) {
        sdl_fail("could not create floor graphics pipeline");
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
          pipeline_(create_graphics_pipeline(device_.get(), window_.get())),
          camera_uniforms_(create_camera_uniforms()) {}

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

        SDL_GPURenderPass *render_pass =
            SDL_BeginGPURenderPass(commands, &color_target, 1, nullptr);
        if (render_pass == nullptr) {
            const std::string error = SDL_GetError();
            SDL_SubmitGPUCommandBuffer(commands);
            throw std::runtime_error("could not begin GPU render pass: " + error);
        }

        SDL_BindGPUGraphicsPipeline(render_pass, pipeline_.get());
        SDL_DrawGPUPrimitives(render_pass, 6, 1, 0, 0);
        SDL_EndGPURenderPass(render_pass);

        if (!SDL_SubmitGPUCommandBuffer(commands)) {
            sdl_fail("could not submit GPU command buffer");
        }
    }

  private:
    GpuDevice device_;
    Window window_;
    ClaimedGpuWindow claim_;
    GraphicsPipeline pipeline_;
    CameraUniforms camera_uniforms_;
};

GpuWindow::GpuWindow() : impl_(std::make_unique<Impl>()) {}

GpuWindow::~GpuWindow() = default;

void GpuWindow::render(Color clear_color) { impl_->render(clear_color); }

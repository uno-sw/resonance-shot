#include "GpuWindow.hpp"
#include "SdlRuntime.hpp"

#include <SDL3/SDL.h>

#include <exception>
#include <iostream>

namespace {

int run() {
    const SdlRuntime sdl;
    GpuWindow window;
    constexpr SceneLighting lighting{
        .directional =
            {
                .direction = {.x = -0.45F, .y = -1.0F, .z = -0.35F},
                .color = {.red = 0.75F, .green = 0.75F, .blue = 0.75F},
            },
        .ambient = {.color = {.red = 0.25F, .green = 0.25F, .blue = 0.25F}},
    };

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                running = false;
            }
        }

        if (running) {
            window.render(
                {
                    .red = 0.05F,
                    .green = 0.10F,
                    .blue = 0.20F,
                    .alpha = 1.0F,
                },
                lighting);
        }
    }

    return 0;
}

} // namespace

int main() {
    try {
        return run();
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}

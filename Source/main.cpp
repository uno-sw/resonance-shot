#include <SDL3/SDL.h>

#include <iostream>

namespace {

[[noreturn]] void sdl_fail(const std::string &message) {
    throw std::runtime_error(message + ": " + SDL_GetError());
}

int run() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        sdl_fail("could not initialize SDL video");
    }

    SDL_Window *window = SDL_CreateWindow("Resonance Shot", 1280, 720, 0);
    if (window == nullptr) {
        SDL_Quit();
        sdl_fail("could not create window");
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                running = false;
            }
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
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

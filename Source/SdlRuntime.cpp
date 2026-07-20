#include "SdlRuntime.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>

SdlRuntime::SdlRuntime() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("could not initialize SDL video: ") + SDL_GetError());
    }
}

SdlRuntime::~SdlRuntime() { SDL_Quit(); }

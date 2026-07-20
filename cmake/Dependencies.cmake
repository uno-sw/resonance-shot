include(FetchContent)

set(RESONANCE_SDL_VERSION "3.4.12")
set(RESONANCE_SDL_COMMIT "f87239e71e42da91ca317a12eefb82cfbf3393eb")

# Build SDL into the executable so development and distribution do not depend on an externally
# installed SDL dylib.
set(SDL_SHARED
    OFF
    CACHE BOOL "Build SDL as a shared library" FORCE)
set(SDL_STATIC
    ON
    CACHE BOOL "Build SDL as a static library" FORCE)
set(SDL_TESTS
    OFF
    CACHE BOOL "Build the SDL test programs" FORCE)
set(SDL_TEST_LIBRARY
    OFF
    CACHE BOOL "Build the SDL test library" FORCE)
set(SDL_EXAMPLES
    OFF
    CACHE BOOL "Build the SDL example programs" FORCE)
set(SDL_DISABLE_INSTALL
    ON
    CACHE BOOL "Disable SDL install targets" FORCE)

FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG ${RESONANCE_SDL_COMMIT}
  GIT_PROGRESS TRUE)

FetchContent_MakeAvailable(SDL3)

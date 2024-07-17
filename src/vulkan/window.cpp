#include "window.h"

#include <SDL.h>
#include <SDL_video.h>

namespace mb {

/**
 * @brief creates an SDL2 window with vulkan properties
 * 
 * @param extent : the dimensions of the window
 */
void Window::init(VkExtent2D extent) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  instance = SDL_CreateWindow(
    "Manaburn",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    extent.width,
    extent.height,
    windowFlags
  );
}



}
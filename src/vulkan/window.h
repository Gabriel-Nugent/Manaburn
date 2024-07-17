#pragma once

#include <SDL2/SDL.h>
#include <SDL_video.h>
#include <vulkan/vulkan_core.h>

namespace mb {

/**
 * @brief Window controller for the engine
 * 
 */
class Window {
public:
  Window(){}
  ~Window(){
    SDL_DestroyWindow(instance);
  }

  void init(VkExtent2D extent);

  SDL_Window* instance = nullptr;
  
private:


};

}
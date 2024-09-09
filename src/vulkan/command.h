#pragma once

#include <vulkan/vulkan_core.h>

namespace mb {

class Command {
public:
  Command() {init();}
  ~Command();

  VkCommandPool pool;
  VkCommandBuffer buffer;
private:
  void init();

  void createCommandPool();
  void allocateCommandBuffer();
};

}
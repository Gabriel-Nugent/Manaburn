#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>

#include "device.h"

namespace mb {

class Command {
public:
  Command(const std::shared_ptr<Device> _device) : device(_device) {init();}
  ~Command();

  VkCommandPool pool;
  VkCommandBuffer buffer;
private:
  const std::shared_ptr<Device> device;

  void init();

  void createCommandPool();
  void allocateCommandBuffer();
};

}
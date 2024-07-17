#define SDL_MAIN_HANDLED

#include "engine.h"

int main(int argc, char* argv[]) {
  mb::Engine engine;

  engine.init();
  engine.run();

  return 0;
}
#include "live.hh"
#include "utils.hh"
#include <SDL2/SDL.h>

void live(const std::string &definition) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    die("Failed to initialize SDL");

  int window_width = 600, window_height = (3. / 4.) * (double)window_width;
  SDL_Window *window = SDL_CreateWindow("Sythin live", SDL_WINDOWPOS_CENTERED
      , SDL_WINDOWPOS_CENTERED, window_width, window_height, 0);
  if (!window)
    die("failed to create window");

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_SetRenderDrawColor(renderer, 19, 19, 19, 255);

  bool quit = false;
  while (!quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event))
      if (event.type == SDL_QUIT
          || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE))
        quit = true;

    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  // SDL_CloseAudio();
  SDL_Quit();
}


#include "gfx.hh"
#include "imgui.hh"
#include "utils.hh"
#define GLEW_STATIC
#include <GL/glew.h>
#include "../thirdparty/imgui/imgui.h"

static SDL_Window *window;
static SDL_GLContext glcontext;

void gfx_init(const char *title, int width, int height) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    die("Failed to initialize SDL: %s", SDL_GetError());

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);

  window = SDL_CreateWindow("Sythin", SDL_WINDOWPOS_CENTERED
      , SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
  if (!window)
    die("failed to create window");

  glcontext = SDL_GL_CreateContext(window);

  if (SDL_GL_SetSwapInterval(0) == -1)
    printf("warning: failed to set vsync: %s\n", SDL_GetError());

  GLenum err = glewInit();
  if (err != GLEW_OK)
    die("failed to initialze glew: %s", glewGetErrorString(err));

  imgui_init(window);
}

void gfx_main_loop(bool *done
    , void (*init_cb)(void)
    , void (*frame_cb)(void)
    , void (*update_cb)(double, double)
    , void (*key_event_cb)(unsigned long long, bool)
    , void (*destroy_cb)(void)) {
  init_cb();

  double t = 0, dt = 1. / 60., current_time = SDL_GetTicks() / 1000.
    , accumulator = 0;

  while (!*done) {
    double real_time = SDL_GetTicks() / 1000.
      , elapsed = real_time - current_time;
    current_time = real_time;
    accumulator += elapsed;

    while (accumulator >= dt) {
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT)
          *done = true;
        else if ((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
            && event.key.repeat == 0)
          key_event_cb(event.key.keysym.sym, event.type == SDL_KEYDOWN);
        imgui_process_event(&event);
      }
      update_cb(dt, t);
      t += dt;
      accumulator -= dt;
    }
    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x
        , (int)ImGui::GetIO().DisplaySize.y);
    imgui_new_frame(window);
    frame_cb();
    ImGui::Render();
    SDL_GL_SwapWindow(window);
  }

  destroy_cb();

  imgui_destroy();
  SDL_GL_DeleteContext(glcontext);
  SDL_DestroyWindow(window);
  SDL_Quit();
}


#include "live.hh"
#include "utils.hh"
#include <SDL2/SDL.h>

static double note_to_freq(char note, int octave, int accidental_offset) {
  int octave_diff = octave - 4, semitone_diff = note - 'a';
  return 440. * pow(2., octave_diff) * pow(2., semitone_diff / 12.);
}

static void audio_callback(void *userdata, uint8_t *stream, int len) {
  static int t = 0;
  float seconds_per_sample = 1.f / 48000.f;
  float *stream_ptr = (float*)stream;
  for (int i = 0; i < 4096; ++i) {
    float value = 0.2f * sinf(2.f * 3.141592f * 440.f * (float)(t++) * seconds_per_sample);
    *stream_ptr = value;
    ++stream_ptr;
  }
}

void live(const term_t *const program, const std::string &definition) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0)
    die("Failed to initialize SDL");

  int window_width = 600, window_height = (3. / 4.) * (double)window_width;
  SDL_Window *window = SDL_CreateWindow("Sythin live", SDL_WINDOWPOS_CENTERED
      , SDL_WINDOWPOS_CENTERED, window_width, window_height, 0);
  if (!window)
    die("failed to create window");

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_SetRenderDrawColor(renderer, 19, 19, 19, 255);

  SDL_AudioSpec want, have;
  SDL_memset(&want, 0, sizeof(want));
  want.freq = 48000;
  want.format = AUDIO_F32;
  want.channels = 1;
  want.samples = 4096;
  want.callback = audio_callback;

  SDL_AudioDeviceID dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have
      , SDL_AUDIO_ALLOW_ANY_CHANGE);
  if (dev == 0)
    die("failed to open audio device: %s", SDL_GetError());

  SDL_PauseAudioDevice(dev, 0);

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
  SDL_CloseAudioDevice(dev);
  SDL_Quit();
}


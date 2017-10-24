#include "eval.hh"
#include "live.hh"
#include "utils.hh"
#include <SDL2/SDL.h>

struct passed_data_t {
  term_t *const program;
  const std::string definition;
  std::vector<double> frequencies;
  passed_data_t(term_t *const program, const std::string &definition)
    : program(program)
    , definition(definition) {
  }
};

static double note_to_freq(char note, int octave, int accidental_offset) {
  const std::map<char, int> semitone_offset = {
    { 'C', -9 },
    { 'D', -7 },
    { 'E', -5 },
    { 'F', -4 },
    { 'G', -2 },
    { 'A',  0 },
    { 'B',  2 }
  };
  int octave_diff = octave - 4, semitone_diff = semitone_offset.at(note)
    + accidental_offset;
  return 440. * pow(2., octave_diff) * pow(2., semitone_diff / 12.);
}

static void audio_callback(void *userdata, uint8_t *stream, int len) {
  passed_data_t *passed_data = (passed_data_t*)userdata;
  static uint64_t c = 0;
  float *stream_ptr = (float*)stream;
  for (int i = 0; i < 4096; ++i) {
    double t = (double)(c++) * 1. / 48000.;
    *stream_ptr = 0;
    for (double f : passed_data->frequencies) {
      printf("f=%f\n", f);
      float value = (float)evaluate_definition(passed_data->program
          , passed_data->definition, f, t);
      *stream_ptr += 0.2f * value;
    }
    ++stream_ptr;
  }
}

void live(term_t *const program, const std::string &definition) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0)
    die("Failed to initialize SDL");

  int window_width = 600, window_height = (3. / 4.) * (double)window_width;
  SDL_Window *window = SDL_CreateWindow("Sythin live", SDL_WINDOWPOS_CENTERED
      , SDL_WINDOWPOS_CENTERED, window_width, window_height, 0);
  if (!window)
    die("failed to create window");

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_SetRenderDrawColor(renderer, 19, 19, 19, 255);

  passed_data_t passed_data(program, definition);

  SDL_AudioSpec want, have;
  SDL_memset(&want, 0, sizeof(want));
  want.freq = 48000;
  want.format = AUDIO_F32;
  want.channels = 1;
  want.samples = 4096;
  want.callback = audio_callback;
  want.userdata = (void*)&passed_data;

  SDL_AudioDeviceID dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have
      , SDL_AUDIO_ALLOW_ANY_CHANGE);
  if (dev == 0)
    die("failed to open audio device: %s", SDL_GetError());

  SDL_PauseAudioDevice(dev, 0);

  std::map<int, double> key_frequencies = {
    { SDLK_z, note_to_freq('A', 4, 0) },
    { SDLK_x, note_to_freq('A', 4, 1) },
    { SDLK_c, note_to_freq('B', 4, 0) },
    { SDLK_v, note_to_freq('C', 5, 0) }
  };

  bool quit = false;
  while (!quit) {
    passed_data.frequencies.clear();

    SDL_Event event;
    while (SDL_PollEvent(&event))
      if (event.type == SDL_QUIT
          || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE))
        quit = true;
      else if (event.type == SDL_KEYDOWN)
        if (key_frequencies.count(event.key.keysym.sym)) {
          printf("set %f\n", key_frequencies[event.key.keysym.sym]);
          passed_data.frequencies.push_back(key_frequencies[event.key.keysym.sym]);
        }

    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_CloseAudioDevice(dev);
  SDL_Quit();
}


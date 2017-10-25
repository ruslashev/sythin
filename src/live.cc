#include "eval.hh"
#include "live.hh"
#include "utils.hh"
#include <SDL2/SDL.h>

struct passed_data_t {
  struct frequency_data_t {
    bool on;
    uint64_t c;
    frequency_data_t() : c(0) {}
  };
  term_t *const program;
  const std::string definition;
  std::map<double, frequency_data_t> frequencies; // _very_ sloppy but works
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
  float *stream_ptr = (float*)stream;
  for (int i = 0; i < 4096; ++i) {
    *stream_ptr = 0;
    for (auto &freq_pair : passed_data->frequencies) {
      if (freq_pair.second.on) {
        double t = (double)(freq_pair.second.c++) * 1. / 48000.;
        float value = (float)evaluate_definition(passed_data->program
            , passed_data->definition, freq_pair.first, t);
        *stream_ptr += 0.2f * value;
      }
    }
    ++stream_ptr;
  }
}

void live(term_t *const program, const std::string &definition) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
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

  int octave = 4;
  std::map<int, std::pair<char, int>> key_notes = {
    { SDLK_a, { 'C', 0 } },
    { SDLK_w, { 'C', 1 } },
    { SDLK_s, { 'D', 0 } },
    { SDLK_e, { 'D', 1 } },
    { SDLK_d, { 'E', 0 } },
    { SDLK_f, { 'F', 0 } },
    { SDLK_t, { 'F', 1 } },
    { SDLK_g, { 'G', 0 } },
    { SDLK_y, { 'G', 1 } },
    { SDLK_h, { 'A', 0 } },
    { SDLK_u, { 'A', 1 } },
    { SDLK_j, { 'B', 0 } }
  };

  bool quit = false;
  while (!quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event))
      if (event.type == SDL_QUIT
          || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE))
        quit = true;
      else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        const int key = event.key.keysym.sym;
        if (key_notes.count(key)) {
          const std::pair<char, int> note = key_notes[key];
          passed_data.frequencies[
            note_to_freq(note.first, octave, note.second)].on
            = event.type == SDL_KEYDOWN;
          if (event.type == SDL_KEYUP)
            passed_data.frequencies[
              note_to_freq(note.first, octave, note.second)].c = 0;
        }
        if (key >= SDLK_0 && key <= SDLK_9)
          octave = key - SDLK_0;
      }

    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_CloseAudioDevice(dev);
  SDL_Quit();
}


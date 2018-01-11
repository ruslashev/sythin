#include "eval.hh"
#include "live.hh"
#include "utils.hh"
#include "gfx.hh"
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "imgui.hh"
#include "../thirdparty/imgui.h"

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
  /* TODO static */ const std::map<char, int> semitone_offset = {
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
    for (auto &freq_pair : passed_data->frequencies)
      if (freq_pair.second.on) {
        double t = (double)(freq_pair.second.c++) * 1. / 48000.;
        float value = (float)evaluate_definition(passed_data->program
            , passed_data->definition, freq_pair.first, t);
        *stream_ptr += 0.2f * value;
      }
    ++stream_ptr;
  }
}

static bool done = false;
static SDL_AudioDeviceID dev = 0;
static passed_data_t *passed_data = nullptr;
static int octave = 4;
static const std::map<int, std::pair<char, int>> key_notes = {
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

static void init() {
  SDL_AudioSpec want, have;
  SDL_memset(&want, 0, sizeof(want));
  want.freq = 48000;
  want.format = AUDIO_F32;
  want.channels = 1;
  want.samples = 4096;
  want.callback = audio_callback;
  want.userdata = (void*)passed_data;
  dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_ANY_CHANGE);
  if (dev == 0)
    die("failed to open audio device: %s", SDL_GetError());
  SDL_PauseAudioDevice(dev, 0);

  float s = 20.f / 255.f;
  glClearColor(s, s, s, 1.f);
}

static void update(double dt, double t) {
}

static void draw_gui() {
  if (ImGui::BeginMainMenuBar()) {
    // ImGui::PushStyleColor(ImGuiCol_Button,        r2v( 67,  96, 123));
    // ImGui::PushStyleColor(ImGuiCol_ButtonHovered, r2v( 75, 108, 138));
    // ImGui::PushStyleColor(ImGuiCol_ButtonActive,  r2v( 83, 119, 153));

    ImGui::Text("Sythin");

    ImGui::EndMainMenuBar();
  }

  float padding = 5;
  ImGui::SetNextWindowPos(ImVec2(padding, 25 + padding), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x / 2.f - padding * 2
        , ImGui::GetIO().DisplaySize.y - 25 - padding * 2), ImGuiCond_Always);
  ImGui::Begin("Source code", nullptr, ImGuiWindowFlags_NoResize
      | ImGuiWindowFlags_NoMove
      | ImGuiWindowFlags_NoCollapse);

  static char text[1024*16] =
    "/*\n"
    " The Pentium F00F bug, shorthand for F0 0F C7 C8,\n"
    " the hexadecimal encoding of one offending instruction,\n"
    " more formally, the invalid operand with locked CMPXCHG8B\n"
    " instruction bug, is a design flaw in the majority of\n"
    " Intel Pentium, Pentium MMX, and Pentium OverDrive\n"
    " processors (all in the P5 microarchitecture).\n"
    "*/\n\n"
    "label:\n"
    "\tlock cmpxchg8b eax\n";
  ImGui::InputTextMultiline("##source", text, IM_ARRAYSIZE(text)
      , ImVec2(-1.0f, ImGui::GetTextLineHeight() * 16)
      , ImGuiInputTextFlags_AllowTabInput);

  ImGui::ShowTestWindow(nullptr);

  ImGui::End();
}

static void frame() {
  glClear(GL_COLOR_BUFFER_BIT);
  draw_gui();
}

static void key_event(unsigned long long key, bool down) {
  if (!down && key == SDLK_ESCAPE)
    done = true;

  if (key_notes.count(key)) {
    const std::pair<char, int> note = key_notes.at(key);
    double freq = note_to_freq(note.first, octave, note.second);
    passed_data->frequencies[freq].on = down;
    if (!down)
      passed_data->frequencies[freq].c = 0;
  }
  if (key >= SDLK_0 && key <= SDLK_9)
    octave = key - SDLK_0;
}

static void destroy() {
  SDL_CloseAudioDevice(dev);
}

void live(term_t *const program, const std::string &definition) {
  passed_data = new passed_data_t(program, definition);

  int window_width = 1200, window_height = (3. / 4.) * (double)window_width + 0.5;
  gfx_init("Sythin", window_width, window_height);

  gfx_main_loop(&done, init, frame, update, key_event, destroy);
}


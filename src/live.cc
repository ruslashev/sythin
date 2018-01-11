#include "eval.hh"
#include "live.hh"
#include "utils.hh"
#include "gfx.hh"
#include "lex.hh"
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "imgui.hh"
#include "../thirdparty/imgui/imgui.h"

struct passed_data_t {
  struct frequency_data_t {
    bool on;
    uint64_t c;
    frequency_data_t() : c(0) {}
  };
  term_t *program;
  const std::string definition;
  std::map<double, frequency_data_t> frequencies; // _very_ sloppy but works
  passed_data_t(const std::string &definition)
    : program(nullptr)
    , definition(definition) {
  }
};

static bool g_done = false;
static SDL_AudioDeviceID g_dev = 0;
static std::string g_filename = "";
static char g_source[100000]; // stupid
static passed_data_t *g_passed_data = nullptr;
static std::vector<message_t> g_messages;
static int g_octave = 4;
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

static void init() {
  SDL_AudioSpec want, have;
  SDL_memset(&want, 0, sizeof(want));
  want.freq = 48000;
  want.format = AUDIO_F32;
  want.channels = 1;
  want.samples = 4096;
  want.callback = audio_callback;
  want.userdata = (void*)g_passed_data;
  g_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_ANY_CHANGE);
  if (g_dev == 0)
    die("failed to open audio device: %s", SDL_GetError());
  SDL_PauseAudioDevice(g_dev, 0);

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

    ImGui::Text("%s - sythin", g_filename.c_str());

    ImGui::EndMainMenuBar();
  }

  float padding = 5;
  ImGui::SetNextWindowPos(ImVec2(padding, 25 + padding), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x * (2.f / 5.f) - padding * 2
        , ImGui::GetIO().DisplaySize.y - 25 - padding * 2), ImGuiCond_Always);
  ImGui::Begin("Source code", nullptr, ImGuiWindowFlags_NoResize
      | ImGuiWindowFlags_NoMove
      | ImGuiWindowFlags_NoTitleBar
      | ImGuiWindowFlags_NoCollapse);

  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
  ImGui::InputTextMultiline("##source", g_source, sizeof(g_source)
      , ImVec2(-1.f, -1.f)
      , ImGuiInputTextFlags_AllowTabInput);
  ImGui::PopFont();

  ImGui::ShowTestWindow(nullptr);

  ImGui::End();
}

static void frame() {
  glClear(GL_COLOR_BUFFER_BIT);
  draw_gui();
}

static void key_event(unsigned long long key, bool down) {
  if (!down && key == SDLK_ESCAPE)
    g_done = true;

  if (key_notes.count(key)) {
    const std::pair<char, int> note = key_notes.at(key);
    double freq = note_to_freq(note.first, g_octave, note.second);
    g_passed_data->frequencies[freq].on = down;
    if (!down)
      g_passed_data->frequencies[freq].c = 0;
  }
  if (key >= SDLK_0 && key <= SDLK_9)
    g_octave = key - SDLK_0;
}

static void destroy() {
  SDL_CloseAudioDevice(g_dev);
}

void reload_file() {
  if (g_passed_data->program)
    delete g_passed_data->program;
  g_messages.clear();

  std::string source = read_file("test.sth");
  strncpy(g_source, source.c_str(), sizeof(g_source));

  g_passed_data->program = lex_parse_string(g_source);
  if (!g_passed_data->program)
    exit(1);

  validate_top_level_functions(g_passed_data->program, &g_messages);
  // for (const message_t &message : messages)
  //   printf("%s: %s\n", message_kind_to_string(message.kind).c_str()
  //       , message.content.c_str());
}

void live(std::string _filename, const std::string &definition) {
  g_filename = _filename;

  g_passed_data = new passed_data_t(definition);
  reload_file();

  int window_width = 1200, window_height = (3. / 4.) * (double)window_width + 0.5;
  gfx_init("Sythin", window_width, window_height);

  gfx_main_loop(&g_done, init, frame, update, key_event, destroy);

  if (g_passed_data->program)
    delete g_passed_data->program;
}


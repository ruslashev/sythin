#include "eval.hh"
#include "live.hh"
#include "utils.hh"
#include "gfx.hh"
#include "lex.hh"
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "imgui.hh"
#include "../thirdparty/imgui/imgui.h"
#include <thread>

void replot();
void recalculate_freq_to_note();
void compute();

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

static const float sample_rate = 48000;
static const int num_computed_seconds = 4
    , num_computed_samples = sample_rate * num_computed_seconds + 0.5f;
static const int note_idx_to_char[] = { 'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G',
  'G', 'A', 'A', 'B' }
  , note_idx_to_accidental[] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, };
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

static bool g_done = false;
static SDL_AudioDeviceID g_dev = 0;
static std::string g_filename = "";
static char g_source[100000]; // stupid
static passed_data_t *g_passed_data = nullptr;
static std::vector<message_t> g_messages;
static std::vector<float> g_samples;
static float g_frequency = 55.f /* A1 */, g_seconds = 1;
static std::string g_frequency_to_note = "";
static int g_octave = 4;
static bool playing = true, computed = false, unsaved = false;
static float computed_samples[10][12][num_computed_samples];
static double computation_progress = 0;
std::thread computation_thread;

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
        float t = (float)(freq_pair.second.c++) * 1.f / sample_rate
          , value = (float)evaluate_definition(passed_data->program
              , passed_data->definition, freq_pair.first, t);
        *stream_ptr += 0.2f * value;
      }
    ++stream_ptr;
  }
}

static ImVec4 r2v(int r, int g, int b) {
  return ImVec4((float)r / 255.f, (float)g / 255.f, (float)b / 255.f, 1.f);
}

static void init() {
  SDL_AudioSpec want, have;
  SDL_memset(&want, 0, sizeof(want));
  want.freq = sample_rate;
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

  ImGui::PushStyleColor(ImGuiCol_FrameBg,       r2v( 45,  45,  45));
  ImGui::PushStyleColor(ImGuiCol_WindowBg,      r2v(  7,   7,   7));
  ImGui::PushStyleColor(ImGuiCol_MenuBarBg,     r2v(147,  77,  77));
  ImGui::PushStyleColor(ImGuiCol_Button,        r2v(186,  85,  85));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, r2v(183, 106, 106));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive,  r2v(159,  80,  80));

  unsigned long long b = sizeof(computed_samples), kb = b / 1024, mb = kb / 1024;
  printf("sizeof(computed_samples): b=%llu, kb=%llu, mb=%llu\n", b, kb, mb);
}

static void update(double dt, double t) {
}

static void draw_gui() {
  if (ImGui::BeginMainMenuBar()) {
    ImGui::Text("%s%s - sythin", g_filename.c_str(), unsaved ? "*" : "");
    ImGui::EndMainMenuBar();
  }

  const float padding = 5, yoff = 25;

  ImGuiIO& io = ImGui::GetIO();

  ImGui::SetNextWindowPos(ImVec2(padding, yoff + padding), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * (1.f / 2.f)
        - padding * 1.5f, io.DisplaySize.y - yoff - padding * 2)
      , ImGuiCond_Always);

  ImGui::Begin("Source code", nullptr, ImGuiWindowFlags_NoResize
      | ImGuiWindowFlags_NoMove
      | ImGuiWindowFlags_NoTitleBar
      | ImGuiWindowFlags_NoCollapse);

  if (ImGui::Button("Save")) {
  }
  ImGui::SameLine();
  if (ImGui::Button("Compute")) {
    computation_thread = std::thread(compute);
  }
  if (!computed) {
    ImGui::SameLine();
    ImGui::TextWrapped("Warning: code is not computed, all sounds will be "
        "interpreted on the fly");
  }
  ImGui::ProgressBar(computation_progress, ImVec2(0, 0));

  ImGui::PushFont(io.Fonts->Fonts[1]);
  if (playing)
    ImGui::PushStyleColor(ImGuiCol_FrameBg, r2v(20, 20, 20));
  unsaved |= ImGui::InputTextMultiline("##source", g_source, sizeof(g_source)
      , ImVec2(-1.f, -1.f), 0);
  if (playing)
    ImGui::PopStyleColor();
  playing = !ImGui::IsItemActive();
  ImGui::PopFont();

  ImGui::ShowTestWindow();

  ImGui::End();

  ImGui::SetNextWindowPos(ImVec2(padding * 0.5f + io.DisplaySize.x
        * (1.f / 2.f), yoff + padding), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * (1.f / 2.f)
        - padding * 1.5f, io.DisplaySize.y - yoff - padding * 2)
      , ImGuiCond_Always);

  ImGui::Begin("Graph", nullptr, ImGuiWindowFlags_NoResize
      | ImGuiWindowFlags_NoMove
      | ImGuiWindowFlags_NoTitleBar
      | ImGuiWindowFlags_NoCollapse);

  ImGui::Text("Time");
  ImGui::SameLine(80);
  ImGui::SliderFloat("seconds", &g_seconds, 0.1f, 5.0f, "%.3f");

  ImGui::Text("Frequency");

  ImGui::SameLine(80);
  if (ImGui::SliderFloat("Hz", &g_frequency, 16.35f, 1975.53f, "%.3f", 2.f))
    recalculate_freq_to_note();

  ImGui::SameLine();
  ImGui::Text("%s", g_frequency_to_note.c_str());

  if (ImGui::Button("Replot"))
    replot();
  ImGui::SameLine();
  if (ImGui::Button("Frequency presets"))
    ImGui::OpenPopup("Frequency presets");
  ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * (1.f / 4.f), io.DisplaySize.x * (131.f / 500.f))
      , ImGuiCond_Always);
  if (ImGui::BeginPopupModal("Frequency presets", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Columns(7, "presetz", false);
    for (int o = 0; o <= 6; ++o) {
      std::string C = "C" + std::to_string(o), Cs = "C#" + std::to_string(o),
        D = "D" + std::to_string(o), Ds = "D#" + std::to_string(o),
        E = "E" + std::to_string(o), F = "F" + std::to_string(o),
        Fs = "F#" + std::to_string(o), G = "G" + std::to_string(o),
        Gs = "G#" + std::to_string(o), A = "A" + std::to_string(o),
        As = "A#" + std::to_string(o), B = "B" + std::to_string(o);
#define sel ImGui::Selectable
#define rec recalculate_freq_to_note
      if (sel(C.c_str()))  { g_frequency = note_to_freq('C', o, 0); rec(); }
      if (sel(Cs.c_str())) { g_frequency = note_to_freq('C', o, 1); rec(); }
      if (sel(D.c_str()))  { g_frequency = note_to_freq('D', o, 0); rec(); }
      if (sel(Ds.c_str())) { g_frequency = note_to_freq('D', o, 1); rec(); }
      if (sel(E.c_str()))  { g_frequency = note_to_freq('E', o, 0); rec(); }
      if (sel(F.c_str()))  { g_frequency = note_to_freq('F', o, 0); rec(); }
      if (sel(Fs.c_str())) { g_frequency = note_to_freq('F', o, 1); rec(); }
      if (sel(G.c_str()))  { g_frequency = note_to_freq('G', o, 0); rec(); }
      if (sel(Gs.c_str())) { g_frequency = note_to_freq('G', o, 1); rec(); }
      if (sel(A.c_str()))  { g_frequency = note_to_freq('A', o, 0); rec(); }
      if (sel(As.c_str())) { g_frequency = note_to_freq('A', o, 1); rec(); }
      if (sel(B.c_str()))  { g_frequency = note_to_freq('B', o, 0); rec(); }
#undef sel
#undef rec
      ImGui::NextColumn();
    }
    ImGui::Columns(1);

    if (ImGui::Button("Close"))
      ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
  }

  ImGui::PlotLines("", g_samples.data()
      , g_samples.size(), 0, g_passed_data->definition.c_str(), -1.f, 1.f, ImVec2(ImGui::GetContentRegionAvailWidth(), 200));

  ImGui::End();
}

static void frame() {
  glClear(GL_COLOR_BUFFER_BIT);
  draw_gui();
}

static void key_event(unsigned long long key, bool down) {
  if (playing) {
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

  replot();
}

void replot() {
  g_samples.clear();
  const float amplitude = 32760, scale = 1.f;
  for (uint64_t i = 0; i < (uint64_t)(sample_rate * g_seconds + 0.5f); i++)
    g_samples.push_back((float)evaluate_definition(g_passed_data->program
          , g_passed_data->definition, g_frequency
          , (double)i / (double)sample_rate) * scale);

  recalculate_freq_to_note();
}

void recalculate_freq_to_note() {
  std::vector<std::string> notes = {
    "C0", "C#0", "D0", "D#0", "E0", "F0", "F#0", "G0", "G#0", "A0", "A#0", "B0",
    "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1",
    "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",
    "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
    "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
    "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5",
    "C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6", "A6", "A#6", "B6",
    "C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7", "A7", "A#7", "B7",
    "C8", "C#8", "D8", "D#8", "E8", "F8", "F#8", "G8", "G#8", "A8", "A#8", "B8",
    "C9", "C#9", "D9", "D#9", "E9", "F9", "F#9", "G9", "G#9", "A9", "A#9", "B9"
  };

  int A4_idx = 57, minus = 0, plus = 1, r_index = 0, cent_index = 0, side;
  double A4 = 440., frequency = A4, r = pow(2.0, 1.0 / 12.0)
    , cent = pow(2.0, 1.0 / 1200.0), input = g_frequency;

  if (input >= frequency) {
    while (input >= r * frequency) {
      frequency = r * frequency;
      ++r_index;
    }
    while (input > cent * frequency) {
      frequency = cent * frequency;
      ++cent_index;
    }
    if ((cent * frequency - input) < (input - frequency))
      ++cent_index;
    if (cent_index > 50) {
      ++r_index;
      cent_index = 100 - cent_index;
      side = (cent_index != 0) ? minus : plus;
    } else
      side = plus;
  } else {
    while (input <= frequency / r) {
      frequency = frequency / r;
      --r_index;
    }
    while (input < frequency / cent) {
      frequency /= cent;
      ++cent_index;
    }
    if ((input - frequency / cent) < (frequency - input))
      ++cent_index;
    if (cent_index >= 50) {
      --r_index;
      cent_index = 100 - cent_index;
      side = plus;
    } else
      side = (cent_index != 0) ? minus : plus;
  }

  g_frequency_to_note = notes[A4_idx + r_index];
  if (cent_index) {
    g_frequency_to_note += (side == plus) ? " + " : " - ";
    g_frequency_to_note += std::to_string(cent_index) + " cents";
  }
}

void compute() {
  const double progress_change = 1. / 10. / 12. / (double)num_computed_samples;
  computation_progress = 0;
  // for (int o = 0; o <= 9; ++o)
  int o = g_octave;
    for (int n = 0; n < 12; ++n) {
      float f = note_to_freq(note_idx_to_char[n], o, note_idx_to_accidental[n]);
      for (int t = 0; t < num_computed_samples; ++t) {
        computed_samples[o][n][t] = evaluate_definition(g_passed_data->program
            , g_passed_data->definition, f, t);
        computation_progress += progress_change;
      }
    }

  computed = true;
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


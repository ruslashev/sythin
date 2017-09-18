#pragma once

#include <array>
#include <vector>

typedef std::array<std::vector<uint16_t>, 2> samples_t;

void write_wav(const char *filename, int sample_rate
    , samples_t samples);


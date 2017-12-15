#pragma once

void gfx_init(const char *title, int width, int height);
void gfx_main_loop(bool *done
    , void (*init_cb)(void)
    , void (*frame_cb)(void)
    , void (*update_cb)(double, double)
    , void (*key_event_cb)(unsigned long long, bool)
    , void (*destroy_cb)(void));


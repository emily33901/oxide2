#pragma once

#include <stdbool.h>

typedef enum Oxide_InitResult {
    oxide_init_success,
    oxide_init_window_failed,
    oxide_init_gl_failed,
    oxide_init_target_failed,
} Oxide_InitResult;

extern Oxide_InitResult oxide_init(const char *target_name);
extern void             oxide_shutdown();

extern bool oxide_set_target(const char *target_name);
extern void oxide_begin_frame();
extern void oxide_end_frame();

typedef struct Oxide_WindowSize {
    int width;
    int height;
} Oxide_WindowSize;

extern Oxide_WindowSize oxide_resize();

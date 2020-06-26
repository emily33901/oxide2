#include "oxide_internal.h"

#include <stdint.h>

#define GLEW_STATIC
#include "GL/glew.h"

bool oxide_init_gl() {
    oxide_init_os_gl();

    glewInit();

    glClearColor(0, 0, 0, 0);

    return true;
}

Oxide_InitResult oxide_init(const char *target_name) {
    bool window_success = oxide_init_window();
    if (!window_success) {
        return oxide_init_window_failed;
    }
    bool gl_success = oxide_init_gl();
    if (!gl_success) {
        return oxide_init_gl_failed;
    }

    if (!oxide_set_target(target_name)) {
        return oxide_init_target_failed;
    }

    oxide_resize();

    return oxide_init_success;
}

void oxide_shutdown() {
}

void oxide_begin_frame() {
    oxide_begin_os_frame();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void oxide_end_frame() {
    oxide_end_os_frame();
}

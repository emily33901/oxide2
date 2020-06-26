#include "oxide.h"

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>

bool __stdcall consoleHandler(unsigned signal) {

    if (signal == CTRL_C_EVENT) {
        printf("Shutting down...\n");
        oxide_shutdown();
        exit(0);
    }

    return TRUE;
}

int main() {
    Oxide_InitResult res = oxide_init("Notepad");

    if (res) {
        printf("Failed to load oxide %d\n", res);
        return 1;
    }

    if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
        printf("\nERROR: Could not set control handler");
        return 1;
    }

    while (true) {
        oxide_begin_frame();
        // ...
        oxide_end_frame();
    }
}
#else
int main() {
    printf("This platform is not supported in this test right now...\n");
    return 0;
}
#endif
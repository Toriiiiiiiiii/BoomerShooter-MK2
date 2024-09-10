#include <raylib.h>

int main() {
    InitWindow(640, 480, "Hello, World!");

    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

#include <cstdio>
#include <cmath>
#include "world.hpp"
#include "sprite.hpp"

using namespace Engine;

#define WIDTH  640
#define HEIGHT 480

int main(int argc, char **argv) {
    World w = World({}, {}, {});
    w.LoadMapFile("test.map");
    //printf("here\n");
    InitWindow(WIDTH*2, HEIGHT*2, "Hello, World!");
    SetTargetFPS(60);

    Player p = Player(0.0, 0.0, 64.0, 0.0);
    p.Init(&w);

    StaticSprite s = StaticSprite("smiley.png", -350, -125, 0);
    s.Init(&w);

    while(!WindowShouldClose()) {
        p.Update(&w);
        s.Update(&w);

        BeginDrawing();
        ClearBackground(BLACK);
        w.Render3D(&p, WIDTH, HEIGHT, 2);
        s.Render(&w, &p, WIDTH, HEIGHT);

        DrawFPS(0, 0);
        //printf("%lf\n", RAD2DEG * p.angle);

        //DrawCircle(WIDTH/2, HEIGHT/2, 3, YELLOW);
        //DrawLine(WIDTH/2, HEIGHT/2, WIDTH/2 + 15 * sin(p.angle), HEIGHT/2 - 15 * cos(p.angle), WHITE);
        //for(auto& wall : w.worldWalls) {
        //    Point p1 = w.worldVertices[wall.vertexIndex1];
        //    Point p2 = w.worldVertices[wall.vertexIndex2];

        //    DrawLine(WIDTH/2 + p1.x - p.x, HEIGHT/2 - p1.y + p.y, WIDTH/2 + p2.x - p.x, HEIGHT/2 - p2.y + p.y, wall.wallColour);
        //}

        //for(auto& sector : w.worldSectors) {
        //    for(auto& entity : sector.entities) {
        //        DrawCircle(WIDTH/2 + entity->x - p.x, HEIGHT/2 - entity->y + p.y, 10, RED);
        //    }
        //}

        EndDrawing();
        p.z = w.worldSectors[p.currentSector].floorHeight + 72;
    }

    return 0;
}

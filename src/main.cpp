#include <cstdio>
#include <cmath>
#include "world.hpp"

using namespace Engine;

#define WIDTH  640
#define HEIGHT 480

#define MOVE_SPEED 10

Point lerp(Point a, Point b, double t) {
    return Point(
        (b.x - a.x) * t + (a.x),
        (b.y - a.y) * t + (a.y)
    );
}

Wall *checkCollision(World *w, Player *p, Point oldPos, Point newPos, sdword skipWall = -1) {
    Sector currentSec = w->worldSectors[p->currentSector];

    for(auto& wIndex : currentSec.wallIndices) {
        if(wIndex == skipWall) continue;
        Wall wall = w->worldWalls[wIndex];

        Point p1 = w->worldVertices[wall.vertexIndex1];
        Point p2 = w->worldVertices[wall.vertexIndex2];

        Vector2 npos = {(float)newPos.x, (float)newPos.y};
        Vector2 pos1 = {(float)p1.x, (float)p1.y};
        Vector2 pos2 = {(float)p2.x, (float)p2.y};

        for(double t = 0; t < 1; t += 0.001) {
            Point point = lerp(oldPos, newPos, t);
            npos = {(float)point.x, (float)point.y};

            bool colliding = CheckCollisionCircleLine(npos, 2, pos1, pos2);

            if(colliding) {
                if(wall.isPortal) {
                    dword sectorToCheck = wall.frontSectorIndex;
                    if(sectorToCheck == p->currentSector) sectorToCheck = wall.backSectorIndex;

                    Sector sec = w->worldSectors[sectorToCheck];
                    if(sec.floorHeight > currentSec.floorHeight + 16) return w->worldWalls.data() + wIndex;
                    if(sec.ceilHeight - currentSec.floorHeight < 85) return w->worldWalls.data() + wIndex;
                    if(sec.ceilHeight - sec.floorHeight < 85) return w->worldWalls.data() + wIndex;

                    dword cSec = p->currentSector;
                    p->currentSector = sectorToCheck;

                    Wall *res = checkCollision(w, p, oldPos, newPos, wIndex);
                    p->currentSector = cSec;

                    return res;
                }

                return w->worldWalls.data() + wIndex;
            }
        }
    }

    return NULL;
}

int main(int argc, char **argv) {
    World w = World({}, {}, {});
    w.LoadMapFile("test.map");
    //printf("here\n");
    InitWindow(WIDTH, HEIGHT, "Hello, World!");
    SetTargetFPS(30);

    Player p = Player(0.0, 0.0, 64.0, 0.0);

    while(!WindowShouldClose()) {
        if(IsKeyDown(KEY_LEFT)) {
            p.angle -= PI/90;
        } else if(IsKeyDown(KEY_RIGHT)) {
            p.angle += PI/90;
        }

        Point newPos = Point(p.x, p.y);
        if(IsKeyDown(KEY_W)) {
            newPos.x += sin(p.angle) * MOVE_SPEED;
            newPos.y += cos(p.angle) * MOVE_SPEED;
        } 
        if(IsKeyDown(KEY_S)) {
            newPos.x -= sin(p.angle) * MOVE_SPEED;
            newPos.y -= cos(p.angle) * MOVE_SPEED;
        }
        if(IsKeyDown(KEY_A)) {
            newPos.x -= cos(p.angle) * MOVE_SPEED;
            newPos.y += sin(p.angle) * MOVE_SPEED;
        }
        if(IsKeyDown(KEY_D)) {
            newPos.x += cos(p.angle) * MOVE_SPEED;
            newPos.y -= sin(p.angle) * MOVE_SPEED;
        }

        BeginDrawing();
        ClearBackground(BLACK);
        w.Render3D(&p, WIDTH/2, HEIGHT/2, 2);

        Point currentPos = Point(p.x, p.y);
        Wall *collision = checkCollision(&w, &p, currentPos, Point(newPos.x, newPos.y));
        if(!collision) {
            p.x = newPos.x;
            p.y = newPos.y;
        } else {
            Point wallPoint1 = w.worldVertices[collision->vertexIndex1];
            Point wallPoint2 = w.worldVertices[collision->vertexIndex2];

            double dx = wallPoint2.x - wallPoint1.x;
            double dy = wallPoint2.y - wallPoint1.y;
            double dirMultiplierX = 1;
            double dirMultiplierY = 1;

            if(dx < 0) {
                dx = wallPoint1.x - wallPoint2.x;
                dirMultiplierX = -1;
            }

            double pdx = newPos.x - currentPos.x;
            double pdy = newPos.y - currentPos.y;

            if(dy != 0) {
                double theta = std::atan2(pdy, pdx);
                double alpha = std::atan2(dy, dx);
                double gamma = PI - (theta + alpha);

                double v = MOVE_SPEED * std::cos(gamma);
                Point positionDelta = Point(
                    v * std::cos(alpha) * dirMultiplierX,
                    v * std::sin(alpha) * dirMultiplierY               
                );

                newPos.x = currentPos.x + positionDelta.x;
                newPos.y = currentPos.y + positionDelta.y;
            } else {
                newPos.x = currentPos.x + pdx;
                newPos.y = currentPos.y;
            }

            if(!checkCollision(&w, &p, currentPos, Point(newPos.x, newPos.y))) {
                p.x = newPos.x;
                p.y = newPos.y;
            }
        }
        DrawFPS(0, 0);

        EndDrawing();
        p.z = w.worldSectors[p.currentSector].floorHeight + 72;
    }

    return 0;
}

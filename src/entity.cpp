#include "world.hpp"
#define MOVE_SPEED 5

namespace Engine {
    void Entity::GetCurrentSector(World *w) {
        // Calculate what sector the entity is in.
        //   1. Translate the world to player-origin coordinates
        //   2. Iterate through each wall in the sector
        //   3. Cast a ray from the origin in the -y direction
        //   4. Count the number of intersections
        //   5. If there is an even number of intersections, the player is within the sector.
        //
        //   THIS ONLY WORKS WITH A CONVEX SECTOR.
        for(dword sIndex = 0; sIndex < w->worldSectors.size(); ++sIndex) {
            Sector *s = &w->worldSectors[sIndex];
            dword nintersections = 0;

            for(auto wIndex : s->wallIndices) {
                Wall wll = w->worldWalls[wIndex];
                Point v1 = w->worldVertices[wll.vertexIndex1];
                Point v2 = w->worldVertices[wll.vertexIndex2];

                bool entityInLineRange = ((v2.x >= x) && (v1.x <= x)) || ((v2.x <= x) && (v1.x >= x));
                if(!entityInLineRange) continue;

                v1.x -= x;
                v1.y -= y;

                v2.x -= x;
                v2.y -= y;

                if(v2.x == v1.x) continue;
                double m = (v2.y - v1.y) / (v2.x - v1.x);
                double c = v1.y - m * v1.x;

                double y = c;
                if(y < 0) { 
                    nintersections++;
                }             
            }

            if(nintersections % 2 == 1) {
                currentSector = sIndex;
            }
        }
    }

    void Entity::Init(World *w) {
        GetCurrentSector(w);

        w->worldSectors[currentSector].entities.push_back(this);
    }

    void Entity::Update(World *w) {
        dword oldS = currentSector;
        GetCurrentSector(w);

        if(currentSector != oldS) {
            std::vector<Entity*>::iterator it;
            int i;
            for(it = w->worldSectors[oldS].entities.begin(); it != w->worldSectors[oldS].entities.end(); it++, i++) {
                if(w->worldSectors[oldS].entities[i] == this) {
                    w->worldSectors[oldS].entities.erase(it);
                    break;
                } 
            }

            w->worldSectors[currentSector].entities.push_back(this);
        }
    }

    void Entity::Render(World *w, Player *p, dword width, dword height) {}

    void Player::Update(World *w) {
        Entity::Update(w);

        if(IsKeyDown(KEY_LEFT)) {
            angle -= PI/90;
        } else if(IsKeyDown(KEY_RIGHT)) {
            angle += PI/90;
        }

        Point newPos = Point(x, y);
        if(IsKeyDown(KEY_W)) {
            newPos.x += sin(angle) * MOVE_SPEED;
            newPos.y += cos(angle) * MOVE_SPEED;
        } 
        if(IsKeyDown(KEY_S)) {
            newPos.x -= sin(angle) * MOVE_SPEED;
            newPos.y -= cos(angle) * MOVE_SPEED;
        }
        if(IsKeyDown(KEY_A)) {
            newPos.x -= cos(angle) * MOVE_SPEED;
            newPos.y += sin(angle) * MOVE_SPEED;
        }
        if(IsKeyDown(KEY_D)) {
            newPos.x += cos(angle) * MOVE_SPEED;
            newPos.y -= sin(angle) * MOVE_SPEED;
        }

        Point currentPos = Point(x, y);
        Wall *collision = checkCollision(w, this, currentPos, Point(newPos.x, newPos.y));
        if(!collision) {
            x = newPos.x;
            y = newPos.y;
        } else {
            Point wallPoint1 = w->worldVertices[collision->vertexIndex1];
            Point wallPoint2 = w->worldVertices[collision->vertexIndex2];

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

            if(!checkCollision(w, this, currentPos, Point(newPos.x, newPos.y))) {
                x = newPos.x;
                y = newPos.y;
            }
        }
    }
}

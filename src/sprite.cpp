#include "sprite.hpp"
#include <cstdio>

namespace Engine {
    void StaticSprite::Render(World *w, Player *p, dword width, dword height) {
        Point pos_rot = Point(x, y).Project2D(p);

        double z1 = w->worldSectors[currentSector].floorHeight - p->z;
        double z2 = z1 + spr.height;

        if(pos_rot.y < 15) return; 

        Point p1_rot = Point(
            pos_rot.x - spr.width/2,
            pos_rot.y
        );

        Point p2_rot = Point(
            pos_rot.x + spr.width/2,
            pos_rot.y
        );

        Point p3_rot = p1_rot;
        Point p4_rot = p2_rot;

        Point p1 = Point(
             (p1_rot.x * 200) / (p1_rot.y) + width/2,
            -(z1       * 200) / (p1_rot.y) + height/2
        );
        Point p2 = Point(
             (p2_rot.x * 200) / (p2_rot.y) + width/2,
            -(z1       * 200) / (p2_rot.y) + height/2
        );
        Point p3 = Point(
             (p3_rot.x * 200) / (p3_rot.y) + width/2,
            -(z2       * 200) / (p3_rot.y) + height/2
        );
        Point p4 = Point(
             (p4_rot.x * 200) / (p4_rot.y) + width/2,
            -(z2       * 200) / (p4_rot.y) + height/2
        );

        double dx = p2.x - p1.x;
        double dy = p1.y - p3.y;

        for(int x = 0; x < dx; ++x) {
            for(int y = 0; y < dy; ++y) {
                double tx = x/dx;
                double ty = y/dy;

                Point texCoord = Point(
                    lerp(Point(0, 0), Point(spr.width, 0), tx).x,
                    lerp(Point(0, 0), Point(0, spr.height), ty).y
                );

                dword sprX = texCoord.x;
                dword sprY = texCoord.y;

                Color c = GetImageColor(spr, sprX, sprY);
                SetPix(Point(x + p1.x, y + p3.y), width, height, c);
            }
        }
        //DrawRectangle(2*p1.x, 2*p1.y, 2*( p4.x-p1.x), (2*p1.y-2*p4.y), RED);
    }
}

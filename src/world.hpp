#pragma once
#include "types.hpp"

#include <raylib.h>

#include <cmath>
#include <string>
#include <vector>

namespace Engine {
    static bool* pixelsWritten;
    static byte* pixels;
    static Texture2D tex;

    class Sector;
    class Entity;
    class Player;
    class World;
    class Point;
    class Wall;

    void SetPix(Point pos, dword width, dword height, Color c);


    Wall *checkCollision(World *w, Entity *p, Point oldPos, Point newPos, sdword skipWall = -1);

    enum class EntityType {
        Player,
        StaticSprite,
        Enemy
    };

    class Entity {
    public:
        double x, y, z, angle;
        dword currentSector;
        EntityType type;

        Entity(double x, double y, double z, double angle, dword cSec = 0, EntityType t=EntityType::StaticSprite): x(x), y(y), z(z), angle(angle), currentSector(cSec), type(t) {}

        virtual void Init(World *w);
        virtual void Update(World *w);
        virtual void Render(World *w, Player *p, dword width, dword height);

        void GetCurrentSector(World *w);
    };

    // Contains player information
    // Used for rendering the world and collisions
    class Player : public Entity {
    public:
        Player(double x, double y, double z, double angle, dword cSec = 0): Entity(x, y, z, angle, cSec, EntityType::Player) {}

        void Update(World *w) override;
    };

    class Point {
    public:
        double x, y;

        Point(double x = 0, double y = 0) : x(x), y(y) {}

        Point Project2D(Player *p);
        Point Project3D(Player *p, float z, float *depth = NULL);

        Point& operator= (const Point p) {
            if(this != &p) {
                x = p.x;
                y = p.y;
            }
            return *this;
        }
    };

    static inline Point lerp(Point a, Point b, double t) {
        return Point(
            (b.x - a.x) * t + (a.x),
            (b.y - a.y) * t + (a.y)
        );
    }


    class Wall {
    public:
        dword  vertexIndex1;
        dword  vertexIndex2;
        Color  wallColour;
        bool   isPortal;
        sdword frontSectorIndex;
        sdword backSectorIndex;

        Wall(dword v1, dword v2, Color c, bool isPortal = false, sdword front = -1, sdword back = -1) 
            : vertexIndex1(v1), vertexIndex2(v2), wallColour(c),
              isPortal(isPortal), frontSectorIndex(front), backSectorIndex(back) {}

        bool Draw3D(Player *p, sdword width, sdword height, std::vector<Point> points, float z, float h, float sectorLightMultiplier, Point *p1out = NULL, Point *p2out = NULL, Point *p3out = NULL, Point *p4out = NULL);
        bool DrawPortal(Player *p, sdword width, sdword height, std::vector<Point> points, std::vector<Sector> sectors, dword sectorID, Point *p1out = NULL, Point *p2out = NULL, Point *p3out = NULL, Point *p4out = NULL);
    };

    class Sector {
    public:
        std::vector<Entity *> entities;
        std::vector<dword> wallIndices;

        Color floorColour;
        Color ceilColour;

        sdword floorHeight;
        sdword ceilHeight;

        float lightMultiplier;

        Sector(std::vector<dword> walls, Color fc, Color cc, sdword fh, sdword ch, float l) :
            entities({}), wallIndices(walls), floorColour(fc), ceilColour(cc), floorHeight(fh), ceilHeight(ch), lightMultiplier(l) {}

        void Draw3D(Player *p, World *w, dword width, dword height, sdword currentSectorIndex, sdword lastSectorIndex = -1);
    };

    class World {
    public:
        std::vector<Point>  worldVertices;
        std::vector<Wall>   worldWalls;
        std::vector<Sector> worldSectors; 

        World(std::vector<Point> points, std::vector<Wall> walls, std::vector<Sector> sectors)
            : worldVertices(points), worldWalls(walls), worldSectors(sectors) {}

        int LoadMapFile(std::string path);

        void Render3D(Player *p, dword width, dword height, dword scale = 1);
    };
}

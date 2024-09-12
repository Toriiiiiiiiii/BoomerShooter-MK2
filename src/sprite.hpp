#pragma once
#include "world.hpp"

namespace Engine {
    class StaticSprite : public Entity {
    public:
        Image spr;

        StaticSprite(std::string imgPath, double x, double y, double z, dword cSec = 0) : Entity(x, y, z, 0, cSec, EntityType::StaticSprite) {
            spr = LoadImage(imgPath.c_str());
            ImageFormat(&spr, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        }

        void Render(World *w, Player *p, dword width, dword height) override;
    };
}

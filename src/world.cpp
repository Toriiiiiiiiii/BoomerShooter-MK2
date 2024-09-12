#include "world.hpp"

#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <iostream>

std::vector<std::string> SplitStringBySpaces(std::string s) {
    std::vector<std::string> result = {};
    std::string buf = "";

    Engine::dword index = 0;

    while(index < s.size()) {
        if(s[index] == ' ' && buf != "") {
            result.push_back(buf);
            buf = "";
        }
        else if(s[index] != ' ') {
            buf += s[index];
        }

        index++;
    }

    if(buf != "") {
        result.push_back(buf);
    }

    return result;
}

namespace Engine {
    Wall *checkCollision(World *w, Entity *p, Point oldPos, Point newPos, sdword skipWall) {
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

    void SetPix(Point pos, dword width, dword height, Color c) {
        if(!c.a) return;
        if(pos.x < 0 || pos.y < 0) return;
        if(pos.x >= width || pos.y >= height) return;

        if(pixelsWritten[(dword)pos.y * width + (dword)pos.x]) return;
        pixelsWritten[(dword)pos.y * width + (dword)pos.x] = true;

        int index = ((dword)pos.y * width + (dword)pos.x) * 4;
        pixels[index + 0] = c.r;
        pixels[index + 1] = c.g;
        pixels[index + 2] = c.b;
        pixels[index + 3] = 255;
    }

    Point Point::Project2D(Player *p) {
        Point result = Point(0, 0);

        // Translate the point by player's coordinates
        result.x = x - p->x;
        result.y = y - p->y;

        // Rotate the point by player's angle
        Point temp = Point(result.x, result.y);
        result.x = temp.x * cos(p->angle) - temp.y * sin(p->angle);
        result.y = temp.x * sin(p->angle) + temp.y * cos(p->angle);

        return result;
    }

    Point Point::Project3D(Player *p, float z, float* depth) {
        Point P2D = Project2D(p);
        if(depth != NULL) *depth = P2D.y;

        Point ScreenPoint = Point(0, 0);
        float localZ = z - p->z;

        if(P2D.y == 0) P2D.y = 0.001;

        ScreenPoint.x = (P2D.x * 200)  / P2D.y;
        ScreenPoint.y = (localZ * 200) / P2D.y;

        return ScreenPoint;
    }

    bool Wall::Draw3D(Player *p, sdword width, sdword height, std::vector<Point> points, float z, float h, float sectorLightMultiplier, Point *p1out, Point *p2out, Point *p3out, Point *p4out) {
        Point v1 = points[vertexIndex1];
        Point v2 = points[vertexIndex2];

        double vdx = v2.x - v1.x;
        double vdy = v2.y - v1.y;

        double theta = 0;

        if(v1.x < v2.x) {
            if(v1.y < v2.y) {
                theta = PI/2 + atan2(vdx, vdy);
            } else {
                theta = atan2(vdx, vdy);
            }
        } else {
            if(v1.y < v2.y) {
                theta = PI + atan2(vdx, vdy);
            } else {
                theta = 3*PI/2 + atan2(vdx, vdy);
            }
        }

        Point p1_rot = points[vertexIndex1].Project2D(p);
        Point p2_rot = points[vertexIndex2].Project2D(p);

        double z1 = z - p->z;
        double z2 = h - p->z;

        // Do not draw if behind player
        if(p1_rot.y < 0.00001 && p2_rot.y < 0.00001) return false;

        // Clip walls behind player if they are not visible
        double m = 0;
        if(p2_rot.x - p1_rot.x != 0) {
            m = (p2_rot.y - p1_rot.y) / (p2_rot.x - p1_rot.x);
        }

        double c = p1_rot.y - m * p1_rot.x;
        if(p1_rot.y < 1) {
            if(m != 0) {
                p1_rot.x = (1 - c) / m;
                p1_rot.y = 1;
            } else {
                p1_rot.y = 1;
            }
        }

        if(p2_rot.y < 1) {
            if(m != 0) {
                p2_rot.x = (1 - c) / m;
                p2_rot.y = 1;
            } else {
                p2_rot.y = 1;
            }
        }

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

        if(p2.x < p1.x) {
            Point swp = p2;
            p2 = p1;
            p1 = swp;

            swp = p4;
            p4 = p3;
            p3 = swp;
        } 

        float mBottom = (p2.y - p1.y) / (p2.x - p1.x);
        float mTop    = (p4.y - p3.y) / (p4.x - p3.x);

        float cBottom = p1.y - mBottom * p1.x;
        float cTop    = p3.y - mTop    * p3.x;

        float x1 = p1.x;
        float x2 = p2.x;

        if(x1 < 0) x1 = 0;
        if(x2 < 0) x2 = 0;
        if(x1 >= width) x1 = width;
        if(x2 >= width) x2 = width;

        double angledLightMultiplier = 0.5 * (theta/(2*PI)) + 0.5;
        float r = wallColour.r * sectorLightMultiplier * angledLightMultiplier;
        float g = wallColour.g * sectorLightMultiplier * angledLightMultiplier;
        float b = wallColour.b * sectorLightMultiplier * angledLightMultiplier;

        if(r > 255) r = 255;
        if(g > 255) g = 255;
        if(b > 255) b = 255;

        Color col = {
            (byte)(r),
            (byte)(g),
            (byte)(b),
            255
        };

        for(float x = x1; x < x2; ++x) {
            double yBottom = mBottom * x + cBottom;
            double yTop    = mTop    * x + cTop;

            if(yBottom < 0)       yBottom = 0;
            if(yTop    < 0)       yTop    = 0;
            if(yBottom >= height) yBottom = height;
            if(yTop    >= height) yTop    = height;

            for(float y = yTop; y < yBottom; ++y) {
                SetPix(Point(x, y), width, height, col);
            }
        }

        if(p1out) *p1out = p1;
        if(p2out) *p2out = p2;
        if(p3out) *p3out = p3;
        if(p4out) *p4out = p4;

        return true;
    }

    bool Wall::DrawPortal(Player *p, sdword width, sdword height, std::vector<Point> points, std::vector<Sector> sectors, dword sectorID, Point *p1out, Point *p2out, Point *p3out, Point *p4out) {
        Sector front = sectors[frontSectorIndex];
        Sector back  = sectors[backSectorIndex];

        dword lowestFloorSecIndex = backSectorIndex;
        dword highestFloorSecIndex = frontSectorIndex;
        dword lowestCeilSecIndex = backSectorIndex;
        dword highestCeilSecIndex = frontSectorIndex;
        float minFloor, maxFloor;
        float minCeil, maxCeil;

        if(back.floorHeight > front.floorHeight) {
            lowestFloorSecIndex = frontSectorIndex;
            highestFloorSecIndex = backSectorIndex;

            minFloor = front.floorHeight;
            maxFloor = back.floorHeight;
        } else {
            minFloor = back.floorHeight;
            maxFloor = front.floorHeight;
        }

        if(back.ceilHeight > front.ceilHeight) {
            lowestCeilSecIndex = frontSectorIndex;
            highestCeilSecIndex = backSectorIndex;

            minCeil = front.ceilHeight;
            maxCeil = back.ceilHeight;
        } else {
            minCeil = back.ceilHeight;
            maxCeil = front.ceilHeight;
        }

        Wall lowWall = Wall(0, 1, wallColour);
        Wall highWall = Wall(0, 1, wallColour);

        Point p1, p2, p3, p4;

        if(lowestFloorSecIndex == sectorID) {
            if(!lowWall.Draw3D(p, width, height, { points[vertexIndex1], points[vertexIndex2] }, minFloor, maxFloor, sectors[lowestFloorSecIndex].lightMultiplier, &p1, &p2)) return false;

            if(p1out) *p1out = p1;
            if(p2out) *p2out = p2;
        } else {
            if(!lowWall.Draw3D(p, width, height, { points[vertexIndex1], points[vertexIndex2] }, maxFloor, maxFloor, sectors[highestFloorSecIndex].lightMultiplier, &p1, &p2)) return false;

            if(p1out) *p1out = p1;
            if(p2out) *p2out = p2;
        }

        if(lowestCeilSecIndex != sectorID) {
            if(!highWall.Draw3D(p, width, height, { points[vertexIndex1], points[vertexIndex2] }, minCeil, maxCeil, sectors[highestCeilSecIndex].lightMultiplier, NULL, NULL, &p3, &p4)) return false;

            if(p3out) *p3out = p3;
            if(p4out) *p4out = p4;
        } else {
            if(!highWall.Draw3D(p, width, height, { points[vertexIndex1], points[vertexIndex2] }, minCeil, minCeil, sectors[lowestCeilSecIndex].lightMultiplier, NULL, NULL, &p3, &p4)) return false;

            if(p3out) *p3out = p3;
            if(p4out) *p4out = p4;
        }

        return true;
    }

    void Sector::Draw3D(Player *p, World *world, dword width, dword height, sdword currentSectorIndex, sdword lastSectorIndex) {
        // Draw Entities
        for(auto& entity : entities) {
            entity->Render(world, p, width, height);
        }

        std::vector<dword> indexesToIgnore = {};
        for(auto& wallIndex : wallIndices) {
            Wall w = world->worldWalls[wallIndex];

            Point p1, p2, p3, p4;

            bool res = false;
            if(w.isPortal) res = w.DrawPortal(p, width, height, world->worldVertices, world->worldSectors, currentSectorIndex, &p1, &p2, &p3, &p4);
            else res = w.Draw3D(p, width, height, world->worldVertices, floorHeight, ceilHeight, lightMultiplier, &p1, &p2, &p3, &p4);

            if(!res) { 
                indexesToIgnore.push_back(wallIndex);
                continue;
            }

            // Render Flats
            float mBottom = (p2.y - p1.y) / (p2.x - p1.x);
            float mTop    = (p4.y - p3.y) / (p4.x - p3.x);

            float cBottom = p1.y - mBottom * p1.x;
            float cTop    = p3.y - mTop    * p3.x;

            float x1 = p1.x;
            float x2 = p2.x;

            Color cCeil = {
                (byte)(ceilColour.r * lightMultiplier),
                (byte)(ceilColour.g * lightMultiplier),
                (byte)(ceilColour.b * lightMultiplier),
                255
            };

            Color cFloor = {
                (byte)(floorColour.r * lightMultiplier),
                (byte)(floorColour.g * lightMultiplier),
                (byte)(floorColour.b * lightMultiplier),
                255
            };

            for(float x = x1; x < x2; ++x) {
                double yBottom = mBottom * x + cBottom;
                double yTop    = mTop    * x + cTop;

                if(yBottom < 0)       yBottom = 0;
                if(yTop    < 0)       yTop    = 0;
                if(yBottom >= height) yBottom = height - 1;
                if(yTop    >= height) yTop    = height - 1;

                for(float y = yBottom; y < height; ++y) {
                    SetPix(Point(x, y), width, height, cFloor);
                }

                for(float y = 0; y < yTop; ++y) {
                    SetPix(Point(x, y), width, height, cCeil);
                }
            }
        }

        // Draw Portals
        for(auto& wallIndex : wallIndices) {
            if(!world->worldWalls[wallIndex].isPortal) continue;
            //if(std::find(indexesToIgnore.begin(), indexesToIgnore.end(), wallIndex) != indexesToIgnore.end()) continue;

            Wall w = world->worldWalls[wallIndex];
            sdword sectorIndex = 0;
            if(w.frontSectorIndex == currentSectorIndex) {
                sectorIndex = w.backSectorIndex;
            }
            else {
                sectorIndex = w.frontSectorIndex;
            }

            if(sectorIndex == lastSectorIndex) continue;
            world->worldSectors[sectorIndex].Draw3D(p, world, width, height, sectorIndex, currentSectorIndex);
        }

    }

    void World::Render3D(Player *p, dword width, dword height, dword scale) {
        if(!pixelsWritten) pixelsWritten = (bool *)std::malloc(width * height * sizeof(bool));
        if(!pixels) pixels = (byte *)std::malloc(width * height * 4); 

        if(!tex.id) {
            Image img = GenImageColor(width, height, BLACK);
            ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

            tex = LoadTextureFromImage(img);
            UnloadImage(img);
        }

        for(dword i = 0; i < width * height; ++i) {
            pixelsWritten[i] = false;
        }

        for(dword i = 0; i < width * height * 4; ++i) {
            pixels[i] = 0;
        }

        
        worldSectors[p->currentSector].Draw3D(p, this, width, height, p->currentSector);

        UpdateTexture(tex, pixels);
        DrawTextureEx(tex, {0, 0}, 0.0f, (float)scale, WHITE);
    }

    int World::LoadMapFile(std::string path) {
        std::ifstream file(path);
        if(!file.is_open()) {
            return 1;
        }

        std::vector<std::string> lines = {};
        std::string line;
        while(std::getline(file, line)) {
            lines.push_back(line);
        }

        for(auto& l : lines) {
            if(l == "") continue;
            std::vector<std::string> args = SplitStringBySpaces(l);
            
            std::string type = args[0];
            if(type == "vertex") {
                Point p = Point(
                    std::strtof(args[1].c_str(), NULL),
                    std::strtof(args[2].c_str(), NULL)
                );

                worldVertices.push_back(p);
            }

            else if(type == "wall") {
                sdword pointIndex1 = std::strtod(args[1].c_str(), NULL); 
                sdword pointIndex2 = std::strtod(args[2].c_str(), NULL);
                Color  c           = Color{(byte)std::strtod(args[3].c_str(), NULL),
                                           (byte)std::strtod(args[4].c_str(), NULL),
                                           (byte)std::strtod(args[5].c_str(), NULL),
                                           255};

                worldWalls.push_back(Wall(pointIndex1, pointIndex2, c));
            }

            else if(type == "portal") {
                sdword pointIndex1 = std::strtod(args[1].c_str(), NULL); 
                sdword pointIndex2 = std::strtod(args[2].c_str(), NULL);
                sdword frontSector = std::strtod(args[3].c_str(), NULL);
                sdword backSector  = std::strtod(args[4].c_str(), NULL);
                Color  c           = Color{(byte)std::strtod(args[5].c_str(), NULL),
                                           (byte)std::strtod(args[6].c_str(), NULL),
                                           (byte)std::strtod(args[7].c_str(), NULL),
                                           255};

                worldWalls.push_back(Wall(pointIndex1, pointIndex2, c, true, frontSector, backSector));
            }

            else if(type == "sector") {
                dword nWalls = std::strtod(args[1].c_str(), NULL);
                std::vector<dword> wallIndexes = {};

                for(dword i = 0; i < nWalls; ++i) {
                    wallIndexes.push_back(std::strtod(args[2 + i].c_str(), NULL));
                }

                dword index = 2 + nWalls;
                sdword floorHeight = std::strtod(args[index + 0].c_str(), NULL);
                sdword ceilHeight  = std::strtod(args[index + 1].c_str(), NULL);

                Color  floorC      = Color{(byte)std::strtod(args[index + 2].c_str(), NULL),
                                           (byte)std::strtod(args[index + 3].c_str(), NULL),
                                           (byte)std::strtod(args[index + 4].c_str(), NULL),
                                           255};

                Color  ceilC       = Color{(byte)std::strtod(args[index + 5].c_str(), NULL),
                                           (byte)std::strtod(args[index + 6].c_str(), NULL),
                                           (byte)std::strtod(args[index + 7].c_str(), NULL),
                                           255};

                float  lightMult   = std::strtof(args[index + 8].c_str(), NULL);

                worldSectors.push_back(Sector(wallIndexes, floorC, ceilC, floorHeight, ceilHeight, lightMult));
            }
        }

        return 0;
    }
}

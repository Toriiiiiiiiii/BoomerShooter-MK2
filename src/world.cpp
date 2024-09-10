#include "world.hpp"

#include <cstddef>
#include <cstdlib>
#include <fstream>
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
    void SetPix(Point pos, dword width, dword height, Color c) {
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

    void Wall::Draw3D(Player *p, sdword width, sdword height, std::vector<Point> points, float z, float h, Point *p1out, Point *p2out, Point *p3out, Point *p4out) {
        Point p1_rot = points[vertexIndex1].Project2D(p);
        Point p2_rot = points[vertexIndex2].Project2D(p);

        double z1 = z - p->z;
        double z2 = h - p->z;

        // Do not draw if behind player
        if(p1_rot.y < 0.0001 && p2_rot.y < 0.0001) return;

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

        for(float x = x1; x < x2; ++x) {
            double yBottom = mBottom * x + cBottom;
            double yTop    = mTop    * x + cTop;

            if(yBottom < 0)       yBottom = 0;
            if(yTop    < 0)       yTop    = 0;
            if(yBottom >= height) yBottom = height;
            if(yTop    >= height) yTop    = height;

            for(float y = yTop; y < yBottom; ++y) {
                SetPix(Point(x, y), width, height, wallColour);
            }
        }

        if(p1out) *p1out = p1;
        if(p2out) *p2out = p2;
        if(p3out) *p3out = p3;
        if(p4out) *p4out = p4;
    }

    void Wall::DrawPortal(Player *p, sdword width, sdword height, std::vector<Point> points, std::vector<Sector> sectors, dword sectorID, Point *p1out, Point *p2out, Point *p3out, Point *p4out) {
        Sector front = sectors[frontSectorIndex];
        Sector back  = sectors[backSectorIndex];

        dword lowestFloorSecIndex = backSectorIndex;
        dword lowestCeilSecIndex = backSectorIndex;
        float minFloor, maxFloor;
        float minCeil, maxCeil;

        if(back.floorHeight > front.floorHeight) {
            lowestFloorSecIndex = frontSectorIndex;
            minFloor = front.floorHeight;
            maxFloor = back.floorHeight;
        } else {
            minFloor = back.floorHeight;
            maxFloor = front.floorHeight;
        }

        if(back.ceilHeight > front.ceilHeight) {
            lowestCeilSecIndex = frontSectorIndex;
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
            lowWall.Draw3D(p, width, height, { points[vertexIndex1], points[vertexIndex2] }, minFloor, maxFloor, &p1, &p2);

            if(p1out) *p1out = p1;
            if(p2out) *p2out = p2;
        } else {
            lowWall.Draw3D(p, width, height, { points[vertexIndex1], points[vertexIndex2] }, maxFloor, maxFloor, &p1, &p2);

            if(p1out) *p1out = p1;
            if(p2out) *p2out = p2;
        }

        if(lowestCeilSecIndex != sectorID) {
            highWall.Draw3D(p, width, height, { points[vertexIndex1], points[vertexIndex2] }, minCeil, maxCeil, NULL, NULL, &p3, &p4);

            if(p3out) *p3out = p3;
            if(p4out) *p4out = p4;
        } else {
            highWall.Draw3D(p, width, height, { points[vertexIndex1], points[vertexIndex2] }, minCeil, minCeil, NULL, NULL, &p3, &p4);

            if(p3out) *p3out = p3;
            if(p4out) *p4out = p4;
        }

    }

    void Sector::Draw3D(Player *p, dword width, dword height, std::vector<Wall> walls, std::vector<Point> points, std::vector<Sector> sectors, sdword currentSectorIndex, sdword lastSectorIndex) {
        for(auto& wallIndex : wallIndices) {
            Wall w = walls[wallIndex];

            Point p1, p2, p3, p4;

            if(w.isPortal) w.DrawPortal(p, width, height, points, sectors, currentSectorIndex, &p1, &p2, &p3, &p4);
            else w.Draw3D(p, width, height, points, floorHeight, ceilHeight, &p1, &p2, &p3, &p4);

            // Render Flats
            float mBottom = (p2.y - p1.y) / (p2.x - p1.x);
            float mTop    = (p4.y - p3.y) / (p4.x - p3.x);

            float cBottom = p1.y - mBottom * p1.x;
            float cTop    = p3.y - mTop    * p3.x;

            float x1 = p1.x;
            float x2 = p2.x;

            for(float x = x1; x < x2; ++x) {
                double yBottom = mBottom * x + cBottom;
                double yTop    = mTop    * x + cTop;

                if(yBottom < 0)       yBottom = 0;
                if(yTop    < 0)       yTop    = 0;
                if(yBottom >= height) yBottom = height - 1;
                if(yTop    >= height) yTop    = height - 1;

                for(float y = yBottom; y < height; ++y) {
                    SetPix(Point(x, y), width, height, floorColour);
                }

                for(float y = 0; y < yTop; ++y) {
                    SetPix(Point(x, y), width, height, ceilColour);
                }
            }
        }

        // Draw Portals
        for(auto& wallIndex : wallIndices) {
            if(!walls[wallIndex].isPortal) continue;

            Wall w = walls[wallIndex];
            sdword sectorIndex = 0;
            if(w.frontSectorIndex == currentSectorIndex) {
                sectorIndex = w.backSectorIndex;
            }
            else {
                sectorIndex = w.frontSectorIndex;
            }

            if(sectorIndex == lastSectorIndex) continue;
            sectors[sectorIndex].Draw3D(p, width, height, walls, points, sectors, sectorIndex, currentSectorIndex);
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

        // Calculate what sector the player is in.
        //   1. Translate the world to player-origin coordinates
        //   2. Iterate through each wall in the sector
        //   3. Cast a ray from the origin in the -y direction
        //   4. Count the number of intersections
        //   5. If there is an even number of intersections, the player is within the sector.
        //
        //   THIS ONLY WORKS WITH A CONVEX SECTOR.
        for(dword sIndex = 0; sIndex < worldSectors.size(); ++sIndex) {
            Sector s = worldSectors[sIndex];
            dword nintersections = 0;

            for(auto wIndex : s.wallIndices) {
                Wall w = worldWalls[wIndex];
                Point v1 = worldVertices[w.vertexIndex1];
                Point v2 = worldVertices[w.vertexIndex2];

                bool playerInLineRange = ((v2.x >= p->x) && (v1.x <= p->x)) || ((v2.x <= p->x) && (v1.x >=  p->x));
                if(!playerInLineRange) continue;

                v1.x -= p->x;
                v1.y -= p->y;

                v2.x -= p->x;
                v2.y -= p->y;

                if(v2.x == v1.x) continue;
                double m = (v2.y - v1.y) / (v2.x - v1.x);
                double c = v1.y - m * v1.x;

                double y = c;
                if(y < 0) { 
                    nintersections++;
                }             
            }

            if(nintersections % 2 == 1) {
                p->currentSector = sIndex;
            }
        }
        
        worldSectors[p->currentSector].Draw3D(p, width, height, worldWalls, worldVertices, worldSectors, p->currentSector);

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

                worldSectors.push_back(Sector(wallIndexes, floorC, ceilC, floorHeight, ceilHeight));
            }
        }

        return 0;
    }
}

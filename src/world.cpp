#include "world.hpp"
#include <stdlib.h>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>
#include <PerlinNoise.hpp>
#include <iostream>


World::World(int _width, int _height, int _seed, unsigned char _sea_level): width(_width), height(_height), seed(_seed), sea_level(_sea_level), MAX_TEMP(35), MIN_TEMP(-15)
{
    tiles = std::vector<Tile>(width * height);
}

void World::Generate()
{
    CreateTiles();
    GenerateTectonics();
    GenerateHeightmap();
    GenerateClimate();
    GenerateRivers();
    PopulateTiles();
}

void World::Regenerate(int _seed)
{
    seed = _seed;
    Generate();
}

int World::GetWidth() const
{
    return width;
}

int World::GetHeight() const
{
    return height;
}

int World::GetSeed() const
{
    return seed;
}

void World::PreviewTectonics() const
{
    Color plateColors[] = {
        MAROON,
        GREEN,
        BLUE,
        PURPLE,
        MAGENTA,
        GOLD,
        YELLOW,
        DARKPURPLE,
        DARKGREEN,
        DARKBLUE,
        DARKGRAY,
        PINK,
        VIOLET,
        LIME,
        SKYBLUE,
        ORANGE,
        BEIGE,
        BROWN,
        DARKBROWN,
        GRAY,
        DARKGRAY
    };

    // Draw the tiles colored by their parent plate
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            int plateId = tiles[index].GetPlateId();
            Color c = plateColors[plateId % 21];
            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, c);
        }
    }

    // Draw velocity arrows and plate center dots
    for (size_t i = 0; i < plates.size(); ++i) {
        Vector2 center = plates[i];
        Vector2 vel = plate_velocities[i];

        // Convert grid coordinates to pixel space
        Vector2 startPos = { center.x * TILE_SIZE, center.y * TILE_SIZE };
        
        // Scale velocity for visibility (tweak the multiplier if arrows are too short/long)
        float arrowScale = TILE_SIZE * 3.0f; 
        Vector2 endPos = { startPos.x + (vel.x * arrowScale), startPos.y + (vel.y * arrowScale) };

        // Draw the velocity line in black (you can use DrawLineEx for thickness if you want)
        DrawLineV(startPos, endPos, BLACK);

        // Draw the red center dot on top
        DrawCircleV(startPos, 5, RED);
    }
}

void World::PreviewHeight() const
{
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            unsigned char height = tiles[index].GetHeight();
            unsigned char shade = height;
            Color c;
            if (height <= sea_level){
                int blue = std::min(255, shade + 25);
                c = { shade, shade, static_cast<unsigned char>(blue), 255 };
            }
            else {
                int green = std::min(255, shade + 25);
                c = { shade, static_cast<unsigned char>(green), shade, 255 };
            }

            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, c);
        }
    }
}

void World::PreviewTemperature() const
{
    // Define exact temperature anchors and their corresponding colors
    struct TempStop {
        float temp;
        Color color;
    };

    TempStop stops[] = {
        {-20.0f, { 15,  35, 130, 255}}, // Deep blue (Freezing)
        {  0.0f, { 70, 210, 200, 255}}, // Light turquoise (Freezing point / Frost)
        { 15.0f, { 50, 190,  60, 255}}, // Vibrant green (Temperate)
        { 25.0f, {245, 175,  40, 255}}, // Yellow-orange (Hot / Arid)
        { 40.0f, {250,  25,  25, 255}}  // Deep red (Scorching)
    };
    int numStops = 5;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            float temp = tiles[index].GetTemperature();
            temp = std::clamp(temp, MIN_TEMP, MAX_TEMP);

            // Find which two color stops this temperature falls between
            int i = 0;
            while (i < numStops - 1 && temp > stops[i + 1].temp) {
                i++;
            }

            Color c;
            if (i >= numStops - 1) {
                c = stops[numStops - 1].color;
            } else if (temp <= stops[0].temp) {
                c = stops[0].color;
            } else {
                // Local interpolation factor (0.0 to 1.0) between the two active stops
                float t = (temp - stops[i].temp) / (stops[i + 1].temp - stops[i].temp);
                t = std::clamp(t, 0.0f, 1.0f);

                unsigned char r = static_cast<unsigned char>(stops[i].color.r + t * (stops[i + 1].color.r - stops[i].color.r));
                unsigned char g = static_cast<unsigned char>(stops[i].color.g + t * (stops[i + 1].color.g - stops[i].color.g));
                unsigned char b = static_cast<unsigned char>(stops[i].color.b + t * (stops[i + 1].color.b - stops[i].color.b));

                c = { r, g, b, 255 };
            }

            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, c);
        }
    }
}

void World::PreviewWinds() const
{
    // Define exact magnitude stops and their corresponding colors 
    // (ranging from dark blue -> blue -> purple -> magenta)
    struct WindStop {
        float magnitude;
        Color color;
    };

    WindStop stops[] = {
        {0,  { 100, 220, 160, 255}},
        {1,  { 180, 200, 80, 255}}, 
        {2,  { 220, 120, 40, 255}}, 
        {3,  { 220, 50, 30, 255}},
        {4,  { 200, 20, 150, 255}},
        {5,  { 240, 120, 230, 255}}
    };
    int numStops = 6;

    // Draw the heightmap below the wind vectors
    PreviewHeight();

    // Draw wind vectors over the tiles
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            Vector2 wind = tiles[index].GetWind();
            float mag = std::sqrt(wind.x * wind.x + wind.y * wind.y);

            // Find which two color stops this magnitude falls between
            int i = 0;
            while (i < numStops - 1 && mag > stops[i + 1].magnitude) {
                i++;
            }

            Color c;
            if (i >= numStops - 1) {
                c = stops[numStops - 1].color;
            } else if (mag <= stops[0].magnitude) {
                c = stops[0].color;
            } else {
                float t = (mag - stops[i].magnitude) / (stops[i + 1].magnitude - stops[i].magnitude);
                t = std::clamp(t, 0.0f, 1.0f);

                unsigned char r = static_cast<unsigned char>(stops[i].color.r + t * (stops[i + 1].color.r - stops[i].color.r));
                unsigned char g = static_cast<unsigned char>(stops[i].color.g + t * (stops[i + 1].color.g - stops[i].color.g));
                unsigned char b = static_cast<unsigned char>(stops[i].color.b + t * (stops[i + 1].color.b - stops[i].color.b));

                c = { r, g, b, 255 };
            }

            // Center of the tile in pixel space
            Vector2 center = {
                (x * TILE_SIZE) + (TILE_SIZE * 0.5f),
                (y * TILE_SIZE) + (TILE_SIZE * 0.5f)
            };

            // Scale the wind vector for clear visualization
            float lineScale = TILE_SIZE * 1.0f;
            Vector2 endPos = {
                center.x + (wind.x * lineScale),
                center.y + (wind.y * lineScale)
            };

            // Draw center point and directional line
            DrawCircleV(center, 2.0f, c);
            DrawLineEx(center, endPos, 1, c);
        }
    }
}

void World::PreviewRainfall() const
{
    struct RainStop {
        float rainfall; 
        Color color;
    };

    RainStop stops[] = {
        {0.0f,  { 220,  40,  40, 255}}, // Red (Arid / Desert)
        {0.25f, { 240, 180,  40, 255}}, // Yellow-Orange (Semi-arid / Steppe)
        {0.5f,  {  60, 190,  80, 255}}, // Green (Temperate / Moderate)
        {0.75f, {  50, 130, 230, 255}}, // Blue (Wet / Humid)
        {1.0f,  { 210,  40, 200, 255}}  // Magenta (Monsoonal Wetlands / Jungle)
    };
    int numStops = 5;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            
            // Render oceans as blue water
            if (tiles[index].GetHeight() <= sea_level) {
                DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, { 40, 100, 180, 255 });
                continue;
            }

            float rainVal = static_cast<float>(tiles[index].GetRainfall()) / 255.0f;
            rainVal = std::clamp(rainVal, 0.0f, 1.0f);

            int i = 0;
            while (i < numStops - 1 && rainVal > stops[i + 1].rainfall) {
                i++;
            }

            Color c;
            if (i >= numStops - 1) {
                c = stops[numStops - 1].color;
            } else if (rainVal <= stops[0].rainfall) {
                c = stops[0].color;
            } else {
                float t = (rainVal - stops[i].rainfall) / (stops[i + 1].rainfall - stops[i].rainfall);
                t = std::clamp(t, 0.0f, 1.0f);

                unsigned char r = static_cast<unsigned char>(stops[i].color.r + t * (stops[i + 1].color.r - stops[i].color.r));
                unsigned char g = static_cast<unsigned char>(stops[i].color.g + t * (stops[i + 1].color.g - stops[i].color.g));
                unsigned char b = static_cast<unsigned char>(stops[i].color.b + t * (stops[i + 1].color.b - stops[i].color.b));

                c = { r, g, b, 255 };
            }

            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, c);
        }
    }
}

void World::PreviewSoilFertility() const
{
    Color dry_col = {240, 200, 40, 255};
    Color fertile_col = {50, 180, 10, 255};

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            
            // Render oceans as blue water
            if (tiles[index].GetHeight() <= sea_level) {
                DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, { 40, 100, 180, 255 });
                continue;
            }

            float fertility = tiles[index].GetSoilFertility();

            Color c = {
                std::lerp(dry_col.r, fertile_col.r, fertility),
                std::lerp(dry_col.g, fertile_col.g, fertility),
                std::lerp(dry_col.b, fertile_col.b, fertility),
                255
            };

            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, c);
        }
    }
}

void World::PreviewLumber() const
{
    Color low_col = {250, 140, 40, 255};
    Color high_col = {10, 200, 100, 255};

    // Get highest max lumber value.
    float max_lumber_max = 0;
    for (const Tile& t : tiles){
        if (t.GetMaxLumber() > max_lumber_max) max_lumber_max = t.GetMaxLumber();
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            
            // Render oceans as blue water
            if (tiles[index].GetHeight() <= sea_level) {
                DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, { 40, 100, 180, 255 });
                continue;
            }

            float norm_max_lumber = tiles[index].GetMaxLumber() / max_lumber_max;

            Color c = {
                std::lerp(low_col.r, high_col.r, norm_max_lumber),
                std::lerp(low_col.g, high_col.g, norm_max_lumber),
                std::lerp(low_col.b, high_col.b, norm_max_lumber),
                255
            };

            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, c);
        }
    }
}

void World::PreviewStone() const
{
    struct StoneStop {
        float stone;
        Color color;
    };

    StoneStop stops[] = {
        { 0.0f, { 10,  10,  10, 255} },
        { 500000.0f, {250, 240, 245, 255} },
        { 2500000.0f, {165, 50, 145, 255} },
        { 20000000.0f, {255, 120, 234, 155}}
    };
    int numStops = 4;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            
            // Render oceans as blue water
            if (tiles[index].GetHeight() <= sea_level) {
                DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, { 40, 100, 180, 255 });
                continue;
            }

            float stoneVal = static_cast<float>(tiles[index].GetStone());

            // Find which two color stops this stone value falls between
            int i = 0;
            while (i < numStops - 1 && stoneVal > stops[i + 1].stone) {
                i++;
            }

            Color c;
            if (i >= numStops - 1) {
                c = stops[numStops - 1].color;
            } else if (stoneVal <= stops[0].stone) {
                c = stops[0].color;
            } else {
                float t = (stoneVal - stops[i].stone) / (stops[i + 1].stone - stops[i].stone);
                t = std::clamp(t, 0.0f, 1.0f);

                unsigned char r = static_cast<unsigned char>(stops[i].color.r + t * (stops[i + 1].color.r - stops[i].color.r));
                unsigned char g = static_cast<unsigned char>(stops[i].color.g + t * (stops[i + 1].color.g - stops[i].color.g));
                unsigned char b = static_cast<unsigned char>(stops[i].color.b + t * (stops[i + 1].color.b - stops[i].color.b));

                c = { r, g, b, 255 };
            }

            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, c);
        }
    }
}

void World::PreviewMetals() const
{
    Color low_col = {200, 200, 220, 255};
    Color high_col = {245, 60, 20, 255};

    // Get highest stone value.
    float max_metals = 0;
    for (const Tile& t : tiles){
        if (t.GetMetals() > max_metals) max_metals = t.GetMetals();
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            
            // Render oceans as blue water
            if (tiles[index].GetHeight() <= sea_level) {
                DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, { 40, 100, 180, 255 });
                continue;
            }

            float norm_max_metals = tiles[index].GetMetals() / max_metals;

            Color c = {
                std::lerp(low_col.r, high_col.r, norm_max_metals),
                std::lerp(low_col.g, high_col.g, norm_max_metals),
                std::lerp(low_col.b, high_col.b, norm_max_metals),
                255
            };

            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, c);
        }
    }
}

void World::PreviewCoal() const
{
    Color low_col = {200, 200, 220, 255};
    Color high_col = {65, 45, 80, 255};

    // Get highest stone value.
    float max_coal = 0;
    for (const Tile& t : tiles){
        if (t.GetCoal() > max_coal) max_coal = t.GetCoal();
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            
            // Render oceans as blue water
            if (tiles[index].GetHeight() <= sea_level) {
                DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, { 40, 100, 180, 255 });
                continue;
            }

            float norm_max_metals = tiles[index].GetCoal() / max_coal;

            Color c = {
                std::lerp(low_col.r, high_col.r, norm_max_metals),
                std::lerp(low_col.g, high_col.g, norm_max_metals),
                std::lerp(low_col.b, high_col.b, norm_max_metals),
                255
            };

            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, c);
        }
    }
}

void World::PreviewMinerals() const
{
    Color low_col = {160, 160, 180, 255};
    Color high_col = {235, 250, 100, 255};

    // Get highest stone value.
    float max_minerals = 0;
    for (const Tile& t : tiles){
        if (t.GetMinerals() > max_minerals) max_minerals = t.GetMinerals();
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            
            // Render oceans as blue water
            if (tiles[index].GetHeight() <= sea_level) {
                DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, { 40, 100, 180, 255 });
                continue;
            }

            float norm_max_metals = tiles[index].GetMinerals() / max_minerals;

            Color c = {
                std::lerp(low_col.r, high_col.r, norm_max_metals),
                std::lerp(low_col.g, high_col.g, norm_max_metals),
                std::lerp(low_col.b, high_col.b, norm_max_metals),
                255
            };

            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, c);
        }
    }
}

void World::SetMaxTemp(float temp)
{
    MAX_TEMP = temp;
}

void World::SetMinTemp(float temp)
{
    MIN_TEMP = temp;
}

float World::GetMaxTemp() const
{
    return MAX_TEMP;
}

float World::GetMinTemp() const
{
    return MIN_TEMP;
}

unsigned char World::GetSeaLevel() const
{
    return sea_level;
}

const Tile &World::GetTile(int x, int y) const
{
    return tiles[x + y * width];
}

Tile &World::GetTile(int x, int y)
{
    return tiles[x + y * width];
}

const std::vector<Tile> &World::GetTiles() const
{
    return tiles;
}

std::vector<Tile> &World::GetTiles()
{
    return tiles;
}

void World::CreateTiles()
{
    for (Tile& t : tiles){
        t = Tile(0);
    }
}


void World::GenerateTectonics()
{
    srand(seed);
    int target_plates = width * height / 250;
    target_plates = std::max(2, target_plates); // Ensure at least 2 plates

    // Generate initial random plates
    plates = std::vector<Vector2>(target_plates);
    for (int i = 0; i < target_plates; i++){
        plates[i] = {static_cast<float>(rand() % width), static_cast<float>(rand() % height)};
    }

    // --- LLOYD'S RELAXATION (Runs 4 times to smooth out shapes) ---
    for (int iteration = 0; iteration < 4; ++iteration) {
        
        // 1. Voronoi assignment
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int index = x + y * width;
                
                float minDistance = 9999999.9f;
                int closestPlate = 0;

                for (size_t i = 0; i < plates.size(); ++i) {
                    float dx = static_cast<float>(x) - plates[i].x;
                    float dy = static_cast<float>(y) - plates[i].y;
                    float distSq = (dx * dx) + (dy * dy);

                    if (distSq < minDistance) {
                        minDistance = distSq;
                        closestPlate = static_cast<int>(i);
                    }
                }
                tiles[index].SetPlateId(closestPlate);
            }
        }

        // 2. Compute centroids (average position of all tiles in a plate)
        std::vector<float> sumX(plates.size(), 0.0f);
        std::vector<float> sumY(plates.size(), 0.0f);
        std::vector<int> count(plates.size(), 0);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int index = x + y * width;
                int pid = tiles[index].GetPlateId();
                sumX[pid] += static_cast<float>(x);
                sumY[pid] += static_cast<float>(y);
                count[pid]++;
            }
        }

        // 3. Move plate seeds to their actual centers
        for (size_t i = 0; i < plates.size(); ++i) {
            if (count[i] > 0) {
                plates[i].x = sumX[i] / static_cast<float>(count[i]);
                plates[i].y = sumY[i] / static_cast<float>(count[i]);
            }
        }
    }

    // Final Voronoi pass after relaxation so tiles match final seed positions
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            
            float minDistance = 9999999.9f;
            int closestPlate = 0;

            for (size_t i = 0; i < plates.size(); ++i) {
                float dx = static_cast<float>(x) - plates[i].x;
                float dy = static_cast<float>(y) - plates[i].y;
                float distSq = (dx * dx) + (dy * dy);

                if (distSq < minDistance) {
                    minDistance = distSq;
                    closestPlate = static_cast<int>(i);
                }
            }
            tiles[index].SetPlateId(closestPlate);
        }
    }

    // --- MULTI-PASS ITERATIVE BLOBBY MERGING ---
    int merge_passes = 3;
    for (int pass = 0; pass < merge_passes; ++pass) {

        // 1. Find max plate ID currently active
        int max_plate_id = 0;
        for (const auto& tile : tiles) {
            if (tile.GetPlateId() > max_plate_id) {
                max_plate_id = tile.GetPlateId();
            }
        }
        int num_plates = max_plate_id + 1;

        // 2. Calculate current size (tile count) of every plate & set hard size cap (25% of map max)
        std::vector<int> plateSizes(num_plates, 0);
        for (const auto& tile : tiles) {
            int pid = tile.GetPlateId();
            if (pid >= 0 && pid < num_plates) {
                plateSizes[pid]++;
            }
        }
        int max_allowed_size = (width * height) * 0.1f;

        // 3. Identify edge plates for compensation
        std::vector<bool> isEdgePlate(num_plates, false);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                    int pid = tiles[x + y * width].GetPlateId();
                    if (pid >= 0 && pid < num_plates) {
                        isEdgePlate[pid] = true;
                    }
                }
            }
        }

        // 4. Use std::map to track border lengths (thickness of contact face)
        std::map<std::pair<int, int>, int> borderLengths;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int current_plate = tiles[x + y * width].GetPlateId();

                if (x + 1 < width) {
                    int rightId = tiles[(x + 1) + y * width].GetPlateId();
                    if (current_plate != rightId) {
                        borderLengths[std::minmax(current_plate, rightId)]++;
                    }
                }
                if (y + 1 < height) {
                    int downId = tiles[x + (y + 1) * width].GetPlateId();
                    if (current_plate != downId) {
                        borderLengths[std::minmax(current_plate, downId)]++;
                    }
                }
            }
        }

        // 5. Build adjacency list from border keys
        std::vector<std::vector<int>> plateNeighbors(num_plates);
        for (auto& pair : borderLengths) {
            int p1 = pair.first.first;
            int p2 = pair.first.second;
            plateNeighbors[p1].push_back(p2);
            plateNeighbors[p2].push_back(p1);
        }

        // 6. Setup Union-Find parent array
        std::vector<int> plate_parent(num_plates);
        for (int i = 0; i < num_plates; ++i) {
            plate_parent[i] = i;
        }

        auto find_root = [&](int i, auto& self) -> int {
            if (plate_parent[i] == i) return i;
            return plate_parent[i] = self(plate_parent[i], self);
        };

        // 7. Evaluate merge chance with toned-down probabilities & Fat-Face target selection
        for (int i = 0; i < num_plates; ++i) {
            int rootI = find_root(i, find_root);
            if (rootI != i) continue; 

            const auto& neighbors = plateNeighbors[i];
            int neighborCount = neighbors.size();
            if (neighborCount == 0) continue;

            int effectiveNeighborCount = neighborCount;
            if (isEdgePlate[i]) {
                effectiveNeighborCount += 1; // Edge compensation
            }

            // Toned-down probabilities capped safely below 1.0 to prevent worms/gigablobs
            float mergeChance = 0.0f;
            switch (effectiveNeighborCount) {
                case 2: mergeChance = 0.05f; break;
                case 3: mergeChance = 0.15f; break;
                case 4: mergeChance = 0.25f; break;
                default: mergeChance = 0.75f; break; // Capped max probability
            }

            float roll = static_cast<float>(rand()) / RAND_MAX;
            if (roll < mergeChance) {
                int targetPlate = -1;
                int maxSharedBorder = -1;

                // FAT-FACE SELECTION: Pick neighbor with the thickest shared tile border
                for (int nId : neighbors) {
                    int rootTarget = find_root(nId, find_root);
                    if (plateSizes[rootTarget] >= max_allowed_size) continue; // Respect size cap

                    auto borderKey = std::minmax(i, nId);
                    int sharedCount = borderLengths.count(borderKey) ? borderLengths[borderKey] : 0;

                    if (sharedCount > maxSharedBorder) {
                        maxSharedBorder = sharedCount;
                        targetPlate = nId;
                    }
                }

                if (targetPlate != -1) {
                    int rootTarget = find_root(targetPlate, find_root);
                    if (rootI != rootTarget) {
                        plate_parent[rootI] = rootTarget;
                        plateSizes[rootTarget] += plateSizes[rootI];
                    }
                }
            }
        }

        // 8. Apply Union-Find updates to tiles and compact IDs for next pass
        std::vector<int> compact_map(num_plates, -1);
        int new_id_counter = 0;

        for (int i = 0; i < num_plates; ++i) {
            int root = find_root(i, find_root);
            if (compact_map[root] == -1) {
                compact_map[root] = new_id_counter++;
            }
        }

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int index = x + y * width;
                int currentId = tiles[index].GetPlateId();
                int root = find_root(currentId, find_root);
                int finalId = compact_map[root];
                tiles[index].SetPlateId(finalId);
            }
        }
    }

    // Final pass: Recalculate plate center positions for the red preview dots
    int final_max_id = 0;
    for (const auto& tile : tiles) {
        if (tile.GetPlateId() > final_max_id) final_max_id = tile.GetPlateId();
    }
    int final_plate_count = final_max_id + 1;

    plates = std::vector<Vector2>(final_plate_count, {0, 0});
    std::vector<float> sumX(final_plate_count, 0.0f);
    std::vector<float> sumY(final_plate_count, 0.0f);
    std::vector<int> count(final_plate_count, 0);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            int pid = tiles[index].GetPlateId();
            sumX[pid] += static_cast<float>(x);
            sumY[pid] += static_cast<float>(y);
            count[pid]++;
        }
    }

    for (int i = 0; i < final_plate_count; ++i) {
        if (count[i] > 0) {
            plates[i] = {sumX[i] / count[i], sumY[i] / count[i]};
        }
    }

    // Now that all the plates are settled, we give them some velocities
    plate_velocities = std::vector<Vector2>(plates.size());
    
    for (int i = 0; i < plate_velocities.size(); i++) {
        // Random angle and speed
        float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * PI;
        float speed = 0.5f + static_cast<float>(rand()) / RAND_MAX * 1.5f; // speed magnitude
        
        plate_velocities[i] = {
            cosf(angle) * speed,
            sinf(angle) * speed
        };
    }
}

void World::GenerateHeightmap()
{
    // =============================
    // === BASE PERLIN HEIGHTMAP ===
    siv::PerlinNoise::seed_type perlin_seed = static_cast<siv::PerlinNoise::seed_type>(seed);
    siv::PerlinNoise perlin{perlin_seed};
    
    float centerX = width * 0.5f;
    float centerY = height * 0.5f;
    float maxDist = std::sqrt(centerX * centerX + centerY * centerY);
    float fadeStart = 0.6f;
    
    // Use a temporary float buffer for the entire height calculation to allow smooth filtering
    std::vector<float> heightBuffer(width * height, 0.0f);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            float dampening = 1.0f;

            float nx = static_cast<float>(x) / width;
            float ny = static_cast<float>(y) / height;
            
            float dx = static_cast<float>(x) - centerX;
            float dy = static_cast<float>(y) - centerY;
            float t = std::sqrt(dx * dx + dy * dy) / maxDist;

            if (t > fadeStart) {
                dampening = 1.0f - (t - fadeStart) / (1.0f - fadeStart);
                dampening *= dampening;
            }
            
            float elevation = perlin.normalizedOctave2D_01(nx * 4.0f, ny * 4.0f, 3, 0.5f);
            elevation *= dampening;
            elevation = elevation * 120.0f;

            heightBuffer[index] = elevation;
        }
    }

    // =======================================================
    // === NOW WE APPLY TECTONICS (SYMMETRIC EDGE PAIRS)   ===
    // =======================================================
    std::vector<float> tectonicModifiers(width * height, 0.0f);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            int plateA = tiles[index].GetPlateId();

            int neighbors[2][2] = { {x + 1, y}, {x, y + 1} };
            for (auto& n : neighbors) {
                int nx = n[0];
                int ny = n[1];

                if (nx < width && ny < height) {
                    int nIndex = nx + ny * width;
                    int plateB = tiles[nIndex].GetPlateId();

                    if (plateA != plateB && plateA >= 0 && plateB >= 0 && 
                        plateA < plates.size() && plateB < plates.size()) 
                    {
                        Vector2 cA = plates[plateA];
                        Vector2 cB = plates[plateB];
                        Vector2 centerDir = { cB.x - cA.x, cB.y - cA.y };
                        float centerLen = std::sqrt(centerDir.x * centerDir.x + centerDir.y * centerDir.y);
                        if (centerLen < 0.0001f) continue;
                        centerDir.x /= centerLen;
                        centerDir.y /= centerLen;

                        Vector2 vA = plate_velocities[plateA];
                        Vector2 vB = plate_velocities[plateB];
                        Vector2 relVel = { vB.x - vA.x, vB.y - vA.y };

                        float approachSpeed = relVel.x * centerDir.x + relVel.y * centerDir.y;

                        float tectonicDelta = 0.0f;
                        if (approachSpeed > 0.0f) {
                            tectonicDelta = -approachSpeed * 1.5f; 
                        } else {
                            tectonicDelta = std::abs(approachSpeed) * 6.0f; 
                        }

                        float breakUpNoise = perlin.octave2D_01(static_cast<float>(x) * 0.2f, static_cast<float>(y) * 0.2f, 2);
                        if (approachSpeed <= 0.0f && breakUpNoise < 0.35f) {
                            tectonicDelta *= 0.2f; 
                        }

                        int spreadRadius = 6;
                        float edgeCenterX = (static_cast<float>(x) + nx) * 0.5f;
                        float edgeCenterY = (static_cast<float>(y) + ny) * 0.5f;

                        for (int sy = -spreadRadius; sy <= spreadRadius; ++sy) {
                            for (int sx = -spreadRadius; sx <= spreadRadius; ++sx) {
                                float targetX = edgeCenterX + static_cast<float>(sx);
                                float targetY = edgeCenterY + static_cast<float>(sy);

                                int gridX = static_cast<int>(std::round(targetX));
                                int gridY = static_cast<int>(std::round(targetY));

                                if (gridX >= 0 && gridX < width && gridY >= 0 && gridY < height) {
                                    float distDx = targetX - edgeCenterX;
                                    float distDy = targetY - edgeCenterY;
                                    float dist = std::sqrt(distDx * distDx + distDy * distDy);

                                    if (dist <= spreadRadius) {
                                        float rawFalloff = 1.0f - (dist / (spreadRadius + 1.0f));
                                        float falloff = rawFalloff * rawFalloff;
                                        
                                        int targetIndex = gridX + gridY * width;
                                        tectonicModifiers[targetIndex] += tectonicDelta * falloff;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Apply tectonic modifiers to height buffer
    for (int i = 0; i < width * height; ++i) {
        heightBuffer[i] += tectonicModifiers[i];
    }

    // ==============================================
    // === INLAND PLATE SWELL & INTERNAL MOUNTAINS ===
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            int currentPlate = tiles[index].GetPlateId();

            if (currentPlate >= 0 && (currentPlate % 2 != 0)) {
                float minDistToBorder = 999.0f;
                int searchRadius = 15; 

                for (int sy = -searchRadius; sy <= searchRadius; ++sy) {
                    for (int sx = -searchRadius; sx <= searchRadius; ++sx) {
                        int nx = x + sx;
                        int ny = y + sy;
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            if (tiles[nx + ny * width].GetPlateId() != currentPlate) {
                                float dist = std::sqrt(static_cast<float>(sx * sx + sy * sy));
                                if (dist < minDistToBorder) {
                                    minDistToBorder = dist;
                                }
                            }
                        }
                    }
                }

                float inlandFactor = std::clamp(minDistToBorder / 10.0f, 0.0f, 1.0f);
                inlandFactor = inlandFactor * inlandFactor * (3.0f - 2.0f * inlandFactor);

                float base_continental_increment = 4;
                float domeBoost = inlandFactor * 2.0f + base_continental_increment;

                float nx = static_cast<float>(x) / width;
                float ny = static_cast<float>(y) / height;
                float internalNoise = perlin.normalizedOctave2D_01(nx * 12.0f, ny * 12.0f, 3, 0.5f);

                float internalMountains = internalNoise * inlandFactor * 48.0f;

                heightBuffer[index] += domeBoost + internalMountains;
            }
        }
    }

    // ============================================================
    // === FINAL SMOOTHING / DIFFUSION PASS (ELIMINATES CLIFFS) ===
    std::vector<float> smoothedBuffer = heightBuffer;
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            float sum = 0.0f;
            // 3x3 Box blur kernel to melt away any lingering step-cliffs or jagged neighbors
            for (int sy = -1; sy <= 1; ++sy) {
                for (int sx = -1; sx <= 1; ++sx) {
                    sum += heightBuffer[(x + sx) + (y + sy) * width];
                }
            }
            smoothedBuffer[x + y * width] = sum / 9.0f;
        }
    }

    // Assign final clamped values to tiles
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            float finalHeight = std::clamp(smoothedBuffer[index], 0.0f, 255.0f);
            tiles[index].SetHeight(static_cast<unsigned char>(finalHeight));
        }
    }
}

void World::GenerateClimate()
{
    siv::PerlinNoise::seed_type perlin_seed = static_cast<siv::PerlinNoise::seed_type>(seed + 12345);
    siv::PerlinNoise perlin{perlin_seed};

    // ===============
    // === 1. WIND ===
    // ===============
    struct WindSource {
        Vector2 pos;
        Vector2 vel;
    };

    std::vector<WindSource> windSources;
    int numSources = std::max(2, (width * height) / 2500);
    
    srand(seed);
    for (int i = 0; i < numSources; ++i) {
        Vector2 origin = { static_cast<float>(rand() % width), static_cast<float>(rand() % height) };
        float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * PI;
        float speed = 0.5f + static_cast<float>(rand()) / RAND_MAX * 1.5f; 
        windSources.push_back({ origin, { cosf(angle) * speed, sinf(angle) * speed } });
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            float nx = static_cast<float>(x) / width;
            float ny = static_cast<float>(y) / height;

            Vector2 interpolatedWind = {0.0f, 0.0f};
            float totalWeight = 0.0f;

            for (const auto& src : windSources) {
                float dx = src.pos.x - static_cast<float>(x);
                float dy = src.pos.y - static_cast<float>(y);
                float distSq = (dx * dx) + (dy * dy) + 1.0f;
                float weight = 1.0f / distSq;
                interpolatedWind.x += src.vel.x * weight;
                interpolatedWind.y += src.vel.y * weight;
                totalWeight += weight;
            }

            if (totalWeight > 0.0f) {
                interpolatedWind.x /= totalWeight;
                interpolatedWind.y /= totalWeight;
            }

            if (tiles[index].GetHeight() <= sea_level) {
                float multiplier = 1 + (sea_level - tiles[index].GetHeight()) / sea_level;
                interpolatedWind.x *= multiplier;
                interpolatedWind.y *= multiplier;
            }

            float noiseAngle = perlin.normalizedOctave2D_01(nx * 3.0f, ny * 3.0f, 2, 0.5f) * 2.0f * PI;
            Vector2 finalWind = {
                interpolatedWind.x + cosf(noiseAngle) * 0.5f,
                interpolatedWind.y + sinf(noiseAngle) * 0.5f
            };

            tiles[index].SetWind(finalWind);
        }
    }

    // ============================================
    // === 2. TEMPERATURE (With Wind Advection) ===
    // ============================================
    float equatorY = height * 0.5f;
    float maxDistFromEquator = height * 0.5f;

    float polarWidth     = 0.20f; 
    float temperateWidth = 0.50f; 
    float tropicalWidth  = 0.30f; 

    float polarTempMin     = MIN_TEMP; 
    float polarTempMax     = -5.0f;
    float temperateTempMin = -5.0f;
    float temperateTempMax = 25.0f;
    float tropicalTempMin  = 25.0f;
    float tropicalTempMax  = MAX_TEMP; 

    float thresholdPolar     = polarWidth;
    float thresholdTemperate = thresholdPolar + temperateWidth;

    // Temporary buffer to calculate base temperatures before wind mixing
    std::vector<float> tempBuffer(width * height, 0.0f);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = x + y * width;
            float nx = static_cast<float>(x) / width;
            float ny = static_cast<float>(y) / height;

            float bandWarp = (perlin.normalizedOctave2D_01(nx * 2.0f, ny * 2.0f, 2, 0.5f) - 0.5f) * 0.3f;
            float distFromEquator = std::abs(static_cast<float>(y) - equatorY);
            float poleDist = 1.0f - (distFromEquator / maxDistFromEquator);
            poleDist = std::clamp(poleDist + bandWarp, 0.0f, 1.0f);

            float baseTemp = 0.0f;
            if (poleDist <= thresholdPolar) {
                float t = poleDist / thresholdPolar;
                baseTemp = polarTempMin + t * (polarTempMax - polarTempMin);
            } 
            else if (poleDist <= thresholdTemperate) {
                float t = (poleDist - thresholdPolar) / temperateWidth;
                baseTemp = temperateTempMin + t * (temperateTempMax - temperateTempMin);
            } 
            else {
                float t = (poleDist - thresholdTemperate) / tropicalWidth;
                baseTemp = tropicalTempMin + t * (tropicalTempMax - tropicalTempMin);
            }

            float localNoise = (perlin.normalizedOctave2D_01(nx * 8.0f, ny * 8.0f, 1, 0.5f) - 0.5f) * 2.0f; 
            float estimatedTemp = baseTemp + localNoise;

            float tileHeight = static_cast<float>(tiles[index].GetHeight());
            float heightAboveSea = std::max(0.0f, tileHeight - static_cast<float>(sea_level));
            float maxPossibleHeightAboveSea = 255.0f - static_cast<float>(sea_level);
            float hNorm = heightAboveSea / maxPossibleHeightAboveSea;

            float maxTemperatureDrop = 45.0f; 
            float tempDrop = std::pow(hNorm, 2.2f) * maxTemperatureDrop;

            tempBuffer[index] = std::clamp(estimatedTemp - tempDrop, MIN_TEMP, MAX_TEMP);
        }
    }

    // Wind Advection & Thermal Diffusion Loop
    std::vector<float> currentTempBuffer = tempBuffer;
    std::vector<float> nextTempBuffer = tempBuffer;

    for (int pass = 0; pass < 100; pass++) {
        for (int y = 0; y < height - 1; y++) {
            for (int x = 0; x < width - 1; x++) {
                int index = x + y * width;
                Vector2 wind = tiles[index].GetWind();
                
                // Look upstream based on wind direction
                float upwindX = static_cast<float>(x) - wind.x * 0.5f;
                float upwindY = static_cast<float>(y) - wind.y * 0.5f;
                
                int ux = static_cast<int>(std::clamp(std::round(upwindX), 0.0f, static_cast<float>(width - 1)));
                int uy = static_cast<int>(std::clamp(std::round(upwindY), 0.0f, static_cast<float>(height - 1)));
                int upwindIndex = ux + uy * width;

                float localT = currentTempBuffer[index];
                float upstreamT = currentTempBuffer[upwindIndex];
                
                // Include local 3x3 diffusion to smooth temperatures organically over multiple passes
                float neighborSum = 0.0f;
                int neighborCount = 0;
                for (int sy = -1; sy <= 1; sy++) {
                    for (int sx = -1; sx <= 1; sx++) {
                        if (sx < 0 || sy < 0 || sx > width || sy > height){
                            continue;
                        }
                        neighborSum += currentTempBuffer[(x + sx) + (y + sy) * width];
                        neighborCount++;
                    }
                }
                float avgNeighborT = neighborSum / neighborCount;

                // Blend advection and diffusion
                float advectionVal = std::lerp(localT, upstreamT, 0.25f);
                nextTempBuffer[index] = std::lerp(advectionVal, avgNeighborT, 0.1f);
            }
        }
        currentTempBuffer = nextTempBuffer;
    }

    for (int i = 0; i < 2; i++) {
        for (int y = 1; y < height - 1; y++) {
            for (int x = 1; x < width - 1; x++) {
                float sum = 0.0f;
                int count = 0;
                for (int sy = -1; sy <= 1; sy++) {
                    for (int sx = -1; sx <= 1; sx++) {
                        sum += currentTempBuffer[(x + sx) + (y + sy) * width];
                        count++;
                    }
                }
                nextTempBuffer[x + y * width] = sum / static_cast<float>(count);
            }
        }
        currentTempBuffer = nextTempBuffer;
    }

    // Assign final smoothed temperatures to tiles
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = x + y * width;
            tiles[index].SetTemperature(std::clamp(currentTempBuffer[index], MIN_TEMP, MAX_TEMP));
        }
    }

    // =====================================================
    // === 3. PERLIN RAINMAP + WIND/OROGRAPHIC MODIFIERS ===
    // =====================================================
    siv::PerlinNoise::seed_type rain_seed = static_cast<siv::PerlinNoise::seed_type>(seed);
    siv::PerlinNoise perlinRain{rain_seed};

    std::vector<float> rainfallBuffer(width * height, 0.0f);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            if (tiles[index].GetHeight() <= sea_level) continue;

            float nx = static_cast<float>(x) / width;
            float ny = static_cast<float>(y) / height;

            // A. Base Perlin Noise Continental Rainfall (Broad, organic climate zones)
            float baseRain = perlinRain.normalizedOctave2D_01(nx * 3.0f, ny * 3.0f, 3, 0.5f);
            baseRain = 2.5 * std::pow(baseRain, 2.5);

            // B. Wind & Orographic Modification
            float mod = 0.0f;
            if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                float hRight = static_cast<float>(tiles[(x + 1) + y * width].GetHeight());
                float hLeft  = static_cast<float>(tiles[(x - 1) + y * width].GetHeight());
                float hDown  = static_cast<float>(tiles[x + (y + 1) * width].GetHeight());
                float hUp    = static_cast<float>(tiles[x + (y - 1) * width].GetHeight());

                float gradX = (hRight - hLeft) * 0.5f;
                float gradY = (hDown - hUp) * 0.5f;

                Vector2 wind = tiles[index].GetWind();
                float windSpeed = std::sqrt(wind.x * wind.x + wind.y * wind.y);

                if (windSpeed > 0.0001f) {
                    float dirX = wind.x / windSpeed;
                    float dirY = wind.y / windSpeed;
                    // Positive = wind hits uphill slope (extra rain/windward boost)
                    // Negative = wind blows downhill (rain shadow/leeward dry spot)
                    float alignment = (dirX * gradX + dirY * gradY);
                    mod = alignment * 0.025f; 
                }
            }

            // Combine base noise with wind modifier
            rainfallBuffer[index] = std::clamp(baseRain + mod, 0.0f, 1.0f);
        }
    }

    // ============================================
    // === 4. SMOOTHING & TARGET MEAN BALANCING ===
    // ============================================
    for (int pass = 0; pass < 3; ++pass) {
        std::vector<float> smoothed = rainfallBuffer;
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                int index = x + y * width;
                if (tiles[index].GetHeight() <= sea_level) continue;

                float sum = 0.0f;
                int count = 0;
                for (int sy = -1; sy <= 1; ++sy) {
                    for (int sx = -1; sx <= 1; ++sx) {
                        int nIndex = (x + sx) + (y + sy) * width;
                        if (tiles[nIndex].GetHeight() > sea_level) {
                            sum += rainfallBuffer[nIndex];
                            count++;
                        }
                    }
                }
                if (count > 0) smoothed[index] = sum / static_cast<float>(count);
            }
        }
        rainfallBuffer = smoothed;
    }

    // Assign final calibrated values
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            if (tiles[index].GetHeight() <= sea_level) continue;

            tiles[index].SetRainfall(static_cast<unsigned char>(std::clamp(rainfallBuffer[index] * 255.0f, 0.0f, 255.0f)));
        }
    }
}


void World::GenerateRivers()
{
    // =========================================================
    // === PHASE 1: DENSE MULTI-SOURCE CONTINENT SEEDING     ===
    // =========================================================
    std::vector<bool> visited(width * height, false);
    std::vector<std::pair<int, int>> chosenSeeds;

    // 1. Calculate distance from sea for every tile to find true inland cores
    std::vector<float> distToCoast(width * height, 0.0f);
    std::vector<int> coastQueue;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            if (tiles[index].GetHeight() <= sea_level) {
                coastQueue.push_back(index);
                visited[index] = true;
            }
        }
    }

    size_t coastHead = 0;
    while (coastHead < coastQueue.size()) {
        int cIdx = coastQueue[coastHead++];
        int cx = cIdx % width;
        int cy = cIdx / width;
        float currentDist = distToCoast[cIdx];

        int dxs[] = { -1, 1, 0, 0 };
        int dys[] = { 0, 0, -1, 1 };
        for (int i = 0; i < 4; ++i) {
            int nx = cx + dxs[i];
            int ny = cy + dys[i];
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                int nIdx = nx + ny * width;
                if (!visited[nIdx]) {
                    visited[nIdx] = true;
                    distToCoast[nIdx] = currentDist + 1.0f;
                    coastQueue.push_back(nIdx);
                }
            }
        }
    }

    // Reset visited for landmass flood-filling
    std::fill(visited.begin(), visited.end(), false);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            if (tiles[index].GetHeight() <= sea_level || visited[index]) continue;

            // Flood-fill to extract the landmass
            std::vector<int> landmass;
            std::vector<int> queue;
            queue.push_back(index);
            visited[index] = true;

            float maxDistOnContinent = 0.0f;
            size_t head = 0;
            while (head < queue.size()) {
                int cIdx = queue[head++];
                landmass.push_back(cIdx);
                if (distToCoast[cIdx] > maxDistOnContinent) {
                    maxDistOnContinent = distToCoast[cIdx];
                }

                int cx = cIdx % width;
                int cy = cIdx / width;
                int dxs[] = { -1, 1, 0, 0 };
                int dys[] = { 0, 0, -1, 1 };
                for (int i = 0; i < 4; ++i) {
                    int nx = cx + dxs[i];
                    int ny = cy + dys[i];
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        int nIdx = nx + ny * width;
                        if (tiles[nIdx].GetHeight() > sea_level && !visited[nIdx]) {
                            visited[nIdx] = true;
                            queue.push_back(nIdx);
                        }
                    }
                }
            }

            // If the landmass is valid, score tiles based on Elevation, Distance to Coast, and Rainfall
            if (landmass.size() > 15 && maxDistOnContinent > 3.0f) {
                std::sort(landmass.begin(), landmass.end(), [&](int a, int b) {
                    float elevA = static_cast<float>(tiles[a].GetHeight()) / 255.0f;
                    float elevB = static_cast<float>(tiles[b].GetHeight()) / 255.0f;
                    
                    float distA = distToCoast[a] / maxDistOnContinent;
                    float distB = distToCoast[b] / maxDistOnContinent;
                    
                    float rainA = static_cast<float>(tiles[a].GetRainfall()) / 255.0f;
                    float rainB = static_cast<float>(tiles[b].GetRainfall()) / 255.0f;

                    // Composite score: 30% Elevation, 40% Distance-to-Coast, 30% Rainfall
                    float scoreA = (elevA * 0.3f) + (distA * 0.4f) + (rainA * 0.3f);
                    float scoreB = (elevB * 0.3f) + (distB * 0.4f) + (rainB * 0.3f);

                    return scoreA > scoreB; // Highest composite score first
                });

                int targetSources = std::max(1, static_cast<int>(landmass.size()) / 150);
                int sourcesSpawned = 0;

                for (int idx : landmass) {
                    if (sourcesSpawned >= targetSources) break;

                    int px = idx % width;
                    int py = idx / width;

                    bool tooClose = false;
                    for (const auto& seed : chosenSeeds) {
                        int distSq = (px - seed.first) * (px - seed.first) + (py - seed.second) * (py - seed.second);
                        if (distSq < 36) { // Minimum 6 tiles apart
                            tooClose = true;
                            break;
                        }
                    }

                    if (!tooClose) {
                        chosenSeeds.push_back({px, py});
                        sourcesSpawned++;
                    }
                }
            }
        }
    }
    // =========================================================
    // === PHASE 2: D8 DOWNHILL ROUTING WITH PIT-ESCAPING    ===
    // =========================================================
    for (const auto& seed : chosenSeeds) {
        int cx = seed.first;
        int cy = seed.second;
        
        std::vector<std::pair<int, int>> currentPath;
        int safetyCounter = 0;
        int maxSteps = width * height; 
        bool reachedSea = false;

        while (safetyCounter < maxSteps) {
            int currentIndex = cx + cy * width;
            currentPath.push_back({cx, cy});

            if (tiles[currentIndex].GetHeight() <= sea_level) {
                reachedSea = true;
                break;
            }

            float lowestHeight = 9999.0f;
            int bestX = cx;
            int bestY = cy;
            bool isDiagonal = false;

            int dxs[] = { -1, 1,  0, 0, -1, -1,  1,  1 };
            int dys[] = {  0, 0, -1, 1, -1,  1, -1,  1 };

            for (int i = 0; i < 8; ++i) {
                int nx = cx + dxs[i];
                int ny = cy + dys[i];

                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                    int nIndex = nx + ny * width;
                    float nHeight = static_cast<float>(tiles[nIndex].GetHeight());

                    if (nHeight < lowestHeight) {
                        lowestHeight = nHeight;
                        bestX = nx;
                        bestY = ny;
                        isDiagonal = (dxs[i] != 0 && dys[i] != 0);
                    }
                } else {
                    bestX = std::clamp(nx, 0, width - 1);
                    bestY = std::clamp(ny, 0, height - 1);
                    reachedSea = true;
                    break;
                }
            }

            // PIT ESCAPE: If trapped on a flat spot (no lower neighbor), force step toward the nearest lower/ocean tile
            if (bestX == cx && bestY == cy) {
                bool escaped = false;
                // Search slightly broader 5x5 neighborhood for lower ground
                for (int sy = -2; sy <= 2; ++sy) {
                    for (int sx = -2; sx <= 2; ++sx) {
                        int nx = cx + sx;
                        int ny = cy + sy;
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            int nIndex = nx + ny * width;
                            if (tiles[nIndex].GetHeight() < tiles[currentIndex].GetHeight()) {
                                bestX = cx + (sx > 0 ? 1 : (sx < 0 ? -1 : 0));
                                bestY = cy + (sy > 0 ? 1 : (sy < 0 ? -1 : 0));
                                escaped = true;
                                break;
                            }
                        }
                    }
                    if (escaped) break;
                }
                if (!escaped) break; // Truly trapped at a global minimum
            }

            if (isDiagonal) {
                currentPath.push_back({bestX, cy}); 
            }

            cx = bestX;
            cy = bestY;
            safetyCounter++;
        }

        // Carve path to sea level
        if (reachedSea) {
            for (const auto& pt : currentPath) {
                int pIndex = pt.first + pt.second * width;
                if (tiles[pIndex].GetHeight() > sea_level) {
                    tiles[pIndex].SetHeight(sea_level);
                }
            }
        }
    }
}

void World::PopulateTiles()
{
    int dxs[] = { -1, 1,  0, 0, -1, -1,  1,  1 };
    int dys[] = {  0, 0, -1, 1, -1,  1, -1,  1 };

    srand(seed);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = x + y * width;
            Tile &tile = tiles[index];

            // Oceans have zero fertility and lumber
            if (tile.GetHeight() <= sea_level) {
                tile.SetSoilFertility(0.0f);
                tile.SetMaxLumber(0.0f);
                tile.SetLumberRegen(0.0f);
                tile.SetCurrentLumber(0.0f);
                continue;
            }

            float norm_rain = static_cast<float>(tile.GetRainfall()) / 255.0f;
            float temp_c = tile.GetTemperature();
            float norm_temp = (temp_c - MIN_TEMP) / (MAX_TEMP - MIN_TEMP);
            
            // Elevation normal
            float norm_elevation = static_cast<float>(tile.GetHeight() - sea_level) / (255.0f - static_cast<float>(sea_level));
            // Elevation factor: near sea level gets a full multiplier, high mountains get penalized
            float elev_factor = 1.0f - std::clamp(norm_elevation * 0.8f, 0.0f, 1.0f);

            // =====================================
            // === 1. SOIL FERTILITY CALCULATION ===
            // =====================================
            float target_temp_norm = (25.0f - MIN_TEMP) / (MAX_TEMP - MIN_TEMP); // 0.8f
            float target_rain = 0.6f;

            float curve_tolerance = 3.0f;
            float temp_diff = norm_temp - target_temp_norm;
            float temp_suitability = std::max(0.0f, 1.0f - (temp_diff * temp_diff * curve_tolerance));

            float rain_diff = norm_rain - target_rain;
            float rain_suitability = std::max(0.0f, 1.0f - (rain_diff * rain_diff * curve_tolerance));

            float base_fertility = temp_suitability * rain_suitability * elev_factor;

            // Steepness & Roughness Calculation
            float totalDiff = 0.0f;
            int validNeighbors = 0;
            float currentH = static_cast<float>(tile.GetHeight());

            for (int i = 0; i < 8; ++i) {
                int nx = x + dxs[i];
                int ny = y + dys[i];

                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                    int nIndex = nx + ny * width;
                    float neighborH = static_cast<float>(tiles[nIndex].GetHeight());

                    // Do not count water tiles
                    if (neighborH <= sea_level) continue;

                    totalDiff += std::abs(neighborH - currentH);
                    validNeighbors++;
                }
            }

            float avgSlope = validNeighbors > 0 ? (totalDiff / validNeighbors) : 0.0f;
            float mountain_threshold = 15.0f;
            float norm_steepness = std::clamp(avgSlope / mountain_threshold, 0.0f, 1.0f);
            float steepness_multiplier = 1.15f - (0.40f * norm_steepness);
            
            float final_fertility = std::clamp(base_fertility * steepness_multiplier, 0.0f, 1.0f);
            tile.SetSoilFertility(final_fertility);

            // ============================================
            // === 2. LUMBER & FOREST CARRYING CAPACITY ===
            // ============================================
            float max_lumber = 0.0f;
            float lumber_regen = 0.0f;

            // Hard cutoffs for barren/frozen zones
            if (norm_rain > 0.005f && temp_c > -12.0f && norm_elevation < 0.75f) {
                // A. Rainfall Factor: Linear scaling
                float rain_factor = norm_rain;

                // B. Temperature Factor: Broad bell curve supporting boreal (-5°C) to tropical (30°C+)
                float temp_pos = (temp_c - (-10.0f)) / (36.0f - (-10.0f));
                float forest_temp_factor = 1.0f - std::pow(std::clamp(temp_pos, 0.0f, 1.0f) - 0.6f, 2.0f) * 2.5f;
                forest_temp_factor = std::clamp(forest_temp_factor, 0.1f, 1.0f);

                // C. Wind Exposure Factor: High wind magnitude reduces sustainable density
                Vector2 wind = tile.GetWind();
                float wind_mag = std::sqrt(wind.x * wind.x + wind.y * wind.y);
                float wind_factor = std::clamp(1.0f - (wind_mag * 0.09f), 0.4f, 1.0f);

                // D. Tree Line Factor: Thin out at high elevations
                float tree_line_factor = 1.0f - std::clamp(norm_elevation / 0.75f, 0.0f, 1.0f);

                // E. Soil Fertility Influence: Fertile land supports denser woodlands
                float fertility_influence = 0.5f + (0.5f * final_fertility);

                // Combine into overall ecological multiplier
                float eco_multiplier = rain_factor * forest_temp_factor * wind_factor * tree_line_factor * fertility_influence;
                
                // Scale base max lumber
                float max_lumber_modifier = 500;
                max_lumber = max_lumber_modifier * eco_multiplier;
                max_lumber = std::clamp(max_lumber, 0.0f, 1500.0f);
                
                // Growth efficiency is lower in cold climates and faster in warm climates, the more rain the faster regrowth
                float growth_efficiency = norm_temp * 0.35f + norm_rain * 0.65f;
                growth_efficiency = std::clamp(growth_efficiency, 0.01f, 1.0f);
                
                // Base regen ratio
                float lumber_regeneration_modifier = 0.00172f;
                lumber_regen = max_lumber * lumber_regeneration_modifier * growth_efficiency;
                
            }
            
            tile.SetMaxLumber(max_lumber);
            tile.SetLumberRegen(lumber_regen);
            tile.SetCurrentLumber(max_lumber); // Initialize tile fully forested


            // ================
            // === 3. STONE ===
            // ================
            unsigned int stone_amount = 0;
            
            // Stone primarily forms in elevated, rugged, and barren terrain
            float barrenness = 1.0f - final_fertility;
            
            // Composite score weighting elevation, slope steepness, and barren soil
            float min_stone = 1e-10f;
            float stone_score = min_stone + (norm_elevation * 0.4f) + (norm_steepness * 0.4f) + (barrenness * 0.2f);
            stone_score = std::clamp(stone_score, 0.0f, 1.0f);

            // Scale exponentially so extreme jagged peaks hit rich status (>100M)
            long long stone_multiplier = 5e8;
            stone_amount = (std::pow(stone_score, 8.0f) * stone_multiplier);
            //stone_amount = stone_score * stone_multiplier;
            if (stone_amount > UINT32_MAX) {
                stone_amount = UINT32_MAX;
            }

            tile.SetStone(stone_amount);

            // ==================
            // === 4. METALS  ===
            // ==================
            unsigned int metal_amount = 0;
            
            // Perlin noise for broad, organic distribution of mineral zones
            static siv::PerlinNoise perlin{static_cast<siv::PerlinNoise::seed_type>(seed - 12973)};
            float nx_metal = static_cast<float>(x) / width;
            float ny_metal = static_cast<float>(y) / height;
            float metal_noise = perlin.normalizedOctave2D_01(nx_metal * 5.0f, ny_metal * 5.0f, 3, 0.5f);
            metal_noise = std::max(0.0f, metal_noise - 0.5f); // Noise floor

            // Terrain weighting: Ores favor rugged, barren, and elevated regions
            float metal_score = (norm_steepness * 0.4f) + (barrenness * 0.3f) + (norm_elevation * 0.3f);
            
            // Combine noise with terrain score
            float final_metal_factor = metal_noise * metal_score;

            int metal_multiplier = 3e7; 
            metal_amount = (std::pow(final_metal_factor, 2.0f) * metal_multiplier);
                
            metal_amount = std::min(UINT32_MAX, metal_amount);

            tile.SetMetals(metal_amount);
            
            // ==================
            // === 5. COAL    ===
            // ==================
            unsigned int coal_amount = 0;

            // Coal forms in flat, low-elevation basins with high biomass/lumber potential
            float lowland_factor = 1.0f - std::clamp(norm_elevation * 1.5f, 0.0f, 1.0f);
            float flatness_factor = 1.0f - norm_steepness;

            // Normalize max lumber as a proxy for organic plant/swamp material history
            float norm_lumber = std::clamp(tile.GetMaxLumber() / 1500.0f, 0.0f, 1.0f);

            // Perlin noise for broad, organic regional coal deposit fields
            float nx_coal = static_cast<float>(x) / width;
            float ny_coal = static_cast<float>(y) / height;
            float coal_noise = perlin.normalizedOctave2D_01(nx_coal * 4.0f, ny_coal * 4.0f, 3, 0.5f);
            coal_noise = std::max(0.0f, coal_noise - 0.35f); // Threshold to create distinct coal basins

            // Combine factors: requires low elevation, flat terrain, and vegetation/lumber background
            float coal_score = lowland_factor * flatness_factor * (0.3f + 0.7f * norm_lumber) * coal_noise;
            coal_score = std::clamp(coal_score, 0.0f, 1.0f);

            unsigned int coal_multiplier = 4e6;
            unsigned int calculated_coal = static_cast<long long>(std::pow(coal_score, 2.5f) * coal_multiplier);

            coal_amount = std::min(UINT32_MAX, calculated_coal);
            tile.SetCoal(coal_amount);


            // ==================
            // === 6. MINERALS ===
            // ==================
            unsigned int mineral_amount = 0;


            // Scan 5x5 neighborhood to find distance to the closest water tile
            float closest_water_dist = 999.0f;
            for (int sy = -2; sy <= 2; ++sy) {
                for (int sx = -2; sx <= 2; ++sx) {
                    int nx = x + sx;
                    int ny = y + sy;
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        if (tiles[nx + ny * width].GetHeight() <= sea_level) {
                            float dist = std::sqrt(static_cast<float>(sx * sx + sy * sy));
                            if (dist < closest_water_dist) {
                                closest_water_dist = dist;
                            }
                        }
                    }
                }
            }

            // Gentle water distance multiplier (ranges smoothly from 1.4x next to water down to 1.0x away)
            float water_mult = 1.0f;
            if (closest_water_dist <= 3.0f) {
                float t = closest_water_dist / 3.0f;
                water_mult = std::lerp(1.4f, 1.0f, t);
            }

            // Perlin noise for broad regional mineral basins
            float nx_min = static_cast<float>(x) / width;
            float ny_min = static_cast<float>(y) / height;
            float mineral_noise = perlin.normalizedOctave2D_01(nx_min * 5.0f, ny_min * 5.0f, 3, 0.5f);
            //mineral_noise = std::max(0.0f, mineral_noise - 0.3f);

            // Combine factors with the smooth water multiplier
            float mineral_score = lowland_factor * flatness_factor * mineral_noise * water_mult;
            mineral_score = std::clamp(mineral_score, 0.0f, 1.0f);

            unsigned int mineral_multiplier = 1e7;
            unsigned int calculated_minerals = static_cast<unsigned int>(std::pow(mineral_score, 2.2f) * mineral_multiplier);

            mineral_amount = std::min(UINT32_MAX, calculated_minerals);
            tile.SetMinerals(mineral_amount);


        }
    }

    // Smoothing pass for stone
    int stone_smoothing_iterations = 3;
    std::vector<unsigned int> stoneBuffer(width * height);
    
    for (int i = 0; i < stone_smoothing_iterations; i++){
        // Copy current stone values to buffer
        for (int idx = 0; idx < width * height; ++idx) {
            stoneBuffer[idx] = tiles[idx].GetStone();
        }

        for (int y = 0; y < height; ++y){
            for (int x = 0; x < width; ++x){
                unsigned long long sum = 0;
                int count = 0;
                
                // Correct 3x3 box blur neighbor lookup
                for (int sy = -1; sy <= 1; ++sy) {
                    for (int sx = -1; sx <= 1; ++sx) {
                        int nx = x + sx;
                        int ny = y + sy;
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            sum += stoneBuffer[nx + ny * width];
                            count++;
                        }
                    }
                }
                
                int index = x + y * width;
                tiles[index].SetStone(static_cast<unsigned int>(sum / count));
            }
        }
    }
}


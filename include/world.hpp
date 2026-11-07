#ifndef WORLD_H
#define WORLD_H

#include "tile.hpp"
#include <vector>

class World{
public:
    World(int width, int height, int seed, unsigned char sea_level);
    
    void Generate();
    void Regenerate(int seed);

    int GetWidth() const;
    int GetHeight() const;
    int GetSeed() const;
    
    void PreviewTectonics() const;
    void PreviewHeight() const;
    void PreviewTemperature() const;
    void PreviewWinds() const;
    void PreviewRainfall() const;
    void PreviewSoilFertility() const;
    void PreviewLumber() const;
    void PreviewStone() const;
    void PreviewMetals() const;
    void PreviewCoal() const;
    void PreviewMinerals() const;

    void SetMaxTemp(float temp);
    void SetMinTemp(float temp);

    float GetMaxTemp() const;
    float GetMinTemp() const;

    unsigned char GetSeaLevel() const;
    
    const Tile& GetTile(int x, int y) const;
    Tile& GetTile(int x, int y);
    const std::vector<Tile>& GetTiles() const;
    std::vector<Tile>& GetTiles();
    

    private:
    
    const int width, height;
    const unsigned char sea_level;
    float MAX_TEMP, MIN_TEMP;
    std::vector<Tile> tiles;
    std::vector<Vector2> plates;
    std::vector<Vector2> plate_velocities;
    int seed;
    
    void CreateTiles();
    void GenerateTectonics();
    void GenerateHeightmap();
    void GenerateClimate();
    void GenerateRivers();
    void PopulateTiles();
};

#endif

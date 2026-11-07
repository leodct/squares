#ifndef TILE_H
#define TILE_H

#include <raylib.h>

const int TILE_SIZE = 16; // Pixels

class Tile {
public:
    Tile(unsigned char height = 0);
    
    void SetHeight(unsigned char _height);
    unsigned char GetHeight() const;
    
    void Step(int days = 1);

    // ===============
    // === TERRAIN ===
    void SetPlateId(int plate_id);
    int GetPlateId() const;

    void SetRainfall(unsigned char rainfall);
    unsigned char GetRainfall() const;

    void SetTemperature(float temperature);
    float GetTemperature() const;

    void SetWind(Vector2 wind);
    const Vector2& GetWind() const;

    // =================
    // === RESOURCES ===
    // Lumber
    void SetCurrentLumber(float lumber);
    float GetCurrentLumber() const;

    void SetMaxLumber(float max_lumber);
    float GetMaxLumber() const;

    void SetLumberRegen(float rate);
    float GetLumberRegenerationRate() const;

    // Metals
    void SetMetals(unsigned int metals);
    unsigned int GetMetals() const;
    
    // Coal
    void SetCoal(unsigned int coal);
    unsigned int GetCoal() const;
    
    // Stone
    void SetStone(unsigned int stone);
    unsigned int GetStone() const;
    
    // Minerals
    void SetMinerals(unsigned int minerals);
    unsigned int GetMinerals() const;
    
    // Agriculture
    void SetSoilFertility(float fertility);
    float GetSoilFertility() const;


    // ===============
    // === TERRAIN ===

    // City administration
    void SetCity(int id);
    int GetCity() const;
    
    void Draw() const;

private:
    // === WORLD ===
    // 0-255 low-high.
    unsigned char height;
    // 0-255 low-high.
    unsigned char rainfall;
    // In celsius because celsius are objectively superior.
    float temperature;
    // Wind vector.
    Vector2 wind;           
    // Tectonic plate to which this tile belongs.
    int plate_id;

    /// === RESOURCES ===
    // 0-1 Low-high.
    float soil_fertility;
    // Current amount of lumber.
    float current_lumber;
    // How much lumber the tile regains per day (turn).
    float lumber_regeneration;
    // The maximum mount of lumber this tile can have.
    float max_lumber;
    // Main non-renewable resources
    unsigned int metals;
    unsigned int coal;
    unsigned int stone;
    unsigned int minerals;

    // ID of the city administering this tile.
    int city_id;
};

#endif

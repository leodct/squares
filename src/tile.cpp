#include "tile.hpp"

Tile::Tile(unsigned char _height) : 
    height(_height), plate_id(0), rainfall(128), temperature(20), current_lumber(0),
    max_lumber(0), lumber_regeneration(0), soil_fertility(0.5f), metals(0), coal(0),
    stone(0), minerals(0), city_id(-1)
{
}

void Tile::SetHeight(unsigned char _height)
{
    height = _height;
}

unsigned char Tile::GetHeight() const
{
    return height;
}

void Tile::Step(int days)
{
    // Increase lumber
    ((current_lumber += lumber_regeneration * days) >= max_lumber) ? current_lumber = max_lumber : current_lumber;
}

int Tile::GetPlateId() const
{
    return plate_id;
}

void Tile::SetRainfall(unsigned char _rainfall)
{
    rainfall = _rainfall;
}

unsigned char Tile::GetRainfall() const
{
    return rainfall;
}

void Tile::SetTemperature(float _temperature)
{
    temperature = _temperature;
}

float Tile::GetTemperature() const
{
    return temperature;
}

void Tile::SetWind(Vector2 _wind)
{
    wind = _wind;
}

const Vector2 &Tile::GetWind() const
{
    return wind;
}

void Tile::SetCurrentLumber(float lumber)
{
    current_lumber = lumber;
}

float Tile::GetCurrentLumber() const
{
    return current_lumber;
}

void Tile::SetMaxLumber(float _max_lumber)
{
    max_lumber = _max_lumber;
}

float Tile::GetMaxLumber() const
{
    return max_lumber;
}

void Tile::SetLumberRegen(float rate)
{
    lumber_regeneration = rate;
}

float Tile::GetLumberRegenerationRate() const
{
    return lumber_regeneration;
}

void Tile::SetMetals(unsigned int _metals)
{
    metals = _metals;
}

unsigned int Tile::GetMetals() const
{
    return metals;
}

void Tile::SetCoal(unsigned int _coal)
{
    coal = _coal;
}

unsigned int Tile::GetCoal() const
{
    return coal;
}

void Tile::SetStone(unsigned int _stone)
{
    stone = _stone;
}

unsigned int Tile::GetStone() const
{
    return stone;
}

void Tile::SetMinerals(unsigned int _minerals)
{
    minerals = _minerals;
}

unsigned int Tile::GetMinerals() const
{
    return minerals;
}

void Tile::SetSoilFertility(float fertility)
{
    soil_fertility = fertility;
}

float Tile::GetSoilFertility() const
{
    return soil_fertility;
}

void Tile::SetCity(int id)
{
    city_id = id;
}

int Tile::GetCity() const
{
    return city_id;
}

void Tile::SetPlateId(int _plate_id)
{
    plate_id = _plate_id;
}

void Tile::Draw() const
{
    
}

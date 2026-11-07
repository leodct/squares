#include "city.hpp"
#include <algorithm>

City::City(std::string _name, int origin_tile) : name(_name), origin(origin_tile), country(-1), exists(true)
{
    tiles = std::vector<int>();
}

void City::SetTiles(std::vector<int> ntiles)
{
    tiles = ntiles;
}

void City::AddTiles(std::vector<int> ntiles)
{
    for (int i : ntiles){
        if (std::find(tiles.begin(), tiles.end(), i) == tiles.end())
        tiles.push_back(i);
    }
}

std::vector<int> &City::GetTiles()
{
    return tiles;
}

const std::vector<int> &City::GetTiles() const
{
    return tiles;
}

void City::SetName(std::string new_name)
{
    name = new_name;
}

const std::string City::GetName() const
{
    return name;
}

void City::SetOrigin(int new_origin)
{
    origin = new_origin;
}

int City::GetOrigin() const
{
    return origin;
}

void City::SetCountry(int new_country)
{
    country = new_country;
}

int City::GetCountry() const
{
    return country;
}

bool City::Exists() const
{
    return exists;
}

void City::SetExists(bool value)
{
    exists = value;
}

void City::Destroy()
{
    exists = false;
}

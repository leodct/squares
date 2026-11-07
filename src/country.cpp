#include "country.hpp"
#include <algorithm>

Country::Country(std::string _name, Color _color, int _self_id) : name(_name), color(_color), self_id(_self_id), active(true)
{
}

std::string Country::GetName() const
{
    return name;
}

void Country::SetName(std::string nname)
{
    name = nname;
}

int Country::GetID() const
{
    return self_id;
}

Color Country::GetColor() const
{
    return color;
}

void Country::SetColor(Color ncol)
{
    color = ncol;
}

int Country::GetSize(const std::vector<City>& all_cities) const {
    return GetOwnedTiles(all_cities).size();
}


const std::vector<int> Country::GetOwnedTiles(const std::vector<City> &all_cities) const
{
    std::vector<int> owned_tiles;

    for (City c : all_cities) {
        if (c.GetCountry() == self_id){    
            for (int tile : c.GetTiles()){
                owned_tiles.push_back(tile);
            }
        }
    }

    return owned_tiles;
}

bool Country::GetActive() const
{
    return active;
}

void Country::SetActive(bool flag)
{
    active = flag;
}

#ifndef COUNTRY_H
#define COUNTRY_H
#include <string>
#include <raylib.h>
#include <vector>
#include "city.hpp"

class Country {
public:
    Country(std::string name, Color color, int self_id);

    Color GetColor() const;
    void SetColor(Color ncol);

    /// @returns Amount of tiles owned by this country
    int GetSize(const std::vector<City>& all_cities) const;
    
    const std::vector<int> GetOwnedTiles(const std::vector<City>& all_cities) const;
    
    std::string GetName() const;
    void SetName(std::string nname);

    int GetID() const;

    bool GetActive() const;
    void SetActive(bool flag);

private:
    std::string name;
    Color color;
    // Tiles controlled by the country.
    std::vector<int> controlled_land;
    // The country's internal ID. Will never change.
    const int self_id;

    // Whether this country is active or not. 
    // Inactive countries cannot be interacted with or make decisions.
    bool active;
};

#endif

#ifndef CITY_H
#define CITY_H

#include <vector>
#include <string>

class City {
public:
    City(std::string name, int origin_tile);

    void SetTiles(std::vector<int> ntiles);
    void AddTiles(std::vector<int> ntiles);
    std::vector<int>& GetTiles();
    const std::vector<int>& GetTiles() const;

    void SetName(std::string new_name);
    const std::string GetName() const;

    void SetOrigin(int new_origin);
    int GetOrigin() const;

    void SetCountry(int new_country);
    int GetCountry() const;

    bool Exists() const;
    void Destroy();
    void SetExists(bool value);

private:
    std::vector<int> tiles;
    std::string name;
    int origin;
    int country;
    bool exists;
};

#endif

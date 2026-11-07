#ifndef GAME_H
#define GAME_H
#include <vector>
#include <memory>
#include "country.hpp"
#include "tile.hpp"
#include "world.hpp"

struct Date {
    int day;
    int month;
    int year;
};

struct GameRules {
    int days_per_month = 30;
    int months_per_year = 12; // 360 days per year.
    Date starting_date = {
        .day   = 0, 
        .month = 0, 
        .year  = 0
    }; // Start on 1/1/0 (January 1st, 0).
};

class Game {
public:
    Game(GameRules rules, std::unique_ptr<World> world);

    /**
     * @brief Saves the game under the saves/save_name.sav (pending game name to use funny extension like .hoi4)
     * @param save_name Name of the new save file. Extension not needed.
     * @retval `True` - Success.
     * @retval `False` - Catastrophic failure.
     */
    bool SaveGame(const std::string& save_name) const;
    /**
     * @brief Load a game from a save file.
     * @param save_name Name of the save file. Extension included (So save_name has to include .sav or whatever extension at the end).
     * @returns A pointer to a loaded game, so you can do `Game my_game = Game::LoadGame("game_1.sav");`
     */
    static std::unique_ptr<Game> LoadGame(const std::string& save_name);

    // Countries interface
    void CreateCountry(std::string name, Color color);
    void DeleteCountry(int id);
    const Country& GetCountry(int id);
    const std::vector<Country>& GetCountries() const;
    /**
     * @brief Overwrites country @p `id` with @p `data`'s country data
     */
    void EditCountry(int id, Country data);

    // Cities interface
    void CreateCity(std::string name, int tile_id);
    void DeleteCity(int id);
    const City& GetCity(int id) const;
    const std::vector<City>& GetCities() const;
    /**
     * @brief Overwrites city @p `id` with @p `new_city`'s data.
     */
    void EditCity(int id, City new_city);

    World& GetWorld();
    const World& GetWorld() const;

    std::vector<int>& GetSelection();
    const std::vector<int>& GetSelection() const;
    void SetSelection(std::vector<int>& selection);
    void AddSelection(std::vector<int>& selection);
    void ClearSelection();

    void DrawCountries() const;
    void DrawCities() const;

    void TickDay();

    static const int COUNTRY_NAME_MAX_SIZE = 64;
private:
    std::unique_ptr<World> world;       // World and its tiles.
    std::vector<Country> all_countries; // Game's countries. 
    std::vector<City> all_cities;       // Game's cities.
    std::vector<int> selected_tiles;    // Selected tile indices.
    

    void TickMonth();
    void TickYear();

    Date current_date; // Current game date.
    GameRules rules;

    void DrawTerritory(const Country& country) const;
};

#endif

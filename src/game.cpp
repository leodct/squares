#include "game.hpp"
#include "fs_utils.hpp"
#include <fstream>
#include <algorithm>

Game::Game(GameRules _rules, std::unique_ptr<World> _world) : 
    world(std::move(_world)), rules(_rules),
    current_date(_rules.starting_date)
{
}

bool Game::SaveGame(const std::string &save_name) const
{
    fs::path file_path = fsutils::GetSavesDirectory() / (save_name + ".sav");
    fs::create_directories(file_path.parent_path()); // <- Ensure the folder is a real thing

    std::ofstream file(file_path, std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open file for saving: " << file_path << "\n";
        return false;
    }

    // 1. Write game flag
    const char flag[4] = {'H', 'E', 'R', 'E'};
    file.write(flag, sizeof(flag));

    // 2. Write Date & Rules
    file.write(reinterpret_cast<const char*>(&current_date), sizeof(Date));
    file.write(reinterpret_cast<const char*>(&rules), sizeof(GameRules));

    // 3. Write World basic parameters
    int width = world->GetWidth();
    int height = world->GetHeight();
    unsigned char sea_level = world->GetSeaLevel();
    file.write(reinterpret_cast<const char*>(&width), sizeof(int));
    file.write(reinterpret_cast<const char*>(&height), sizeof(int));
    file.write(reinterpret_cast<const char*>(&sea_level), sizeof(unsigned char));
    int seed = world->GetSeed();
    file.write(reinterpret_cast<const char*>(&seed), sizeof(int));

    // 4. Write Tiles
    const std::vector<Tile>& tiles = world->GetTiles();
    int num_tiles = static_cast<int>(tiles.size());
    file.write(reinterpret_cast<const char*>(&num_tiles), sizeof(int));

    for (const auto& tile : tiles) {
        int plate_id = tile.GetPlateId();
        unsigned char height = tile.GetHeight();
        float temp = tile.GetTemperature();
        unsigned char rainfall = tile.GetRainfall();
        float fertility = tile.GetSoilFertility();
        float curr_lumber = tile.GetCurrentLumber();
        float max_lumber = tile.GetMaxLumber();
        float lumber_regen = tile.GetLumberRegenerationRate();
        unsigned int stone = tile.GetStone();
        unsigned int metals = tile.GetMetals();
        unsigned int coal = tile.GetCoal();
        unsigned int minerals = tile.GetMinerals();
        Vector2 wind = tile.GetWind();

        file.write(reinterpret_cast<const char*>(&plate_id), sizeof(int));
        file.write(reinterpret_cast<const char*>(&height), sizeof(unsigned char));
        file.write(reinterpret_cast<const char*>(&temp), sizeof(float));
        file.write(reinterpret_cast<const char*>(&rainfall), sizeof(unsigned char));
        file.write(reinterpret_cast<const char*>(&fertility), sizeof(float));
        file.write(reinterpret_cast<const char*>(&curr_lumber), sizeof(float));
        file.write(reinterpret_cast<const char*>(&max_lumber), sizeof(float));
        file.write(reinterpret_cast<const char*>(&lumber_regen), sizeof(float));
        file.write(reinterpret_cast<const char*>(&stone), sizeof(unsigned int));
        file.write(reinterpret_cast<const char*>(&metals), sizeof(unsigned int));
        file.write(reinterpret_cast<const char*>(&coal), sizeof(unsigned int));
        file.write(reinterpret_cast<const char*>(&minerals), sizeof(unsigned int));
        file.write(reinterpret_cast<const char*>(&wind), sizeof(Vector2));
    }

    // 5. Write Countries
    int country_count = static_cast<int>(all_countries.size());
    file.write(reinterpret_cast<const char*>(&country_count), sizeof(int));

    for (const auto& country : all_countries) {
        int name_len = country.GetName().length();
        file.write(reinterpret_cast<const char*>(&name_len), sizeof(int));
        file.write(country.GetName().data(), name_len);

        // Save Color
        Color col = country.GetColor();
        file.write(reinterpret_cast<const char*>(&col), sizeof(Color));

        // TODO: FIGURE THIS OUT
        // Save territory vector indices
        //const auto& territory = country.GetTerritory();
        //int territory_size = static_cast<int>(territory.size());
        //file.write(reinterpret_cast<const char*>(&territory_size), sizeof(int));
        //if (territory_size > 0) {
        //    file.write(reinterpret_cast<const char*>(territory.data()), territory_size * sizeof(int));
        //}

        bool active = country.GetActive();
        file.write(reinterpret_cast<const char*>(&active), sizeof(bool));
    }

    return true;
}

std::unique_ptr<Game> Game::LoadGame(const std::string &save_name)
{
    fs::path file_path = fsutils::GetSavesDirectory() / (save_name + ".sav");
    std::ifstream file(file_path, std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open file for loading: " << file_path << "\n";
        return nullptr;
    }

    // 1. Verify game flag
    char flag[4];
    file.read(flag, 4);
    if (flag[0] != 'H' || flag[1] != 'E' || flag[2] != 'R' || flag[3] != 'E') {
        std::cerr << "[ERROR] Save file signature mismatch! Invalid or corrupted file.\n";
        return nullptr;
    }

    // 2. Read Date & Rules
    Date loaded_date;
    GameRules loaded_rules;
    file.read(reinterpret_cast<char*>(&loaded_date), sizeof(Date));
    file.read(reinterpret_cast<char*>(&loaded_rules), sizeof(GameRules));

    // 3. Read World basic parameters
    int width, height, seed;
    file.read(reinterpret_cast<char*>(&width), sizeof(int));
    file.read(reinterpret_cast<char*>(&height), sizeof(int));
    unsigned char sea_level;
    file.read(reinterpret_cast<char*>(&sea_level), sizeof(unsigned char));
    file.read(reinterpret_cast<char*>(&seed), sizeof(int));

    // Recreate the world using the saved seed and dimensions
    auto loaded_world = std::make_unique<World>(width, height, seed, sea_level);

    // 4. Read Tiles
    int num_tiles = 0;
    file.read(reinterpret_cast<char*>(&num_tiles), sizeof(int));
    
    auto& tiles = loaded_world->GetTiles();
    tiles.resize(num_tiles);
    
    for (int i = 0; i < num_tiles; i++) {
        int plate_id;
        unsigned char height;
        float temp;
        unsigned char rainfall;
        float fertility;
        float curr_lumber;
        float max_lumber;
        float lumber_regen;
        unsigned int stone;
        unsigned int metals;
        unsigned int coal;
        unsigned int minerals;
        Vector2 wind;

        file.read(reinterpret_cast<char*>(&plate_id), sizeof(int));
        file.read(reinterpret_cast<char*>(&height), sizeof(unsigned char));
        file.read(reinterpret_cast<char*>(&temp), sizeof(float));
        file.read(reinterpret_cast<char*>(&rainfall), sizeof(unsigned char));
        file.read(reinterpret_cast<char*>(&fertility), sizeof(float));
        file.read(reinterpret_cast<char*>(&curr_lumber), sizeof(float));
        file.read(reinterpret_cast<char*>(&max_lumber), sizeof(float));
        file.read(reinterpret_cast<char*>(&lumber_regen), sizeof(float));
        file.read(reinterpret_cast<char*>(&stone), sizeof(unsigned int));
        file.read(reinterpret_cast<char*>(&metals), sizeof(unsigned int));
        file.read(reinterpret_cast<char*>(&coal), sizeof(unsigned int));
        file.read(reinterpret_cast<char*>(&minerals), sizeof(unsigned int));
        file.read(reinterpret_cast<char*>(&wind), sizeof(Vector2));

        Tile& tile = tiles[i];
        tile.SetPlateId(plate_id);
        tile.SetHeight(height);
        tile.SetTemperature(temp);
        tile.SetRainfall(rainfall);
        tile.SetSoilFertility(fertility);
        tile.SetCurrentLumber(curr_lumber);
        tile.SetMaxLumber(max_lumber);
        tile.SetLumberRegen(lumber_regen);
        tile.SetStone(stone);
        tile.SetMetals(metals);
        tile.SetCoal(coal);
        tile.SetMinerals(minerals);
        tile.SetWind(wind);
    }

    // 5. Construct the Game instance early so we can populate countries
    auto game = std::make_unique<Game>(loaded_rules, std::move(loaded_world));
    game->current_date = loaded_date;

    // 6. Read Countries
    int country_count = 0;
    file.read(reinterpret_cast<char*>(&country_count), sizeof(int));

    game->all_countries.clear();
    for (int i = 0; i < country_count; i++) {
        // Read name length and string data
        int name_len = 0;
        file.read(reinterpret_cast<char*>(&name_len), sizeof(int));
        std::string name(name_len, '\0');
        file.read(&name[0], name_len);

        // Read Color
        Color col;
        file.read(reinterpret_cast<char*>(&col), sizeof(Color));

        // Create the country object
        Country country(name, col, i);

        // Read territory vector indices
        int territory_size = 0;
        file.read(reinterpret_cast<char*>(&territory_size), sizeof(int));
        if (territory_size > 0) {
            // TODO: Figure this out
            
            //auto& territory = country.GetOwnedTiles(c);
            //territory.resize(territory_size);
            //file.read(reinterpret_cast<char*>(territory.data()), territory_size * sizeof(int));
        }

        // Read active flag
        bool active = true;
        file.read(reinterpret_cast<char*>(&active), sizeof(bool));
        country.SetActive(active);

        game->all_countries.push_back(country);
    }

    std::cout << "[INFO] Successfully loaded game: " << save_name << "\n";
    return game;
}

void Game::CreateCountry(std::string name, Color color)
{
    all_countries.push_back(Country(name, color, all_countries.size()));
}

void Game::DeleteCountry(int id)
{
    all_countries[id].SetActive(false);
}

const Country &Game::GetCountry(int id)
{
    return all_countries[id];
}

const std::vector<Country> &Game::GetCountries() const
{
    return all_countries;
}

void Game::EditCountry(int id, Country data)
{
    Country& original = all_countries[id];
    original.SetActive(data.GetActive());
    original.SetColor(data.GetColor());
    original.SetName(data.GetName());
}

void Game::CreateCity(std::string name, int tile_id)
{
    all_cities.push_back(City(name, tile_id));
}

void Game::DeleteCity(int id)
{
    all_cities[id].Destroy();
}


const City &Game::GetCity(int id) const
{
    return all_cities[id];
}

const std::vector<City> &Game::GetCities() const
{
    return all_cities;
}

void Game::EditCity(int id, City new_city)
{
    all_cities[id] = new_city;
}

World &Game::GetWorld()
{
    return *world;
}

const World &Game::GetWorld() const
{
    return *world;
}

std::vector<int> &Game::GetSelection()
{
    return selected_tiles;
}

const std::vector<int> &Game::GetSelection() const
{
    return selected_tiles;
}

void Game::SetSelection(std::vector<int> &selection)
{
    selected_tiles = selection;
}

void Game::AddSelection(std::vector<int> &selection)
{
    for (int id : selection) {
        if (std::find(selected_tiles.begin(), selected_tiles.end(), id) == selected_tiles.end()) {
            selected_tiles.push_back(id);
        }
    }
}

void Game::ClearSelection()
{
    selected_tiles.clear();
}

void Game::DrawCountries() const
{
    for (const Country& c : all_countries){
        DrawTerritory(c);
    }
}

void Game::DrawCities() const
{
    for (auto&c : all_cities) {
        int tile = c.GetOrigin();
        int x = tile % world->GetWidth();
        int y = tile / world->GetWidth();

        float px = (x * TILE_SIZE) + (TILE_SIZE / 2.0f);
        float py = (y * TILE_SIZE) + (TILE_SIZE / 2.0f);

        //DrawRectanglePro({ px, py, TILE_SIZE, TILE_SIZE }, { TILE_SIZE / 2.0f, 0.0f }, 45.0f, MAROON);
        DrawCircleV({px, py}, TILE_SIZE / 2.0f, MAROON);
    }
}

void Game::TickDay()
{
    current_date.day++;
    if (current_date.day >= rules.days_per_month){
        current_date.day = 0;
        current_date.month++;
        TickMonth();
        if (current_date.month >= rules.months_per_year){
            current_date.month = 0;
            current_date.year++;
            TickYear();
        }
    }
}

void Game::TickMonth()
{
}

void Game::TickYear()
{
}

void Game::DrawTerritory(const Country &country) const
{
    int line_thickness = 2;
    Color outline_col = country.GetColor();
    Color inside_col = country.GetColor();
    outline_col.a = 255;
    inside_col.a = 100;

    const auto& territory = country.GetOwnedTiles(all_cities);
    int width = world->GetWidth();
    int height = world->GetHeight();

    // 1. Populate the lookup grid so we know which tiles this country owns
    std::vector<bool> owned_tiles(width * height, false);
    for (int id : territory) {
        if (id >= 0 && id < static_cast<int>(owned_tiles.size())) {
            owned_tiles[id] = true;
        }
    }

    // 2. Loop through territory and draw
    for (int id : territory) {
        int x = id % width;
        int y = id / width;
        
        // Convert grid coordinates to pixel coordinates
        float px = static_cast<float>(x * TILE_SIZE);
        float py = static_cast<float>(y * TILE_SIZE);

        // Draw semi-transparent background fill
        DrawRectangle(static_cast<int>(px), static_cast<int>(py), TILE_SIZE, TILE_SIZE, inside_col);

        // Draw edge line INSET by 1 pixel so it stays strictly within the tile bounds [0, TILE_SIZE]
        // Top edge (shifted down by 1 pixel)
        if (y == 0 || !owned_tiles[x + (y - 1) * width]) {
            DrawLineEx({px, py + 1.0f}, {px + TILE_SIZE, py + 1.0f}, line_thickness, outline_col);
        }
        // Bottom edge (shifted up by 1 pixel)
        if (y == height - 1 || !owned_tiles[x + (y + 1) * width]) {
            DrawLineEx({px, py + TILE_SIZE - 1.0f}, {px + TILE_SIZE, py + TILE_SIZE - 1.0f}, line_thickness, outline_col);
        }
        // Left edge (shifted right by 1 pixel)
        if (x == 0 || !owned_tiles[(x - 1) + y * width]) {
            DrawLineEx({px + 1.0f, py}, {px + 1.0f, py + TILE_SIZE}, line_thickness, outline_col);
        }
        // Right edge (shifted left by 1 pixel)
        if (x == width - 1 || !owned_tiles[(x + 1) + y * width]) {
            DrawLineEx({px + TILE_SIZE - 1.0f, py}, {px + TILE_SIZE - 1.0f, py + TILE_SIZE}, line_thickness, outline_col);
        }
    }
}
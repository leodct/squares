#include "GameRenderer.hpp"
#include "tile.hpp"
#include <vector>

GameRenderer::GameRenderer()
{
}

void GameRenderer::Draw(PreviewMode mode, Camera2D camera, const Game &game, const World &world)
{
    // 1. World Rendering
    BeginMode2D(camera);
    switch(mode) {
        case TECTONIC:    world.PreviewTectonics(); break;
        case HEIGHTMAP:   world.PreviewHeight(); break;
        case TEMPERATURE: world.PreviewTemperature(); break;
        case WIND:        world.PreviewWinds(); break;
        case RAINFALL:    world.PreviewRainfall(); break;
        case FERTILITY:   world.PreviewSoilFertility(); break;
        case LUMBER:      world.PreviewLumber(); break;
        case STONE:       world.PreviewStone(); break;
        case METALS:      world.PreviewMetals(); break;
        case COAL:        world.PreviewCoal(); break;
        case MINERALS:    world.PreviewMinerals(); break;
    }

    // 2. Selection rendering
    DrawSelection(game, world);
    
    // 3. City and Country rendering
    game.DrawCountries();
    game.DrawCities();
    EndMode2D();
}

void GameRenderer::DrawSelection(const Game &game, const World& world)
{
    // --- UNIFIED PERIMETER HIGHLIGHT FOR SELECTED TILES ---
    const auto& selected_tiles = game.GetSelection();
    if (!selected_tiles.empty()) {
        // Fast membership lookup grid
        // Store whether a tile is selected or not in a vector
        std::vector<bool> is_selected(world.GetWidth() * world.GetHeight(), false);
        for (int idx : selected_tiles) {
            if (idx >= 0 && idx < is_selected.size()) {
                is_selected[idx] = true;
            }
        }
         
        Color outline_color = {255, 255, 255, 240};
        float line_thickness = 2.0f;
         
        for (int idx : selected_tiles) {
            int x = idx % world.GetWidth();
            int y = idx / world.GetWidth();
             
            float px = x * TILE_SIZE;
            float py = y * TILE_SIZE;
             
            // Draw edge line only if the neighboring tile in that direction is NOT selected
            // Top edge
            if (y == 0 || !is_selected[x + (y - 1) * world.GetWidth()]) {
                DrawLineEx({px, py}, {px + TILE_SIZE, py}, line_thickness, outline_color);
            }
            // Bottom edge
            if (y == world.GetHeight() - 1 || !is_selected[x + (y + 1) * world.GetWidth()]) {
                DrawLineEx({px, py + TILE_SIZE}, {px + TILE_SIZE, py + TILE_SIZE}, line_thickness, outline_color);
            }
            // Left edge
            if (x == 0 || !is_selected[(x - 1) + y * world.GetWidth()]) {
                DrawLineEx({px, py}, {px, py + TILE_SIZE}, line_thickness, outline_color);
            }
            // Right edge
            if (x == world.GetWidth() - 1 || !is_selected[(x + 1) + y * world.GetWidth()]) {
                DrawLineEx({px + TILE_SIZE, py}, {px + TILE_SIZE, py + TILE_SIZE}, line_thickness, outline_color);
            }
        }
    }
}

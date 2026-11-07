#include <raylib.h>
#include <imgui.h>
#include "rlImGui.h"

#include "world.hpp"
#include "game.hpp"
#include "fs_utils.hpp"
#include "GameRenderer.hpp"
#include "UIRenderer.hpp"
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// Helper function to scan the saves directory for .sav files
std::vector<std::string> GetAvailableSaves() {
    std::vector<std::string> saves;
    fs::path save_dir = fsutils::GetSavesDirectory();
    
    if (fs::exists(save_dir) && fs::is_directory(save_dir)) {
        for (const auto& entry : fs::directory_iterator(save_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".sav") {
                saves.push_back(entry.path().stem().string());
            }
        }
    }
    return saves;
}

int main(int argc, char const *argv[])
{
    // Initialize default game state wrapping the world
    GameRules default_rules;
    std::unique_ptr<World> initial_world = std::make_unique<World>(150, 150, 123, 70);
    initial_world->Generate();
    
    auto current_game = std::make_unique<Game>(default_rules, std::move(initial_world));
    
    // Reference alias so the rest of your render/preview loop works seamlessly without code rewrites
    World* world = &current_game->GetWorld(); 
    
    PreviewMode preview_mode = HEIGHTMAP;

    InitWindow(1200, 800, "Squares");
    SetTargetFPS(60);
    Camera2D camera;
    camera.target = {0,0};
    camera.zoom = 1;

    rlImGuiSetup(true);

    // Save/Load UI state variables
    char save_name_buf[128] = "my_save";
    int selected_save_index = 0;
    std::vector<std::string> available_saves = GetAvailableSaves();
    bool mouse_dragging = false;
    std::pair<Vector2, Vector2> selection;

    while (!WindowShouldClose()) {
        // Logic
        // Only process camera movement and hotkeys if ImGui is NOT capturing the keyboard (e.g. typing a save name)
        if (!ImGui::GetIO().WantCaptureKeyboard && !ImGui::GetIO().WantCaptureMouse) {
            if (IsKeyDown(KEY_KP_ADD)){
                ((camera.zoom += 0.01f) >= 4.0f) ? camera.zoom = 4.0f : camera.zoom;
            }
            if (IsKeyDown(KEY_KP_SUBTRACT)){
                ((camera.zoom -= 0.01f) <= 0.1f) ? camera.zoom = 0.1f : camera.zoom;
            }
            if (IsKeyDown(KEY_W)) camera.target.y -= 10;
            if (IsKeyDown(KEY_S)) camera.target.y += 10;
            if (IsKeyDown(KEY_A)) camera.target.x -= 10;
            if (IsKeyDown(KEY_D)) camera.target.x += 10;

            if (IsKeyPressed(KEY_R)){
                int nseed = rand();
                std::cout << "New seed: " << nseed << std::endl;
                world->Regenerate(nseed);
            }

            // UI multi-tile selection logic
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
                mouse_dragging = true;
                selection.first = GetMousePosition();
            }
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)){
                if (mouse_dragging) {
                    selection.second = GetMousePosition();
                    
                    // Convert screen selection bounds to world/tile coordinates
                    Vector2 worldStart = GetScreenToWorld2D(selection.first, camera);
                    Vector2 worldEnd = GetScreenToWorld2D(selection.second, camera);

                    int startTileX = static_cast<int>(std::floor(worldStart.x / TILE_SIZE));
                    int startTileY = static_cast<int>(std::floor(worldStart.y / TILE_SIZE));
                    int endTileX = static_cast<int>(std::floor(worldEnd.x / TILE_SIZE));
                    int endTileY = static_cast<int>(std::floor(worldEnd.y / TILE_SIZE));

                    int minX = std::clamp(std::min(startTileX, endTileX), 0, world->GetWidth() - 1);
                    int maxX = std::clamp(std::max(startTileX, endTileX), 0, world->GetWidth() - 1);
                    int minY = std::clamp(std::min(startTileY, endTileY), 0, world->GetHeight() - 1);
                    int maxY = std::clamp(std::max(startTileY, endTileY), 0, world->GetHeight() - 1);

                    // Collect all tiles inside the dragged rectangle
                    std::vector<int> newly_selected;
                    for (int y = minY; y <= maxY; ++y) {
                        for (int x = minX; x <= maxX; ++x) {
                            newly_selected.push_back(x + y * world->GetWidth());
                        }
                    }

                    // Check if Shift is held to add to selection or overwrite
                    if (IsKeyDown(KEY_LEFT_SHIFT)) {
                        current_game->AddSelection(newly_selected);
                    } else {
                        current_game->SetSelection(newly_selected);
                    }
                }
                mouse_dragging = false; 
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)){
                mouse_dragging = false;
                current_game->ClearSelection();
            }

        }
        else {
            mouse_dragging = false;
        }

        // Drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);

        GameRenderer::Draw(preview_mode, camera, *current_game, *world);
        
        // 2. UI Rendering
        
        // Click + Drag
        if (mouse_dragging){
            float x1 = selection.first.x;
            float x2 = static_cast<float>(GetMouseX());
            float y1 = selection.first.y;
            float y2 = static_cast<float>(GetMouseY());
            
            DrawRectanglePro(
                {
                    .x = std::min(x1, x2),
                    .y = std::min(y1, y2),
                    .width = std::abs(x1 - x2),
                    .height = std::abs(y1 - y2)
                },
                {0, 0},
                0,
                {
                    .r = 235,
                    .g = 235,
                    .b = 235,
                    .a = 100
                }
            );
        }

        rlImGuiBegin();

        UIRenderer::Draw(preview_mode, camera, *current_game, *world);

        rlImGuiEnd();
        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
    return 0;
}
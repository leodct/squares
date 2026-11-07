#include "UIRenderer.hpp"
#include <imgui.h>
#include <raylib.h>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <cstring>

UIRenderer::UIRenderer()
{
}

void UIRenderer::Draw(PreviewMode mode, Camera2D camera, Game &game, World &world)
{
        //ImGui::SetWindowFontScale(2.0f);
        ImGui::PushFont(NULL, ImGui::GetStyle().FontSizeBase * 1.25f);
        
        // --- PREVIEW MODE SELECTOR WINDOW ---
        ImGui::Begin("World Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        const char* previewModeNames[] = {
            "Tectonic", "Heightmap", "Temperature", "Wind", 
            "Rainfall", "Fertility", "Lumber", "Stone", "Metals",
            "Coal", "Minerals"
        };
        
        int currentModeIndex = static_cast<int>(mode);
        if (ImGui::Combo("Preview Mode", &currentModeIndex, previewModeNames, IM_ARRAYSIZE(previewModeNames))) {
            mode = static_cast<PreviewMode>(currentModeIndex);
        }

        ImGui::Separator();
        if (ImGui::Button("Regenerate World (R)")) {
            int nseed = rand();
            std::cout << "New seed: " << nseed << std::endl;
            world.Regenerate(nseed); 
        }
        
        ImGui::Text("Camera: WASD to Move, +/- to Zoom");
        ImGui::End();

        // --- HOVER INSPECTION TOOLTIP ---
        Vector2 mouseScreenPos = GetMousePosition();
        Vector2 mouseWorldPos = GetScreenToWorld2D(mouseScreenPos, camera);
        
        int tileX = static_cast<int>(std::floor(mouseWorldPos.x / TILE_SIZE));
        int tileY = static_cast<int>(std::floor(mouseWorldPos.y / TILE_SIZE));
        
        if (tileX >= 0 && tileX < world.GetWidth() && tileY >= 0 && tileY < world.GetHeight()) {
            const Tile& tile = world.GetTile(tileX, tileY);
            
            ImGui::BeginTooltip();
            ImGui::Text("Coordinates: (%d, %d)", tileX, tileY);
            ImGui::Separator();
            
            switch(mode) {
                case TECTONIC:
                    ImGui::Text("Plate ID: %d", tile.GetPlateId());
                    break;
                case HEIGHTMAP:
                    ImGui::Text("Height: %d", static_cast<int>(tile.GetHeight()));
                    ImGui::Text("Temperature: %.1f C", tile.GetTemperature());
                    ImGui::Text("Rainfall: %d", tile.GetRainfall());
                    ImGui::Separator();
                    ImGui::Text("Soil Fertility: %.1f", tile.GetSoilFertility() * 100);
                    ImGui::Separator();
                    ImGui::Text("Current Lumber: %.1f", tile.GetCurrentLumber());
                    ImGui::Text("Max Lumber: %.1f", tile.GetMaxLumber());
                    ImGui::Text("Lumber Regeneration: %.3f", tile.GetLumberRegenerationRate());
                    ImGui::Separator();
                    ImGui::Text("Stone: %i", tile.GetStone());
                    ImGui::Text("Metals: %i", tile.GetMetals());
                    ImGui::Text("Coal: %i", tile.GetCoal());
                    break;
                case TEMPERATURE:
                    ImGui::Text("Temperature: %.1f C", tile.GetTemperature());
                    break;
                case WIND:
                    ImGui::Text("Wind speed: %.1f", std::sqrt(tile.GetWind().x * tile.GetWind().x + tile.GetWind().y * tile.GetWind().y));
                    break;
                case RAINFALL:
                    ImGui::Text("Average rainfall: %d", tile.GetRainfall());
                    break;
                case FERTILITY:
                    ImGui::Text("Soil Fertility: %.1f", tile.GetSoilFertility() * 100);
                    break;
                case LUMBER:
                    ImGui::Text("Current Lumber: %.1f", tile.GetCurrentLumber());
                    ImGui::Text("Max Lumber: %.1f", tile.GetMaxLumber());
                    ImGui::Text("Lumber Regeneration: %.3f", tile.GetLumberRegenerationRate());
                    ImGui::Separator();
                    ImGui::Text("Soil Fertility: %.1f", tile.GetSoilFertility() * 100);
                    break;
                case STONE:
                    ImGui::Text("Stone: %i", tile.GetStone());
                    break;
                case METALS:
                    ImGui::Text("Metals: %i", tile.GetMetals());
                    break;
                case COAL:
                    ImGui::Text("Coal: %i", tile.GetCoal());
                    break;
                case MINERALS:
                    ImGui::Text("Minerals: %i", tile.GetMinerals());
                    break;
            }
            ImGui::EndTooltip();
        }

        // --- SAVE/LOAD WINDOW & FILE BROWSER ---
        /*
        ImGui::Begin("Save / Load Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        // Save section
        ImGui::InputText("Save Name", save_name_buf, IM_ARRAYSIZE(save_name_buf));
        if (ImGui::Button("Save Game")) {
            if (game) {
                if (current_game->SaveGame(save_name_buf)) {
                    std::cout << "[UI] Game successfully saved to: " << save_name_buf << ".sav\n";
                    available_saves = GetAvailableSaves(); // Refresh file list
                } else {
                    std::cerr << "[UI] Failed to save game!\n";
                }
            }
        }

        ImGui::Separator();

        // Load / File Browser section
        if (ImGui::Button("Refresh File List")) {
            available_saves = GetAvailableSaves();
        }

        if (!available_saves.empty()) {
            if (selected_save_index >= static_cast<int>(available_saves.size())) {
                selected_save_index = 0;
            }

            // Convert vector of strings to a format ImGui::Combo accepts
            std::vector<const char*> save_cstr_ptrs;
            for (const auto& s : available_saves) {
                save_cstr_ptrs.push_back(s.c_str());
            }

            ImGui::Combo("Existing Saves", &selected_save_index, save_cstr_ptrs.data(), static_cast<int>(save_cstr_ptrs.size()));

            if (ImGui::Button("Load Selected Game")) {
                auto loaded_game = Game::LoadGame(available_saves[selected_save_index]);
                if (loaded_game) {
                    current_game = std::move(loaded_game);
                    world = &current_game->GetWorld();
                    std::cout << "[UI] Successfully loaded game: " << available_saves[selected_save_index] << "\n";
                } else {
                    std::cerr << "[UI] Failed to load game file!\n";
                }
            }
        } else {
            ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "No save files found in saves/");
        }*/

        //ImGui::End();

        // --- COUNTRY DEBUG MENU ---
        ImGui::Begin("Debug - Countries", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        if (ImGui::Button("Create Country",  ImVec2(ImGui::GetContentRegionAvail().x, 30.0f))) {
            ImGui::OpenPopup("CreateCountryModal");
        }

        // COUNTRY CREATION MODAL
        if (ImGui::BeginPopupModal("CreateCountryModal", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            static char country_name_buf[Game::COUNTRY_NAME_MAX_SIZE] = "My Country";
            static float country_color_arr[3] = {0.2f, 0.6f, 0.9f};

            ImGui::Text("Enter new country details:");
            ImGui::Separator();

            ImGui::InputText("Name", country_name_buf, IM_ARRAYSIZE(country_name_buf));
            ImGui::ColorEdit3("Color", country_color_arr);

            ImGui::Spacing();

            if (ImGui::Button("Confirm", ImVec2(120, 0))) {
                Color col = {
                    static_cast<unsigned char>(country_color_arr[0] * 255),
                    static_cast<unsigned char>(country_color_arr[1] * 255),
                    static_cast<unsigned char>(country_color_arr[2] * 255),
                    255
                };
                game.CreateCountry(country_name_buf, col);
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (ImGui::Button("Edit Existing Country", ImVec2(ImGui::GetContentRegionAvail().x, 30.0f))){
            ImGui::OpenPopup("EditCountryModal");
        }

        if (ImGui::BeginPopupModal("EditCountryModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)){
            static int selected_country = 0;
            if (game.GetCountries().size() == 0){
                ImGui::Text("No countries!");
            }
            else {
                ImGui::Combo("CountrySelector", &selected_country, game.GetCountries().data()->GetName().c_str(), static_cast<int>(game.GetCountries().size()));
            }

            if (game.GetCountries().size() != 0) {
                if (ImGui::Button("Select",  ImVec2(ImGui::GetContentRegionAvail().x / 2.0f, 30.0f))){
                    ImGui::OpenPopup("CountryEditor");
                }

                if (ImGui::BeginPopupModal("CountryEditor", nullptr, ImGuiWindowFlags_AlwaysAutoResize)){
                    Country country = game.GetCountry(selected_country);

                    // COLOUR EDITING
                    float original_col[3] = {
                        country.GetColor().r / 255.0f,
                        country.GetColor().g / 255.0f,
                        country.GetColor().b / 255.0f
                    };
                    static bool edited_colour = false;
                    static float ccol[3];

                    if (!edited_colour){
                        ccol[0] = original_col[0];
                        ccol[1] = original_col[1];
                        ccol[2] = original_col[2];
                        edited_colour = true;
                    }
                    ImGui::ColorEdit3("Edit Color", ccol, ImGuiColorEditFlags_NoAlpha);

                    // NAME EDITING
                    static char name_buffer[Game::COUNTRY_NAME_MAX_SIZE] = "";
                    static int last_selected_country = -1;
                                
                    // Reset/load the country's name into the buffer when we first open the editor for this country
                    if (last_selected_country != selected_country) {
                        std::strncpy(name_buffer, country.GetName().c_str(), sizeof(name_buffer));
                        name_buffer[sizeof(name_buffer) - 1] = '\0';
                        last_selected_country = selected_country;
                    }
                
                    ImGui::InputText("Country Name", name_buffer, IM_ARRAYSIZE(name_buffer));


                    // ASSIGNING CITIES
                    if (ImGui::Button("Set Cities", ImVec2(ImGui::GetContentRegionAvail().x, 30))){
                        ImGui::OpenPopup("CityAssignerModal");
                    }

                    if (ImGui::BeginPopupModal("CityAssignerModal")){
                        int total_cities = static_cast<int>(game.GetCities().size());
                        if (total_cities == 0){
                            ImGui::Text("No cities!");
                        }
                        else {
                            static std::vector<bool> selected_city_flags;
                        
                            // Dynamically resize if the number of cities changed
                            if (selected_city_flags.size() != total_cities) {
                                selected_city_flags.resize(total_cities, 0);
                            }
                        
                            for (int i = 0; i < total_cities; i++){
                                // 1. Load the current state for THIS specific city into a local bool
                                bool is_checked = selected_city_flags[i];
                            
                                // 2. Pass its unique address to the checkbox
                                if (ImGui::Checkbox(game.GetCity(i).GetName().c_str(), &is_checked)) {
                                    // 3. Save it back only if the user toggled it this frame
                                    selected_city_flags[i] = is_checked;
                                }
                            }
                        
                            // Add a Confirm/Apply button down here to actually use selected_city_flags!
                            if (ImGui::Button("Apply", ImVec2(ImGui::GetContentRegionAvail().x / 2.0f, 30.0f))) {
                                std::vector<int> cities_to_assign;
                                for (int i = 0; i < total_cities; ++i) {
                                    if (selected_city_flags[i]) {
                                        City c = game.GetCity(i);
                                        c.SetCountry(country.GetID());
                                        game.EditCity(i, c);
                                    }
                                }
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::SameLine();
                        }
                    
                        if (ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvail().x, 30))) {
                            ImGui::CloseCurrentPopup();
                        }
                    
                        ImGui::EndPopup();
                    }




                    if (ImGui::Button("Done", ImVec2(ImGui::GetContentRegionAvail().x / 2, 30))){
                        Color c = {
                            static_cast<unsigned char>(ccol[0] * 255),
                            static_cast<unsigned char>(ccol[1] * 255),
                            static_cast<unsigned char>(ccol[2] * 255),
                            255
                        };
                        country.SetColor(c);
                        country.SetName(name_buffer);

                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::SameLine();
                    
                    if (ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvail().x, 30))){
                        ImGui::CloseCurrentPopup();
                    }

                    game.EditCountry(selected_country, country);
                    ImGui::EndPopup();
                }


                
                ImGui::SameLine();
            }
                
            if (ImGui::Button("Cancel",  ImVec2(ImGui::GetContentRegionAvail().x, 30.0f))){
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }

        ImGui::End();

        // --- TILE SETTINGS UI ---
        if (!game.GetSelection().empty()){
            auto& selection = game.GetSelection();
            int tile_count = selection.size();

            std::string window_title = (tile_count == 1) ? "Tile selector" : std::to_string(tile_count) + " tiles selected."; 
            
            // pin window to bottom right corner of the screen
            ImVec2 display_size = ImGui::GetIO().DisplaySize;
            float padding = 20.0f; // Distance from the edge of the screen
            
            // Set position to bottom-right, always update on resize, pivot from bottom-right corner (1.0, 1.0)
            ImGui::SetNextWindowPos(
                ImVec2(display_size.x - padding, display_size.y - padding), 
                ImGuiCond_Always, 
                ImVec2(1.0f, 1.0f)
            );

            ImGui::Begin(window_title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            // ONLY ONE TILE SELECTED: CAN CREATE CITY!    
            if (tile_count == 1){
                    // City creator
                if (ImGui::Button("New City", ImVec2(240, 0))){
                    ImGui::OpenPopup("Create New City");
                }

                if (ImGui::BeginPopupModal("Create New City", nullptr, ImGuiWindowFlags_AlwaysAutoResize)){                    static char city_name[128] = "Dumblebourg";
                    ImGui::InputText("##city_name", city_name, IM_ARRAYSIZE(city_name));
                    ImGui::SameLine();
                    if (ImGui::Button("Create City", ImVec2(60, 0))) {
                        game.CreateCity(std::string(city_name), selection[0]);
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::Button("Cancel", ImVec2(240, 0))){
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }
            }
                
            // ASSIGN ONE OR MULTIPLE TILES TO A CITY!
            if (ImGui::Button("Set City", ImVec2(240, 0))){
                ImGui::OpenPopup("Assign tiles to a city");
            }

            if (ImGui::BeginPopupModal("Assign tiles to a city", nullptr, ImGuiWindowFlags_AlwaysAutoResize)){
                static int selected_city = 0;
                if (game.GetCities().size() != 0){
                    std::vector<const char*> city_names;
                    for (const City& c : game.GetCities()){
                        city_names.push_back(c.GetName().c_str());
                    }

                    ImGui::Combo("##city_selector_combo", &selected_city, city_names.data(), static_cast<int>(game.GetCities().size()));
                    
                    if (ImGui::Button("Set City", ImVec2(ImGui::GetContentRegionAvail().x, 20))){
                        for (int tile : selection){
                            world.GetTiles()[tile].SetCity(selected_city);
                        }
                        ImGui::CloseCurrentPopup();
                    }
                }
                else {
                    ImGui::Text("No cities");
                }
                
                if (ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvail().x, 20))){
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

        

            ImGui::End();
        }

        ImGui::PopFont();
}

#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include <raylib.h>
#include "game.hpp"
#include "world.hpp"
#include "PreviewMode.hpp"


class GameRenderer{
public:
    GameRenderer();

    static void Draw(PreviewMode mode, Camera2D camera, const Game &game, const World& world);
private:
    static void DrawSelection(const Game& game, const World& world);
};

#endif 

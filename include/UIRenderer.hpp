#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include "PreviewMode.hpp"
#include "game.hpp"
#include "world.hpp"

class UIRenderer{
public:
    UIRenderer();

    static void Draw(PreviewMode mode, Camera2D camera, Game &game, World &world);
private:
};

#endif 
 
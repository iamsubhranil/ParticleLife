#pragma once

struct SDL_Renderer;

struct Particles {
    static void init();
    static void interact(int type1, int type2, float g);
    static void draw(SDL_Renderer *renderer);
};

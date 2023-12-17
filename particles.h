#pragma once

struct SDL_Renderer;

struct Particles {
    static void init();
    static void draw(SDL_Renderer* renderer);
};

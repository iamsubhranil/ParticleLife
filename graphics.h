#pragma once

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

struct Graphics {
    struct Texture {
        SDL_Texture *texture;
        int width, height;
    };

    const static int WIDTH = 1920;
    const static int HEIGHT = 1080;

    static SDL_Window *window;
    static SDL_Renderer *renderer;

    static void init();
    static void run();
    static void destroy();

    static void addTexture(SDL_Texture *t);

    static Texture loadTexture(const char *file, double scale = 1.0);
};

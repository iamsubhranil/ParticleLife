#include "particles.h"

#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>

#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

#include "graphics.h"
#include "threadpool.h"

static const int numParticles = 1500;

static const SDL_Color colorValues[] = {
    SDL_Color(251, 73, 52, 255),   SDL_Color(131, 165, 152, 255),
    SDL_Color(184, 187, 38, 255),  SDL_Color(250, 189, 47, 255),
    SDL_Color(211, 134, 155, 255), SDL_Color(142, 192, 124, 255),
    SDL_Color(254, 128, 25, 255)};

enum Color : int {
    RED = 0,
    BLUE = 1,
    GREEN = 2,
    YELLOW = 3,
    PURPLE = 4,
    AQUA = 5,
    ORANGE = 6
};

static const Color MinColor = RED;
static const Color MaxColor = ORANGE;

auto particles = std::vector<std::vector<SDL_FPoint>>();

auto particleVelocities = std::vector<std::vector<std::pair<float, float>>>();
auto particleTargetVelocities =
    std::vector<std::vector<std::pair<float, float>>>();

float interactionMatrix[MaxColor + 1][MaxColor + 1] = {{0.0}};

const static float cutOffX = 100;
const static float cutOffY = 100;

std::mutex particleMutexes[MaxColor + 1] = {
    std::mutex(), std::mutex(), std::mutex(), std::mutex(),
    std::mutex(), std::mutex(), std::mutex()};

Threadpool threadPool = Threadpool();

void Particles::init() {
    std::uniform_real_distribution<float> heightDist(0, Graphics::HEIGHT);
    std::uniform_real_distribution<float> widthDist(0, Graphics::WIDTH);

    std::default_random_engine re;

    std::uniform_int_distribution<int> colorDist(RED, YELLOW);

    for (int col = MinColor; col < MaxColor + 1; col++) {
        particles.push_back(std::vector<SDL_FPoint>());
        particleVelocities.push_back(std::vector<std::pair<float, float>>());
        particleTargetVelocities.push_back(
            std::vector<std::pair<float, float>>());
        for (int i = 0; i < numParticles; i++) {
            particles[col].push_back(SDL_FPoint{widthDist(re), heightDist(re)});
            particleVelocities[col].push_back({0, 0});
        }
    }

    std::normal_distribution<float> interactDist(-15, 15);
    for (int col1 = MinColor; col1 < MaxColor + 1; col1++) {
        for (int col2 = MinColor; col2 < MaxColor + 1; col2++) {
            // if (col1 == col2)
            //    interactionMatrix[col1][col2] = -10;
            // else
            interactionMatrix[col1][col2] = interactDist(re);
        }
    }
}

static const float maxFx = 300;
static const float maxFy = 300;

void interact(auto &particles1, auto &particles2, auto &particleVelocities1,
              float g) {
    for (size_t i = 0; i < particles1.size(); i++) {
        float fx = 0;
        float fy = 0;
        SDL_FPoint &p1 = particles1[i];

        for (size_t j = 0; j < particles2.size(); j++) {
            const SDL_FPoint p2 = particles2[j];

            float dx = p1.x - p2.x;
            // if (dx > cutOffX || dx < -cutOffX) continue;
            float dy = p1.y - p2.y;
            // if (dy > cutOffY || dy < -cutOffY) continue;

            float d = std::sqrt(dx * dx + dy * dy);
            if (d > 0 && d < 80) {
                float F = g * 1 / d;
                fx += (F * dx);
                fy += (F * dy);
            }
        }

        auto &vx = std::get<0>(particleVelocities1[i]);
        auto &vy = std::get<1>(particleVelocities1[i]);

        vx = (vx + fx) * 0.05;
        vy = (vy + fy) * 0.05;

        p1.x += vx;
        p1.y += vy;

        if (p1.x < 0 || p1.x > Graphics::WIDTH) {
            vx *= -1;
            if (p1.x < 0) {
                p1.x += Graphics::WIDTH;
            } else {
                p1.x -= Graphics::WIDTH;
            }
        }
        if (p1.y < 0 || p1.y > Graphics::HEIGHT) {
            vy *= -1;
            if (p1.y < 0) {
                p1.y += Graphics::HEIGHT;
            } else {
                p1.y -= Graphics::HEIGHT;
            }
        }
    }
}

void interact_wrapper(int type1, int type2, float g, std::mutex &m1,
                      std::mutex &m2) {
    // printf("[ LOCK ] %d %d\n", type1, type2);
    std::scoped_lock lck{m1, m2};

    auto &particles1 = particles[type1];
    auto &particles2 = particles[type2];
    auto &particleVelocities1 = particleVelocities[type1];

    interact(particles1, particles2, particleVelocities1, g);

    // printf("[UNLOCK] %d %d\n", type1, type2);
}

void interact_wrapper0(int type1, float g, std::mutex &m1) {
    std::scoped_lock lck{m1};

    auto &particles1 = particles[type1];
    auto &particleVelocities1 = particleVelocities[type1];

    interact(particles1, particles1, particleVelocities1, g);

    // printf("[UNLOCK] %d %d\n", type1, type1);
}

void Particles::draw(SDL_Renderer *renderer) {
    std::thread threads[MaxColor + 1][MaxColor + 1];
    for (int col1 = MinColor; col1 < MaxColor + 1; col1++) {
        for (int col2 = MinColor; col2 < MaxColor + 1; col2++) {
            if (col1 == col2) {
                threadPool.submit(interact_wrapper0, col1,
                                  interactionMatrix[col1][col1],
                                  std::ref(particleMutexes[col1]));
            } else {
                threadPool.submit(interact_wrapper, col1, col2,
                                  interactionMatrix[col1][col2],
                                  std::ref(particleMutexes[col1]),
                                  std::ref(particleMutexes[col2]));
            }
        }
    }

    threadPool.join();

    for (int col = MinColor; col < MaxColor + 1; col++) {
        SDL_SetRenderDrawColor(renderer, colorValues[col].r, colorValues[col].g,
                               colorValues[col].b, colorValues[col].a);
        SDL_RenderDrawPointsF(renderer, particles[col].data(),
                              particles[col].size());
    }
}

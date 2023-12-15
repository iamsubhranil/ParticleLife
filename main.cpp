#include <time.h>

#include <cstdlib>

#include "graphics.h"

int main() {
    srand(time(NULL));
    Graphics::init();
    Graphics::run();
    Graphics::destroy();
}

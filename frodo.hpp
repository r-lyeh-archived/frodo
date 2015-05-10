// Frodo, a ring dependency framework. zlib/libpng licensed.
// - rlyeh ~~ listening to stoned jesus / i'm the mountain

#pragma once

#include <map>
#include <utility>
#include <functional>
#include <string>

namespace frodo {
    struct level {
        std::string name;
        std::function<bool()> init;
        std::function<bool()> quit;
    };

    bool ring( int lvl, const level &impl );
    bool init();
    bool alive();
    bool reboot( int lvl_target );
    bool quit();
}

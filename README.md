frodo <a href="https://travis-ci.org/r-lyeh/frodo"><img src="https://api.travis-ci.org/r-lyeh/frodo.svg?branch=master" align="right" /></a>
=====

- Frodo is a lightweight ring dependency framework (C++11).
- Frodo is tiny, header-only, cross-platform.
- Frodo is zlib/libpng licensed.

## Some theory
- Rings are made of independant systems, subsystems, libraries, singletons, etc.
- Inner rings provide functionality to outer rings.
- Initialization and deinitialization of rings must follow strict order.

## API
```c++
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
```

## Showcase
```c++
#include "frodo.hpp"
#include <iostream>

namespace memory {
    bool init() {
        std::cout << "[ OK ] mem setup" << std::endl;
        return true;
    }
    bool quit() {
        std::cout << "[ OK ] mem teardown" << std::endl;
        return true;
    }
}

namespace logger {
    bool init() {
        std::cout << "[ OK ] logger setup" << std::endl;
        return true;
    }
    bool quit() {
        std::cout << "[ OK ] logger teardown" << std::endl;
        return true;
    }
}

namespace console {
    bool init() {
        std::cout << "[ OK ] console setup" << std::endl;
        return true;
    }
    bool quit() {
        std::cout << "[ OK ] console teardown" << std::endl;
        return true;
    }
}

int main() {
    // app-defined levels
    // 00 memory and hooks
    frodo::ring(  0, { "memory", memory::init, memory::quit } );
    // 10 subsystems
    frodo::ring( 13, { "logger", logger::init, logger::quit } );
    frodo::ring( 14, { "console", console::init, console::quit } );
    // 20 devices
    //frodo::ring( 20, { "audio", audio::init, audio::quit } );
    //frodo::ring( 25, { "data", data::init, data::quit } );
    //frodo::ring( 27, { "input", input::init, input::quit } );
    // 30 opengl
    //frodo::ring( 34, { "ui", ui::init, ui::quit } );
    //frodo::ring( 30, { "opengl", gl::init, gl::quit } );
    //frodo::ring( 31, { "monitor", monitor::init, monitor::quit } );
    //frodo::ring( 35, { "font", font::init, font::quit } );
    // 40 game
    //frodo::ring( 40, { "model", model::init, model::quit } );
    //frodo::ring( 45, { "world", world::init, world::quit } );
    // 50 ui
    //frodo::ring( 59, { "help", help::init, help::quit } );

    if( frodo::init() ) {
        // app starts here
        std::string dummy;
        std::cout << "Press any key to continue... (try aborting or killing app too)" << std::endl;
        std::getline( std::cin, dummy );

        // shutdown
        if( frodo::quit() ) {
            return 0;
        }
    }

    return -1;
}
```

## Possible output
```c++
$frodo ./sample.out
[ OK ] mem setup
[ OK ] logger setup
[ OK ] console setup
Press any key to continue... (try aborting or killing app too)
^C
[ OK ] console teardown
[ OK ] logger teardown
[ OK ] mem teardown
```

## Todo
- `init({signal...})`
- `reboot(lvl)`

### Changelog
- v1.0.0 (2015/12/07): Header-only, simplified implementation
- v0.0.0 (2015/08/03): Initial commit

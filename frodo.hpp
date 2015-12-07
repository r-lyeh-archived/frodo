// Frodo, a ring dependency framework. zlib/libpng licensed.
// - rlyeh ~~ listening to stoned jesus / i'm the mountain

#pragma once

#define FRODO_VERSION "1.0.0" /* (2015/12/07) Header-only, simplified implementation
#define FRODO_VERSION "0.0.0" // (2015/08/03) Initial commit */

#include <map>
#include <utility>
#include <functional>
#include <string>

// public api

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

// api details

#include <cassert>
#include <cstdlib>
#include <signal.h>
#include <vector>
#include <map>

#ifdef _WIN32
#include <windows.h>
#endif

namespace frodo {

    struct singleton {
        std::multimap< int, frodo::level > map;
        std::vector< frodo::level > unmap;
        volatile sig_atomic_t is_exiting = 0, is_expected = 0;

        static void sigkill( int sig ) {
            /*
            The SIGKILL signal is used to cause immediate program termination. It cannot be handled or ignored, and is therefore always fatal. It is also not possible to block this signal.
            This signal is usually generated only by explicit request. Since it cannot be handled, you should generate it only as a last resort, after first trying a less drastic method such as C-c or SIGTERM. If a process does not respond to any other termination signals, sending it a SIGKILL signal will almost always cause it to go away.
            In fact, if SIGKILL fails to terminate a process, that by itself constitutes an operating system bug which you should report.
            The system will generate SIGKILL for a process itself under some unusual conditions where the program cannot possibly continue to run (even to run a signal handler).
            */
            singleton::get().is_exiting = 1;
        }
        static void sigterm( int sig ) {
            /*
            The SIGTERM signal is a generic signal used to cause program termination. Unlike SIGKILL, this signal can be blocked, handled, and ignored. It is the normal way to politely ask a program to terminate.
            The shell command kill generates SIGTERM by default.
            */
            singleton::get().is_expected = 1;
            singleton::get().is_exiting = 1;
        }
        static void sigint( int sig ) {
            /*
            The SIGINT (“program interrupt”) signal is sent when the user types the INTR character (normally C-c). See Special Characters, for information about terminal driver support for C-c.
            */
            singleton::get().is_expected = 1;
            singleton::get().is_exiting = 1;
        }
        static void sighup( int sig ) {
            /*
            The SIGHUP (“hang-up”) signal is used to report that the user’s terminal is disconnected, perhaps because a network or telephone connection was broken. For more information about this, see Control Modes.
            This signal is also used to report the termination of the controlling process on a terminal to jobs associated with that session; this termination effectively disconnects all processes in the session from the controlling terminal. For more information, see Termination Internals.
            */
            singleton::get().is_expected = 1;
            singleton::get().is_exiting = 1;
        }

        static void void_quit() {
            singleton::get().is_expected = 1;
            frodo::quit();
        }

        static singleton &get() {
            static singleton st;
            static bool installed = (std::atexit( void_quit ), signal(SIGTERM, sigterm), signal(SIGINT, sigint), true);
            return st;
        }
    };

    inline bool ring( int lvl, const level &def ) {
        auto &map = singleton::get().map;
        map.insert( {lvl, def } );
        return true;
    }

    inline bool quit() {
#ifdef _WIN32
        auto &is_expected = singleton::get().is_expected;
        if( !is_expected ) {
            if( IsDebuggerPresent() ) {
                assert( !"unexpected quit()" );
            }
        }
#endif
        auto &unmap = singleton::get().unmap;
        bool ok = true;
        if( !unmap.empty() ) {
            for( auto it = unmap.rbegin(), end = unmap.rend(); ok && it != end; ++it ) {
                const auto &ring = *it;
                if( ring.quit ) {
                    ok &= ring.quit();
                }
            }
            unmap.clear();
        }
        return ok;
    }

    inline bool init() {
        auto &map = singleton::get().map;
        auto &unmap = singleton::get().unmap;
        bool ok = true;
        if( unmap.empty() ) {
            for( auto it = map.begin(), end = map.end(); ok && it != end; ++it ) {
                const auto &ring = it->second;
                if( ring.init ) {
                    ok &= ring.init();
                    if( ok ) {
                        unmap.push_back( ring );
                    }
                }
            }
        }
        return ok;
    }

    inline bool alive() {
        auto &is_exiting = singleton::get().is_exiting;
        return is_exiting ? false : true;
    }

    inline bool reboot( int lvl_target ) {
        // @todo
        return true;
    }
}

#ifdef FRODO_BUILD_DEMO
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
#endif

// Frodo, a ring dependency framework. zlib/libpng licensed.
// - rlyeh ~~ listening to stoned jesus / i'm the mountain

#pragma once

#define FRODO_VERSION "1.0.1" /* (2016/04/11) No STL allocations; Implement reboot
#define FRODO_VERSION "1.0.0" // (2015/12/07) Header-only, simplified implementation
#define FRODO_VERSION "0.0.0" // (2015/08/03) Initial commit */

#include <stdio.h>

// public api

#ifndef FRODO_INSTALL_SIGHANDLERS
#define FRODO_INSTALL_SIGHANDLERS 0
#endif

namespace frodo {
    struct level {
        const char *name;
        bool (*init)();
        bool (*quit)();
    };

    bool ring( int lvl, const level &impl );
    bool init( bool display = false, int from_lvl = 0, int to_lvl = 256 );
    bool alive();
    bool reboot( bool display = false, int lvl = 0 );
    bool quit( bool display = false, int lvl = 0 );
}

// api details

#include <cassert>
#include <cstdlib>
#include <signal.h>

namespace frodo {

    struct singleton {
        frodo::level map[256];
        frodo::level unmap[256];
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
#           if FRODO_INSTALL_SIGHANDLERS
            static bool installed = (std::atexit( void_quit ), signal(SIGTERM, sigterm), signal(SIGINT, sigint), true);
#           endif
            return st;
        }
    };

    inline bool ring( int lvl, const level &def ) {
        auto &map = singleton::get().map;
        map[ lvl ] = def;
        return true;
    }

    inline bool init( bool display, int from, int to ) {
        auto &map = singleton::get().map;
        auto &unmap = singleton::get().unmap;
        bool ok = true;
        for( auto j = from; ok && j < to; ++j ) {
            const auto &ring = map[j];
            if( ring.init ) {
                if( display ) printf( "+ %s\n", ring.name);
                ok &= ring.init();
                if( ok ) {
                    unmap[ j ] = ring;
                }
            }
        }
        return ok;
    }

    inline bool quit( bool display, int lvl ) {
        auto &is_expected = singleton::get().is_expected;
        if( !is_expected ) {
            //assert( !"unexpected quit()" );
        }
        auto &unmap = singleton::get().unmap;
        bool ok = true;
        for( auto j = 256; ok && (--j >= lvl); ) {
            auto &ring = unmap[ j ];
            if( ring.quit ) {
                if( display ) printf( "- %s\n", ring.name);
                ok &= ring.quit();
            }
            ring.name = "";
            ring.init = 0;
            ring.quit = 0;
        }
        return ok;
    }

    inline bool alive() {
        auto &is_exiting = singleton::get().is_exiting;
        return is_exiting ? false : true;
    }

    inline bool reboot( bool display, int lvl ) {
        quit( display, lvl );
        init( display, lvl, 256 );
        return true;
    }
}

#ifdef FRODO_BUILD_DEMO
#include <iostream>
#include <string>

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

    // extra tests
    puts("-0- {");
    frodo::init(true);
    frodo::quit(true);
    puts("-0- }");

    puts("-1- {");

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

    if( frodo::init(true) ) {

        puts("-2- {");
        frodo::reboot(true, 13);
        puts("-2- }");


        // app starts here
        std::string dummy;
        std::cout << "Press any key to continue... (try aborting or killing app too)" << std::endl;
        std::getline( std::cin, dummy );

        // shutdown
        if( frodo::quit(true) ) {
            // ok
            //return 0;
        }
    }

    puts("-1- }");

    // extra tests
    puts("-3- {");
    frodo::init(true);
    frodo::quit(true);
    puts("-3- }");

#if FRODO_INSTALL_SIGHANDLERS
    // non-quit test
    puts("-4- {");
    frodo::init(true);
    puts("-4- }");
#endif

    return -1;
}
#endif

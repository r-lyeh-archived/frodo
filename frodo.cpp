// Frodo, a ring dependency framework. zlib/libpng licensed.
// - rlyeh ~~ listening to stoned jesus / i'm the mountain

#include <cstdlib>
#include <signal.h>

#include <iostream>
#include <vector>

#include "frodo.hpp"

#include <fstream>
#include <sstream>

#include <stdio.h>
#ifdef _MSC_VER
#include <io.h>
#else
#define _dup dup
#define _dup2 dup2
#define _close close
#define _fileno fileno
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <vector>
#include <string>
#include <ostream>
#include <cassert>

namespace {
    // taken from https://github.com/r-lyeh/wire {
    std::vector< std::string > tokenize( const std::string &self, const std::string &delimiters ) {
        std::string map( 256, '\0' );
        for( const unsigned char &ch : delimiters )
            map[ ch ] = '\1';
        std::vector< std::string > tokens(1);
        for( const unsigned char &ch : self ) {
            /**/ if( !map.at(ch)          ) tokens.back().push_back( char(ch) );
            else if( tokens.back().size() ) tokens.push_back( std::string() );
        }
        while( tokens.size() && !tokens.back().size() ) tokens.pop_back();
        return tokens;
    }
    // }
    struct capture {
        int fd, old, ch;
        capture( int ch, FILE *fp ) : ch(ch) {
            old = _dup( ch );
            fd = _fileno( fp );
            _dup2( fd, ch );
        }
        ~capture() {
            _dup2( old, ch );
        }
    };
}


namespace {
    std::multimap< int, frodo::level > map;
    std::vector< frodo::level > unmap;

    volatile sig_atomic_t is_exiting = 0, is_expected = 0;
    void sigkill( int sig ) {
        /*
        The SIGKILL signal is used to cause immediate program termination. It cannot be handled or ignored, and is therefore always fatal. It is also not possible to block this signal.
        This signal is usually generated only by explicit request. Since it cannot be handled, you should generate it only as a last resort, after first trying a less drastic method such as C-c or SIGTERM. If a process does not respond to any other termination signals, sending it a SIGKILL signal will almost always cause it to go away.
        In fact, if SIGKILL fails to terminate a process, that by itself constitutes an operating system bug which you should report.
        The system will generate SIGKILL for a process itself under some unusual conditions where the program cannot possibly continue to run (even to run a signal handler).
        */
        is_exiting = 1;
    }
    void sigterm( int sig ) {
        /*
        The SIGTERM signal is a generic signal used to cause program termination. Unlike SIGKILL, this signal can be blocked, handled, and ignored. It is the normal way to politely ask a program to terminate.
        The shell command kill generates SIGTERM by default.
        */
        is_expected = 1;
        is_exiting = 1;
    }
    void sigint( int sig ) {
        /*
        The SIGINT (“program interrupt”) signal is sent when the user types the INTR character (normally C-c). See Special Characters, for information about terminal driver support for C-c.
        */
        is_expected = 1;
        is_exiting = 1;
    }
    void sighup( int sig ) {
        /*
        The SIGHUP (“hang-up”) signal is used to report that the user’s terminal is disconnected, perhaps because a network or telephone connection was broken. For more information about this, see Control Modes.
        This signal is also used to report the termination of the controlling process on a terminal to jobs associated with that session; this termination effectively disconnects all processes in the session from the controlling terminal. For more information, see Termination Internals.
        */
        is_expected = 1;
        is_exiting = 1;
    }

    void void_quit() {
        is_expected = 1;
        frodo::quit();
    }

    bool install() {
        std::atexit( void_quit );
        signal(SIGTERM, sigterm);
        signal(SIGINT, sigint);
        return true;
    }
}

#define CAPTURE(log, ...) do { \
    { \
        FILE *fp1 = tmpfile(), *fp2 = tmpfile(); \
        { \
            capture c1( 1, fp1 ), c2( 2, fp2 ); \
            __VA_ARGS__; \
        } \
        { \
            rewind(fp1); rewind(fp2); \
            std::string ss1, ss2; \
            do { ss1 += fgetc(fp1); } while (!feof(fp1)); \
            do { ss2 += fgetc(fp2); } while (!feof(fp2)); \
            ss1.resize( ss1.size() - 1 ); \
            ss2.resize( ss2.size() - 1 ); \
            std::string clean; \
            for( auto &line : tokenize( log + ss1 + ss2, "\r\n") ) { \
                clean += std::string("       ") + line + "\n"; \
            } \
            log = clean; \
        } \
        fclose( fp1 ); \
        fclose( fp2 ); \
    } \
    if( log.size() ) { \
    std::clog << " log following:"; \
    } \
    std::clog << (ok ? "\r[ OK" : "\r[FAIL"); \
    std::clog << std::endl; \
    std::clog << log; \
} while(0)

namespace frodo {

    bool ring( int lvl, const level &def ) {
        map.insert( {lvl, def } );
        return true;
    }

    bool quit() {
        bool ok = true;
#ifdef _WIN32
		if( !is_expected ) {
			if( IsDebuggerPresent() ) {
				assert( !"unexpected quit()" );
			}
		}
#endif
        if( !unmap.empty() ) {
            for( auto it = unmap.rbegin(), end = unmap.rend(); ok && it != end; ++it ) {
                const auto &ring = *it;
                std::clog << "[    ] shutdown '" << ring.name << "' ...";
                std::string log;
                CAPTURE( log,
                    if( ring.quit ) {
                            ok &= ring.quit();
                        }
                );
            }
            unmap.clear();
        }
        return ok;
    }

    bool init() {
        static bool installed = install();

        bool ok = true;
        if( unmap.empty() ) {
            for( auto it = map.begin(), end = map.end(); ok && it != end; ++it ) {
                const auto &ring = it->second;
                std::clog << "[    ] setup '" << ring.name << "' ...";
                std::string log;
                CAPTURE(log,
                    if( ring.init ) {
                        ok &= ring.init();
                        if( ok ) {
                            unmap.push_back( ring );
                        }
                    }
                );
            }
        }
        return ok;
    }

    bool alive() {
        return is_exiting ? false : true;
    }

    bool reboot( int lvl_target ) {
        // @todo
        return true;
    }
}

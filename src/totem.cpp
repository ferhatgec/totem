// MIT License
//
// Copyright (c) 2021 Ferhat Geçdoğan All Rights Reserved.
// Distributed under the terms of the MIT License.
//
//

#include <fstream>
#include <sstream>

extern "C" {
    #include <unistd.h>
}

#if defined(__linux__)
    #include <sys/ioctl.h> // linux specific
#endif

#include "../libs/termiospp/include/termiospp"
#include "../include/totem.hpp"
#include "../include/totem_helpers.hpp"

void Totem::init(std::string const filename) noexcept {
    std::ifstream file(filename);

    for(std::string temp; std::getline(file, temp);
        this->file_data.append(temp + "\n")) { ++this->__down;
    } file.close(); this->__full_length = this->__down;

    #if defined(TIOCGSIZE)
        struct ttysize ts;
        if(ioctl(STDIN_FILENO, TIOCGSIZE, &ts) == 0) {
            this->__h = ts.ts_lines;
        }
    #elif defined(TIOCGWINSZ)
        struct winsize ts;
        if(ioctl(STDIN_FILENO, TIOCGWINSZ, &ts) == 0) {
            this->__h = ts.ws_row;
        }
    #endif // TIOCGWINSZ
}

void Totem::init_buffer() noexcept {
    clear(); to_up();
    this->__down = (this->__h / 1.05);

    this->from(false);
    disable_cursor();

    char ch;
    Termios new_termio,
            old_termio; TermiosFlag local_flag;
    term::get_attribute(0, &old_termio); new_termio = old_termio;
    new_termio.set_local_flag(local_flag &= ~TERMIOS_I_CANON);
    new_termio.set_local_flag(local_flag &= ~TERMIOS_ECHO   );
    term::set_attribute      (0, TERMIOS_SA_NOW, &new_termio     );
    while(1) { ch = getchar();
        if(ch == escape) {
            ch = getchar(); ch = getchar();

            switch(ch) {
                case up: {
                    if(this->__up >= 1
                        && this->__up < this->__full_length && this->__full_length > this->__h) {
                        --this->__up; --this->__down;
                        this->from(false);
                    } break;
                }

                case down: {
                    if(this->__down < this->__full_length) {
                        ++this->__down; ++this->__up;
                        this->from(false);
                    } break;
                }

                default: {
                    break;
                }
            } continue;
        }

        if(tolower(ch) == 'q') { break; }
    }

    enable_cursor(); clear();
    term::set_attribute(0, TERMIOS_SA_NOW, &old_termio);
}

void Totem::from(bool is_up) noexcept {
    std::istringstream stream(this->file_data);
    std::string __new; unsigned i = 0;

    if(is_up) {
        for(std::string temp; std::getline(stream, temp); i++) {
            if(i >= this->__up) {
                __new.append(temp + "\n");
            }
        }
    }
    else {
        for(std::string temp; std::getline(stream, temp); i++) {
            if(i < this->__down) {
                __new.append(temp + "\n");
            }
        }
    }

    clear(); print(__new); up_to(this->__up);
}

int main(int argc, char** argv) noexcept {
    if(argc < 2) { return 0; }

    Totem init;
    init.init(std::string(argv[argc - 1]));
    init.init_buffer();
}
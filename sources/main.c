#include "../include/gameboy.h"

int main(int argc, char **argv)
{
    GameboyClass *gameboy = new_class(Gameboy);
    int exit_code = gameboy->run(gameboy, argc, argv);
    destroy_class(gameboy);
    return exit_code;
}

# gameboy

<img align="right" src="./assets/logo.png" width="150px" alt="the gameboy logo" />

Welcome to my **GameBoy** emulator! It's still a _work in progress_ but it aims
to be a fully functional emulator.

- Emulated hardware: `CPU`, `GPU`, `memory` and `graphics`
- Wide compatibility with `ROM`s
- Real-time `assembly` instructions display
- Built with plain `C` for extra fun

# getting started

Clone the repo:

```sh
git clone git@github.com:Jabolol/gameboy.git .
```

Compile the program:

```sh
make
```

Execute the emulator:

```sh
./gameboy /path/to/rom.gb
```

# references

- [GameBoy Emulator Development](https://youtube.com/playlist?list=PLVxiWMqQvhg_yk4qy2cSC3457wZJga_e5)
- [Awesome References](https://gbdev.io/pandocs/)
- [LR35902 Instructions Set](https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html)

# gameboy

An accurate **gameboy** emulator written in C from scratch.

![](./assets/super-mario.png)

## development

In order to build the emulator, you need `cmake` 3.20 or newer, a C11 compiler
and `python3`. `SDL2` and `emsdk` are included as submodules, and `SDL2` is
compiled along with the project. The first configure installs the latest
`emscripten` through `emsdk`, so it needs an internet connection.

1. Clone the repository

```bash
git clone --recurse-submodules git@github.com:Jabolol/gameboy.git
cd gameboy
```

2. Create the `ROMs` directory with the ROMs to be loaded in the web version

```bash
mkdir ROMs && cp /path/to/rom.gb ROMs
```

> [!WARNING]
> The build fails if the `ROMs` directory is missing, so create it even if it
> stays empty.

3. Compile the project

```bash
cmake -B build && cmake --build build
```

Building the emulator also builds the web version into
`www/static/gameboy.{js,wasm}`.

4. Run the emulator

```bash
./build/gameboy /path/to/rom.gb
```

Battery saves are written next to the ROM as `rom.gb.sav`.

## features

- [x] Bus (Memory Management)
- [x] CPU (cycle accurate memory accesses, HALT bug, EI delay)
- [x] PPU (Graphics, access blocking, STAT interrupts, mid-line updates)
- [x] Game Boy Color support (double speed, VRAM/WRAM banks, palettes, HDMA)
- [x] Input (Joypad)
- [x] Timer
- [x] Serial (Link cable without partner)
- [x] Interrupts (V-Blank, LCD, Timer, Serial, Joypad)
- [x] MBC1, MBC1M, MBC2, MBC3 + RTC, MBC30, MBC5 + Rumble, MBC6, MBC7
- [x] MMM01, HuC1, HuC3, Pocket Camera and Wisdom Tree mappers
- [x] Battery saves (.sav files, RTC in the standard 48 byte format)
- [x] Save states and fast forward
- [x] Game controllers with rumble, touch controls on mobile
- [x] DMG palettes (gray, green) and GBC LCD color correction
- [x] Tile viewer colored with the live palettes
- [x] Sound (Square Wave, Wave, Noise, frame sequencer, high-pass filter)
- [x] Web version at [gameboy.monad.deno.net](https://gameboy.monad.deno.net/)

## controls

- `Arrow Keys` - D-Pad (also tilts MBC7 carts)
- `Z` - A
- `X` - B
- `U` - Volume Up
- `D` - Volume Down
- `Tab` - Select
- `Enter` - Start
- `Space` - Fast Forward (hold)
- `S` - Save State
- `L` - Load State
- `P` - Cycle DMG Palette
- `C` - Toggle LCD Color Correction
- `T` - Toggle Tile Viewer (desktop only)
- `Q` - Quit

Game controllers use the D-Pad or left stick, `B` (right) for A, `A` (bottom)
for B, `Start` for Start, `Back` for Select and the right shoulder to fast
forward.

## web version

The emulator is also available as a web version using `emscripten` and `deno`.
In order to run the web version, you need to have `deno` installed.

### game selection

The web version includes 28 games, each downloaded only when selected. Append
`?game=${game}` to the URL to load a specific game. If no game is specified, a
random game will be loaded. Your own ROMs can be loaded with the `Load ROM…`
entry of the selector or by dropping a `.gb`/`.gbc` file onto the screen.

The full list of games can be found in
[`www/utils/gameLoader.ts`](./www/utils/gameLoader.ts).

### ui controls

The web interface includes a control dock with the following features:

- **Canvas Scale** - Cycle between 1x, 2x, and 3x zoom levels (defaults to 1x on
  mobile, 3x on desktop)
- **Volume Control** - Adjust audio volume from 0 to 100% in 10% increments
- **Theme System** - Three available themes: `light`, `dark`, and `auto`
  (automatically detects the game's color palette by sampling canvas pixels and
  switches between light/dark themes accordingly in real-time)
- **Tiles Viewer** - Toggle visibility of the tiles in both VRAM banks, colored
  with the palettes that use them, and the current palettes
- **Save States** - Save and restore the whole machine state at any point
- **Fast Forward** - Run the emulation as fast as possible
- **Palette** - Cycle between the gray and green DMG palettes
- **LCD** - Emulate the washed out colors of the Game Boy Color screen
- **Controllers** - Standard gamepads are supported, with rumble, and touch
  devices get on-screen controls
- **Persistent Saves** - Battery saves and save states are kept in IndexedDB
- **Persistent State** - All settings (scale, volume, theme, tiles, palette,
  LCD) are saved to localStorage and restored on page load

### mobile

On phones the web version turns into a handheld. The screen sits in a replica of
the original Game Boy or a Game Boy Color case, with the D-pad, A/B and
Select/Start as touch controls. The case is picked from the settings menu behind
the gear, and the site can be added to the home screen to play full screen.

<div>
    <img src="./assets/mobile-classic.png" width="auto" height="400px" alt="Classic Game Boy case" />
    <img src="./assets/mobile-berry.png" width="auto" height="400px" alt="Berry Game Boy Color case" />
    <img src="./assets/mobile-grape.png" width="auto" height="400px" alt="Grape Game Boy Color case" />
    <img src="./assets/mobile-kiwi.png" width="auto" height="400px" alt="Kiwi Game Boy Color case" />
</div>
<div>
    <img src="./assets/mobile-dandelion.png" width="auto" height="400px" alt="Dandelion Game Boy Color case" />
    <img src="./assets/mobile-teal.png" width="auto" height="400px" alt="Teal Game Boy Color case" />
    <img src="./assets/mobile-dynamic.png" width="auto" height="400px" alt="Dynamic liquid glass case" />
    <img src="./assets/mobile-atomic-menu.png" width="auto" height="400px" alt="Settings menu on the Atomic Purple case" />
</div>

The Dynamic case is made of liquid glass tinted by the colors at the edge of the
game screen, with buttons in a contrasting color.

### running locally

> [!TIP]
> To enable google analytics, set the `GA4_MEASUREMENT_ID` environment variable
> to your GA4 measurement ID.

Build the project with `cmake` first. This generates `www/static/gameboy.js` and
`www/static/gameboy.wasm` and copies the ROMs available at `ROMs` directory into
`www/static/roms`. Then start the development server at `http://localhost:5173`.

```bash
deno task --cwd www dev
```

For a production build, run `deno task --cwd www build` followed by
`deno task --cwd www start`.

![](./www/static/web-ui.png)

## screenshots

> [Legend of Zelda, The - Link's Awakening](https://gameboy.monad.deno.net/?game=zelda)

![](./assets/zelda.png) ![](./assets/zelda-dx.png)

> [Pokemon - Yellow Version - Special Pikachu Edition](https://gameboy.monad.deno.net/?game=pokemon-yellow)

![](./assets/pokemon-yellow.png)

> [Pokemon - Crystal Version](https://gameboy.monad.deno.net/?game=pokemon-crystal)

![](./assets/pokemon-crystal.png)

> [Dr. Mario](https://gameboy.monad.deno.net/?game=dr-mario)

![](./assets/dr-mario.png) ![](./assets/dr-mario-dx.png)

> [Mega Man - Dr. Wily's Revenge](https://gameboy.monad.deno.net/?game=megaman-willy)

![](./assets/megaman.png)

> [Contra - The Alien Wars](https://gameboy.monad.deno.net/?game=contra)

![](./assets/contra.png)

> [Kirby - Dream Land](https://gameboy.monad.deno.net/?game=kirby-dream)

![](./assets/kirby.png) ![](./assets/kirby-dx.png)

> [Kirby - Dream Land 2](https://gameboy.monad.deno.net/?game=kirby-dream-2)

![](./assets/kirby-2.png) ![](./assets/kirby-2-dx.png)

> [Tetris](https://gameboy.monad.deno.net/?game=tetris)

![](./assets/tetris.png) ![](./assets/tetris-dx.png)

> [Super Mario Land](https://gameboy.monad.deno.net/?game=super-mario)

![](./assets/super-mario.png)

> [Super Mario Bros. Deluxe](https://gameboy.monad.deno.net/?game=super-mario-deluxe)

![](./assets/super-mario-dx.png)

> [Galaga](https://gameboy.monad.deno.net/?game=galaga-dx)

![](./assets/galaga.png) ![](./assets/galaga-dx.png)

## resources

- [Gameboy CPU Manual](http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf): A
  comprehensive guide to the Gameboy CPU.
- [Gameboy Opcodes](https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html):
  List of all opcodes for the Gameboy CPU.
- [Gameboy Pan Docs](https://gbdev.io/pandocs/): A detailed guide to the Gameboy
  hardware.
- [Game Boy: Complete Technical Reference](https://gekkio.fi/files/gb-docs/gbctr.pdf):
  Cycle level documentation of the Gameboy hardware.

## license

This project is licensed under the MIT License - see the [LICENSE](./LICENSE)
file for details.

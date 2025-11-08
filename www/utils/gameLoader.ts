interface EmscriptenModule {
  canvas: HTMLCanvasElement | null;
  arguments: string[];
}

declare global {
  var Module: EmscriptenModule | undefined;
}

const GAME_LIBRARY = [
  "asteroids.gb",
  "batman.gb",
  "contra.gb",
  "donkey-kong.gb",
  "dr-mario-dx.gb",
  "dr-mario.gb",
  "galaga-dx.gb",
  "kirby-dream-2.gb",
  "kirby-dream-dx.gb",
  "kirby-dream.gb",
  "megaman-willy.gb",
  "pokemon-crystal.gbc",
  "pokemon-gold.gbc",
  "pokemon-silver.gbc",
  "pokemon-yellow.gb",
  "super-mario-2.gb",
  "super-mario-dx.gbc",
  "super-mario.gb",
  "tetris-dx.gb",
  "tetris.gb",
  "trip-world.gb",
  "wario-land-3.gbc",
  "zelda-dx.gbc",
  "zelda.gb",
] as const;

type GameName = typeof GAME_LIBRARY[number];

function isValidGame(game: string): game is GameName {
  return (GAME_LIBRARY as readonly string[]).includes(game);
}

function normalizeGameName(game: string): string {
  const validExtensions = [".gb", ".gbc"];

  if (validExtensions.some((ext) => game.endsWith(ext))) {
    return game;
  }

  const candidates = [
    `${game}.gbc`,
    `${game}.gb`,
  ];

  for (const candidate of candidates) {
    if (isValidGame(candidate)) return candidate;
  }

  return game;
}

function getGameFromUrl(): GameName | null {
  if (typeof self === "undefined") return null;

  const urlParams = new URLSearchParams(self.location.search);
  const requestedGame = urlParams.get("game");

  if (!requestedGame) return null;

  const normalizedGame = normalizeGameName(requestedGame);
  return isValidGame(normalizedGame) ? normalizedGame : null;
}

function getRandomGame(): GameName {
  return GAME_LIBRARY[Math.floor(Math.random() * GAME_LIBRARY.length)];
}

function getGameToLoad(): GameName {
  return getGameFromUrl() ?? getRandomGame();
}

export function initializeGameboyModule(): void {
  if (typeof self === "undefined") return;

  const canvas = document.getElementById("canvas") as HTMLCanvasElement | null;
  const game = getGameToLoad();

  self.Module = {
    canvas,
    arguments: [`ROMs/${game}`],
  };
}

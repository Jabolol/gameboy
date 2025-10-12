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
  "dr-mario.gb",
  "kirby-dream.gb",
  "kirby-dream-2.gb",
  "megaman-willy.gb",
  "pokemon-yellow.gb",
  "super-mario.gb",
  "tetris.gb",
  "trip-world.gb",
  "zelda.gb",
] as const;

type GameName = typeof GAME_LIBRARY[number];

function isValidGame(game: string): game is GameName {
  return (GAME_LIBRARY as readonly string[]).includes(game);
}

function normalizeGameName(game: string): string {
  return game.endsWith(".gb") ? game : `${game}.gb`;
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

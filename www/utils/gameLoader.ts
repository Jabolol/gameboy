const GAME_LIBRARY = [
  "asteroids.gb",
  "batman.gb",
  "contra.gb",
  "donkey-kong.gb",
  "dr-mario-dx.gb",
  "dr-mario.gb",
  "galaga-dx.gb",
  "kirby-dream-2-dx.gb",
  "kirby-dream-2.gb",
  "kirby-dream-dx.gb",
  "kirby-dream.gb",
  "kirby-tilt-n-tumble.gbc",
  "megaman-v-dx.gb",
  "megaman-willy.gb",
  "metal-gear-solid.gbc",
  "pokemon-crystal.gbc",
  "pokemon-gold.gbc",
  "pokemon-silver.gbc",
  "pokemon-yellow.gb",
  "super-mario-deluxe.gbc",
  "super-mario.gb",
  "tetris-dx.gb",
  "tetris.gb",
  "trip-world.gb",
  "wario-land-3.gbc",
  "zelda-dx.gbc",
  "zelda-oracle-of-ages.gbc",
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
  if (typeof self === "undefined" || !self.location) return null;

  const urlParams = new URLSearchParams(self.location.search);
  const requestedGame = urlParams.get("game");

  if (!requestedGame) return null;

  const normalizedGame = normalizeGameName(requestedGame);
  return isValidGame(normalizedGame) ? normalizedGame : null;
}

function getRandomGame(): GameName {
  return GAME_LIBRARY[Math.floor(Math.random() * GAME_LIBRARY.length)];
}

export function getGameToLoad(): GameName {
  return getGameFromUrl() ?? getRandomGame();
}

export function getCurrentGame(): GameName | null {
  return getGameFromUrl();
}

export async function fetchRom(game: GameName): Promise<Uint8Array> {
  const response = await fetch(`/roms/${game}`);
  if (!response.ok) throw new Error(`Failed to fetch ${game}`);
  return new Uint8Array(await response.arrayBuffer());
}

const GAME_TITLES: Record<GameName, string> = {
  "asteroids.gb": "Asteroids",
  "batman.gb": "Batman",
  "contra.gb": "Contra: The Alien Wars",
  "donkey-kong.gb": "Donkey Kong",
  "dr-mario-dx.gb": "Dr. Mario DX",
  "dr-mario.gb": "Dr. Mario",
  "galaga-dx.gb": "Galaga DX",
  "kirby-dream-2-dx.gb": "Kirby's Dream Land 2 DX",
  "kirby-dream-2.gb": "Kirby's Dream Land 2",
  "kirby-dream-dx.gb": "Kirby's Dream Land DX",
  "kirby-dream.gb": "Kirby's Dream Land",
  "kirby-tilt-n-tumble.gbc": "Kirby Tilt 'n' Tumble",
  "megaman-v-dx.gb": "Mega Man V DX",
  "megaman-willy.gb": "Mega Man: Dr. Wily's Revenge",
  "metal-gear-solid.gbc": "Metal Gear Solid",
  "pokemon-crystal.gbc": "Pokémon Crystal",
  "pokemon-gold.gbc": "Pokémon Gold",
  "pokemon-silver.gbc": "Pokémon Silver",
  "pokemon-yellow.gb": "Pokémon Yellow",
  "super-mario-deluxe.gbc": "Super Mario Bros. Deluxe",
  "super-mario.gb": "Super Mario Land",
  "tetris-dx.gb": "Tetris DX",
  "tetris.gb": "Tetris",
  "trip-world.gb": "Trip World",
  "wario-land-3.gbc": "Wario Land 3",
  "zelda-dx.gbc": "Link's Awakening DX",
  "zelda-oracle-of-ages.gbc": "Oracle of Ages",
  "zelda.gb": "Link's Awakening",
};

export function formatGameName(game: string): string {
  if (isValidGame(game)) return GAME_TITLES[game];
  return game
    .replace(/\.gb[c]?$/, "")
    .split("-")
    .map((word) => word.charAt(0).toUpperCase() + word.slice(1))
    .join(" ");
}

export const GAME_OPTIONS = GAME_LIBRARY
  .map((game) => ({ value: game, label: formatGameName(game) }))
  .sort((a, b) => a.label.localeCompare(b.label));

export { GAME_LIBRARY, isValidGame };
export type { GameName };

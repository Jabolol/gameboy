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
];

function getGameToLoad() {
  const urlParams = new URLSearchParams(window.location.search);
  const requestedGame = urlParams.get("game");

  if (requestedGame) {
    const normalizedGame = requestedGame.endsWith(".gb")
      ? requestedGame
      : `${requestedGame}.gb`;

    if (GAME_LIBRARY.includes(normalizedGame)) {
      return normalizedGame;
    }
  }

  return GAME_LIBRARY[Math.floor(Math.random() * GAME_LIBRARY.length)];
}

globalThis.Module = {
  canvas: document.getElementById("canvas"),
  arguments: [`ROMs/${getGameToLoad()}`],
};

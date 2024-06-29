const games = [
  "asteroids.gb",
  "batman.gb",
  "contra.gb",
  "donkey-kong.gb",
  "dr-mario.gb",
  "kirby-dream.gb",
  "megaman-willy.gb",
  "super-mario.gb",
  "tetris.gb",
  "trip-world.gb",
  "zelda.gb",
];

let target = games[Math.floor(Math.random() * games.length)];
const urlParams = new URLSearchParams(window.location.search);
const game = urlParams.get("game");

if (game) {
  if (games.includes(game) || games.includes(`${game}.gb`)) {
    target = game.endsWith(".gb") ? game : `${game}.gb`;
  }
}

globalThis.Module = {
  canvas: (() => document.getElementById("canvas"))(),
  arguments: [`ROMs/${target}`],
};

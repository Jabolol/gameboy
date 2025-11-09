import { useState } from "preact/hooks";
import { formatGameName, GAME_LIBRARY } from "../utils/gameLoader.ts";
import type { GameName } from "../utils/gameLoader.ts";

interface GameSelectorProps {
  currentGame: GameName;
}

export function GameSelector({ currentGame }: GameSelectorProps) {
  const [isLoading, setIsLoading] = useState(false);

  const handleGameChange = (event: Event) => {
    const target = event.target as HTMLSelectElement;
    const selectedGame = target.value;

    if (
      selectedGame && selectedGame !== currentGame &&
      typeof self !== "undefined"
    ) {
      setIsLoading(true);
      const url = new URL(self.location.href);
      url.searchParams.set("game", selectedGame);
      self.location.href = url.toString();
    }
  };

  return (
    <div class="relative">
      <select
        id="game-select"
        value={currentGame}
        onChange={handleGameChange}
        disabled={isLoading}
        class={`pr-6 pl-2 py-1 text-sm font-medium bg-transparent transition-all cursor-pointer outline-none focus:outline-none appearance-none ${
          isLoading
            ? "text-gray-400 dark:text-gray-600 cursor-wait"
            : "text-gray-900 dark:text-gray-100 hover:text-gray-600 dark:hover:text-gray-400"
        }`}
        aria-label="Select game"
      >
        {GAME_LIBRARY.map((game) => (
          <option key={game} value={game}>
            {formatGameName(game)}
          </option>
        ))}
      </select>
      {!isLoading && (
        <div class="absolute inset-y-0 right-0 flex items-center pr-1 pointer-events-none">
          <svg
            xmlns="http://www.w3.org/2000/svg"
            width="12"
            height="12"
            viewBox="0 0 12 12"
            class="text-gray-900 dark:text-gray-100"
          >
            <path d="M6 8L2 4h8z" fill="currentColor" />
          </svg>
        </div>
      )}
      {isLoading && (
        <div class="absolute inset-0 flex items-center justify-end pr-2 pointer-events-none">
          <div class="w-3 h-3 border-2 border-gray-400 dark:border-gray-600 border-t-transparent rounded-full animate-spin" />
        </div>
      )}
    </div>
  );
}

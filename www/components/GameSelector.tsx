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
    <div class="relative shrink-0">
      <select
        id="game-select"
        value={currentGame}
        onChange={handleGameChange}
        disabled={isLoading}
        class={`pr-5 sm:pr-6 pl-1.5 sm:pl-2 py-1 text-xs sm:text-sm font-medium bg-transparent transition-all cursor-pointer outline-none focus:outline-none appearance-none max-w-[120px] sm:max-w-none truncate ${
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
        <div class="absolute inset-y-0 right-0 flex items-center pr-0.5 sm:pr-1 pointer-events-none">
          <svg
            xmlns="http://www.w3.org/2000/svg"
            width="12"
            height="12"
            viewBox="0 0 12 12"
            class="text-gray-900 dark:text-gray-100 w-2.5 h-2.5 sm:w-3 sm:h-3"
          >
            <path d="M6 8L2 4h8z" fill="currentColor" />
          </svg>
        </div>
      )}
      {isLoading && (
        <div class="absolute inset-0 flex items-center justify-end pr-1 sm:pr-2 pointer-events-none">
          <div class="w-2.5 h-2.5 sm:w-3 sm:h-3 border-2 border-gray-400 dark:border-gray-600 border-t-transparent rounded-full animate-spin" />
        </div>
      )}
    </div>
  );
}

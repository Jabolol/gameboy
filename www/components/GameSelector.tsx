import { formatGameName, GAME_LIBRARY } from "../utils/gameLoader.ts";
import type { GameName } from "../utils/gameLoader.ts";

interface GameSelectorProps {
  currentGame: GameName;
  onGameChange: (game: GameName) => void;
}

export function GameSelector({ currentGame, onGameChange }: GameSelectorProps) {
  const handleGameChange = (event: Event) => {
    const selectedGame = (event.target as HTMLSelectElement).value as GameName;
    if (selectedGame !== currentGame) {
      onGameChange(selectedGame);
    }
  };

  return (
    <div class="relative shrink-0">
      <select
        id="game-select"
        value={currentGame}
        onChange={handleGameChange}
        class="pr-5 sm:pr-6 pl-1.5 sm:pl-2 py-1 text-xs sm:text-sm font-medium bg-transparent transition-all cursor-pointer outline-none focus:outline-none appearance-none max-w-[120px] sm:max-w-none truncate text-gray-900 dark:text-gray-100 hover:text-gray-600 dark:hover:text-gray-400"
        aria-label="Select game"
      >
        {GAME_LIBRARY.map((game) => (
          <option key={game} value={game}>
            {formatGameName(game)}
          </option>
        ))}
      </select>
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
    </div>
  );
}

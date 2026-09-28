import { useRef } from "preact/hooks";
import { GAME_OPTIONS } from "../utils/gameLoader.ts";
import { CheckIcon } from "./icons/CheckIcon.tsx";
import { LoadRomIcon } from "./icons/LoadRomIcon.tsx";
import { SpinnerIcon } from "./icons/SpinnerIcon.tsx";

interface GameListProps {
  currentGame: string | null;
  pendingGame?: string;
  onSelect: (game: string) => void;
  onRomFile: (file: File) => void;
}

const ROW_CLASSES =
  "flex items-center gap-2 w-full h-10 pl-2 pr-3 rounded-2xl text-left text-[15px] hover:bg-[var(--fill)] active:bg-[var(--fill-strong)]";

export function GameList(
  { currentGame, pendingGame, onSelect, onRomFile }: GameListProps,
) {
  const fileInput = useRef<HTMLInputElement>(null);

  const handleFile = (event: Event) => {
    const input = event.target as HTMLInputElement;
    const file = input.files?.[0];
    if (file) onRomFile(file);
    input.value = "";
  };

  return (
    <>
      <ul aria-label="Games" class="px-0.5">
        {GAME_OPTIONS.map(({ value, label }) => (
          <li key={value}>
            <button
              type="button"
              class={ROW_CLASSES}
              aria-current={value === currentGame || undefined}
              onClick={() => onSelect(value)}
            >
              <span class="w-5 flex justify-center shrink-0">
                {value === pendingGame
                  ? <SpinnerIcon class="w-4 h-4 motion-safe:animate-spin" />
                  : value === currentGame && <CheckIcon class="w-4 h-4" />}
              </span>
              <span class="truncate">{label}</span>
            </button>
          </li>
        ))}
      </ul>
      <div class="h-px mx-3 my-1 bg-[var(--separator)]" />
      <button
        type="button"
        class={ROW_CLASSES}
        onClick={() => fileInput.current?.click()}
      >
        <span class="w-5 flex justify-center shrink-0">
          <LoadRomIcon class="w-4 h-4" />
        </span>
        Open a ROM file…
      </button>
      <input
        ref={fileInput}
        type="file"
        accept=".gb,.gbc"
        class="hidden"
        onChange={handleFile}
      />
    </>
  );
}

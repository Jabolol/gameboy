import { useEffect, useMemo, useRef, useState } from "preact/hooks";
import { useDismiss } from "../hooks/useDismiss.ts";
import {
  formatGameName,
  GAME_OPTIONS,
  isValidGame,
} from "../utils/gameLoader.ts";
import { CheckIcon } from "./icons/CheckIcon.tsx";
import { ChevronUpDownIcon } from "./icons/ChevronUpDownIcon.tsx";
import { LoadRomIcon } from "./icons/LoadRomIcon.tsx";
import { SpinnerIcon } from "./icons/SpinnerIcon.tsx";
import { Popover } from "./Popover.tsx";

interface GameSelectorProps {
  currentGame: string | null;
  pendingGame?: string;
  onGameChange: (game: string) => void;
  onRomFile: (file: File) => void;
  open: boolean;
  onOpenChange: (open: boolean) => void;
}

interface Option {
  value: string;
  label: string;
}

const LOAD_ROM = "load-rom";
const LIST_ID = "game-list";
const PAGE_SIZE = 8;
const TYPEAHEAD_RESET_MS = 600;

const optionId = (index: number) => `${LIST_ID}-${index}`;

export function GameSelector(
  { currentGame, pendingGame, onGameChange, onRomFile, open, onOpenChange }:
    GameSelectorProps,
) {
  const triggerRef = useRef<HTMLButtonElement>(null);
  const panelRef = useRef<HTMLDivElement>(null);
  const fileInput = useRef<HTMLInputElement>(null);
  const typeahead = useRef({ query: "", time: 0 });

  const options = useMemo<Option[]>(() => [
    ...(!currentGame || isValidGame(currentGame)
      ? []
      : [{ value: currentGame, label: formatGameName(currentGame) }]),
    ...GAME_OPTIONS,
    { value: LOAD_ROM, label: "Open a ROM file…" },
  ], [currentGame]);

  const selected = options.findIndex((option) => option.value === currentGame);
  const [active, setActive] = useState(Math.max(0, selected));
  const loading = !!pendingGame || !currentGame;
  const title = pendingGame ?? currentGame;

  const close = () => onOpenChange(false);
  useDismiss(open, close, [triggerRef, panelRef]);

  useEffect(() => {
    if (open) setActive(Math.max(0, selected));
  }, [open]);

  useEffect(() => {
    if (open) {
      document.getElementById(optionId(active))?.scrollIntoView({
        block: "nearest",
      });
    }
  }, [open, active]);

  const choose = (index: number) => {
    const { value } = options[index];
    close();
    if (value === LOAD_ROM) {
      fileInput.current?.click();
      return;
    }
    if (value !== currentGame) onGameChange(value);
    document.getElementById("canvas")?.focus();
  };

  const findByPrefix = (key: string) => {
    const now = performance.now();
    const state = typeahead.current;
    state.query = now - state.time > TYPEAHEAD_RESET_MS
      ? key
      : state.query + key;
    state.time = now;

    const query = state.query.toLowerCase();
    const start = state.query.length === 1 ? active + 1 : active;
    const ordered = [...options.slice(start), ...options.slice(0, start)];
    const match = ordered.find((option) =>
      option.label.toLowerCase().startsWith(query)
    );
    return match ? options.indexOf(match) : -1;
  };

  const onKeyDown = (event: KeyboardEvent) => {
    const trigger = event.currentTarget as HTMLElement;
    if (!open && !trigger.matches(":focus-visible")) return;

    event.stopPropagation();
    const last = options.length - 1;
    const move = (index: number) => {
      event.preventDefault();
      if (!open) onOpenChange(true);
      setActive(Math.min(last, Math.max(0, index)));
    };

    if (event.key.length === 1 && event.key !== " " && !event.metaKey) {
      const match = findByPrefix(event.key);
      if (match >= 0) move(match);
      return;
    }

    switch (event.key) {
      case "ArrowDown":
        return move(open ? active + 1 : selected);
      case "ArrowUp":
        return move(open ? active - 1 : selected);
      case "Home":
        return move(0);
      case "End":
        return move(last);
      case "PageDown":
        return move(active + PAGE_SIZE);
      case "PageUp":
        return move(active - PAGE_SIZE);
      case "Enter":
      case " ":
        event.preventDefault();
        return open ? choose(active) : onOpenChange(true);
      case "Tab":
        if (open) close();
    }
  };

  const handleFile = (event: Event) => {
    const input = event.target as HTMLInputElement;
    const file = input.files?.[0];
    if (file) onRomFile(file);
    input.value = "";
  };

  const loadRomIndex = options.length - 1;

  return (
    <>
      <button
        ref={triggerRef}
        type="button"
        role="combobox"
        aria-label="Game"
        aria-haspopup="listbox"
        aria-expanded={open}
        aria-controls={LIST_ID}
        aria-activedescendant={open ? optionId(active) : undefined}
        aria-busy={loading}
        class="flex items-center gap-1.5 h-10 pl-4 pr-3 rounded-full min-w-0 text-[15px] font-semibold tracking-[-0.01em] hover:bg-[var(--fill)] aria-expanded:bg-[var(--fill)]"
        onClick={() => {
          triggerRef.current?.focus({ preventScroll: true });
          onOpenChange(!open);
        }}
        onKeyDown={onKeyDown}
      >
        <span class="truncate max-w-[8.5rem] sm:max-w-[13rem]">
          {title ? formatGameName(title) : "Loading…"}
        </span>
        {loading
          ? (
            <SpinnerIcon class="w-3.5 h-3.5 shrink-0 text-[var(--label-secondary)] motion-safe:animate-spin" />
          )
          : (
            <ChevronUpDownIcon class="w-3.5 h-3.5 shrink-0 text-[var(--label-secondary)]" />
          )}
      </button>

      <Popover
        open={open}
        align="start"
        panelRef={panelRef}
        class="w-[min(18rem,calc(100vw-2rem))]"
      >
        <ul
          id={LIST_ID}
          role="listbox"
          aria-label="Games"
          class="max-h-[min(24rem,55dvh)] overflow-y-auto overscroll-contain p-1.5 scroll-py-1.5"
        >
          {options.map((option, index) => (
            <li
              key={option.value}
              id={optionId(index)}
              role="option"
              aria-selected={index === selected}
              data-active={index === active || undefined}
              class={`flex items-center gap-2 h-9 pl-2 pr-3 rounded-2xl cursor-default select-none text-[15px] data-active:bg-[var(--fill-strong)] ${
                index === loadRomIndex
                  ? "mt-1.5 relative before:absolute before:-top-1 before:inset-x-3 before:h-px before:bg-[var(--separator)]"
                  : ""
              }`}
              onPointerMove={() => setActive(index)}
              onClick={() => choose(index)}
            >
              <span class="w-5 flex justify-center shrink-0">
                {index === loadRomIndex
                  ? <LoadRomIcon class="w-4 h-4" />
                  : index === selected && <CheckIcon class="w-4 h-4" />}
              </span>
              <span class="truncate">{option.label}</span>
            </li>
          ))}
        </ul>
      </Popover>

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

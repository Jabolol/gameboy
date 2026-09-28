import { useEffect, useRef, useState } from "preact/hooks";
import { useDismiss } from "../hooks/useDismiss.ts";
import type { Scale } from "../types/canvas.ts";
import type { Palette } from "../types/emulator.ts";
import { type CaseId, CASES } from "../types/handheld.ts";
import type { Theme } from "../types/theme.ts";
import { formatGameName } from "../utils/gameLoader.ts";
import type { Snapshot } from "../utils/snapshots.ts";
import { GameList } from "./GameList.tsx";
import { CartridgeIcon } from "./icons/CartridgeIcon.tsx";
import { ArrowUpRightIcon } from "./icons/ArrowUpRightIcon.tsx";
import { ChevronRightIcon } from "./icons/ChevronRightIcon.tsx";
import { DynamicIcon } from "./icons/DynamicIcon.tsx";
import { EllipsisIcon } from "./icons/EllipsisIcon.tsx";
import { FastForwardIcon } from "./icons/FastForwardIcon.tsx";
import { GearIcon } from "./icons/GearIcon.tsx";
import { GitHubIcon } from "./icons/GitHubIcon.tsx";
import { HandheldIcon } from "./icons/HandheldIcon.tsx";
import { LcdIcon } from "./icons/LcdIcon.tsx";
import { LoadIcon } from "./icons/LoadIcon.tsx";
import { MoonIcon } from "./icons/MoonIcon.tsx";
import { PaletteIcon } from "./icons/PaletteIcon.tsx";
import { ResizeIcon } from "./icons/ResizeIcon.tsx";
import { SaveIcon } from "./icons/SaveIcon.tsx";
import { SunIcon } from "./icons/SunIcon.tsx";
import { TilesIcon } from "./icons/TilesIcon.tsx";
import {
  type Choice,
  MenuAction,
  MenuChoice,
  MenuLink,
  MenuSeparator,
  MenuSwatches,
  MenuSwitch,
  type Swatch,
} from "./Menu.tsx";
import { Popover } from "./Popover.tsx";
import { SnapshotList } from "./SnapshotList.tsx";
import { VolumeControl } from "./VolumeControl.tsx";

export interface Settings {
  turbo: boolean;
  scale: Scale;
  theme: Theme;
  palette: Palette;
  colorCorrection: boolean;
  showTiles: boolean;
  handheld: CaseId;
}

interface SettingsMenuProps {
  settings: Settings;
  touch: boolean;
  currentGame: string | null;
  pendingGame?: string;
  onGameChange: (game: string) => void;
  onRomFile: (file: File) => void;
  volume: number;
  onVolumeChange: (volume: number) => void;
  onChange: <K extends keyof Settings>(key: K, value: Settings[K]) => void;
  snapshots: Snapshot[];
  onSaveSnapshot: () => void;
  onRestoreSnapshot: (id: string) => void;
  onDeleteSnapshot: (id: string) => void;
  open: boolean;
  onOpenChange: (open: boolean) => void;
}

interface PageHeaderProps {
  title: string;
  subtitle: string;
  onBack: () => void;
}

type Page = "settings" | "snapshots" | "games";

const PAGE_TITLES: Record<Page, string> = {
  settings: "More options",
  snapshots: "Snapshots",
  games: "Games",
};

const PANEL_ID = "settings-menu";

const SCALES: Choice<Scale>[] = [
  { value: "fit", label: "Fit" },
  { value: 1, label: "1×" },
  { value: 2, label: "2×" },
  { value: 3, label: "3×" },
];

const THEMES: Choice<Theme>[] = [
  { value: "light", label: "Light", icon: SunIcon },
  { value: "dark", label: "Dark", icon: MoonIcon },
  { value: "auto", label: "Dynamic", icon: DynamicIcon },
];

const HANDHELDS: Swatch<CaseId>[] = [
  ...CASES.map(({ id, name, body, translucent }) => ({
    value: id,
    label: name,
    fill: `rgb(${body.join(" ")} / ${translucent ? 0.7 : 1})`,
  })),
  {
    value: "dynamic",
    label: "Dynamic",
    fill:
      "conic-gradient(from 200deg, #ff6b8b, #ffc56b, #8ee57a, #6bb8ff, #b48bff, #ff6b8b)",
  },
];

const PALETTES: Choice<Palette>[] = [
  { value: "gray", label: "Gray" },
  { value: "green", label: "Green" },
];

function PageHeader({ title, subtitle, onBack }: PageHeaderProps) {
  return (
    <button
      type="button"
      class="flex items-center gap-2 w-full min-h-11 px-3 rounded-2xl text-left hover:bg-[var(--fill)]"
      onClick={onBack}
    >
      <ChevronRightIcon class="w-4 h-4 rotate-180 shrink-0" />
      <span class="flex flex-col min-w-0 leading-tight">
        <span class="text-[15px] font-semibold">{title}</span>
        <span class="text-[13px] text-[var(--label-secondary)] truncate">
          {subtitle}
        </span>
      </span>
    </button>
  );
}

export function SettingsMenu(
  {
    settings,
    touch,
    currentGame,
    pendingGame,
    onGameChange,
    onRomFile,
    volume,
    onVolumeChange,
    onChange,
    snapshots,
    onSaveSnapshot,
    onRestoreSnapshot,
    onDeleteSnapshot,
    open,
    onOpenChange,
  }: SettingsMenuProps,
) {
  const triggerRef = useRef<HTMLButtonElement>(null);
  const panelRef = useRef<HTMLDivElement>(null);
  const [page, setPage] = useState<Page>("settings");

  const close = () => onOpenChange(false);
  useDismiss(open, close, [triggerRef, panelRef]);

  useEffect(() => {
    if (!open) setPage("settings");
  }, [open]);

  useEffect(() => {
    const panel = panelRef.current;
    if (open) {
      panel?.querySelector<HTMLElement>("[role=dialog]")?.focus({
        preventScroll: true,
      });
    } else if (panel?.contains(document.activeElement)) {
      triggerRef.current?.focus({ preventScroll: true });
    }
  }, [open, page]);

  const save = () => {
    close();
    onSaveSnapshot();
  };

  const restore = (id: string) => {
    close();
    onRestoreSnapshot(id);
  };

  const selectGame = (game: string) => {
    close();
    onGameChange(game);
  };

  const openRom = (file: File) => {
    close();
    onRomFile(file);
  };

  const gameTitle = currentGame ? formatGameName(currentGame) : "";

  return (
    <>
      <button
        ref={triggerRef}
        type="button"
        class={touch
          ? "case-gear grid place-items-center w-9 h-9 rounded-full"
          : "grid place-items-center w-10 h-10 rounded-full hover:bg-[var(--fill)] aria-expanded:bg-[var(--fill)]"}
        aria-label="More options"
        title="More options"
        aria-haspopup="dialog"
        aria-expanded={open}
        aria-controls={PANEL_ID}
        onClick={() => onOpenChange(!open)}
      >
        {touch
          ? <GearIcon class="w-[22px] h-[22px]" />
          : <EllipsisIcon class="w-5 h-5" />}
      </button>

      <Popover
        open={open}
        align="end"
        side={touch ? "below" : "above"}
        panelRef={panelRef}
        class="w-[min(21rem,calc(100vw-2rem))]"
      >
        <div
          id={PANEL_ID}
          role="dialog"
          aria-label={PAGE_TITLES[page]}
          tabIndex={-1}
          class="p-1.5 outline-none max-h-[70dvh] overflow-y-auto overscroll-contain"
        >
          {page === "settings" && (
            <>
              {touch && (
                <>
                  <MenuAction
                    icon={CartridgeIcon}
                    label={gameTitle || "Loading…"}
                    detail="Change the game"
                    trailing={<ChevronRightIcon class="w-4 h-4" />}
                    onClick={() => setPage("games")}
                  />
                  <div class="px-1.5 py-0.5">
                    <VolumeControl
                      volume={volume}
                      onVolumeChange={onVolumeChange}
                      wide
                    />
                  </div>
                  <MenuSeparator />
                </>
              )}
              <MenuAction
                icon={SaveIcon}
                label="Save snapshot"
                detail="Keep the game exactly as it is now"
                onClick={save}
              />
              <MenuAction
                icon={LoadIcon}
                label="Load snapshot"
                detail={snapshots.length
                  ? `${snapshots.length} saved for this game`
                  : "None saved for this game yet"}
                trailing={<ChevronRightIcon class="w-4 h-4" />}
                onClick={() => setPage("snapshots")}
              />
              <MenuSwitch
                icon={FastForwardIcon}
                label="Fast forward"
                detail="Run the game as fast as possible"
                checked={settings.turbo}
                onChange={(value) => onChange("turbo", value)}
              />

              <MenuSeparator />

              <MenuChoice
                icon={ResizeIcon}
                label="Size"
                choices={SCALES}
                value={settings.scale}
                onChange={(value) => onChange("scale", value)}
              />
              {touch
                ? (
                  <MenuSwatches
                    icon={HandheldIcon}
                    label="Case"
                    detail={HANDHELDS.find(({ value }) =>
                      value === settings.handheld
                    )?.label}
                    swatches={HANDHELDS}
                    value={settings.handheld}
                    onChange={(value) => onChange("handheld", value)}
                  />
                )
                : (
                  <MenuChoice
                    icon={SunIcon}
                    label="Theme"
                    detail={THEMES.find(({ value }) => value === settings.theme)
                      ?.label}
                    choices={THEMES}
                    value={settings.theme}
                    onChange={(value) => onChange("theme", value)}
                  />
                )}
              <MenuChoice
                icon={PaletteIcon}
                label="Palette"
                detail="Original Game Boy games"
                choices={PALETTES}
                value={settings.palette}
                onChange={(value) => onChange("palette", value)}
              />
              <MenuSwitch
                icon={LcdIcon}
                label="Color correction"
                detail="Mimic the Game Boy Color screen"
                checked={settings.colorCorrection}
                onChange={(value) => onChange("colorCorrection", value)}
              />
              {!touch && (
                <MenuSwitch
                  icon={TilesIcon}
                  label="Tile viewer"
                  detail="Show the graphics in video memory"
                  checked={settings.showTiles}
                  onChange={(value) => onChange("showTiles", value)}
                />
              )}

              <MenuSeparator />

              <MenuLink
                icon={GitHubIcon}
                label="Source code"
                href="https://github.com/Jabolol/gameboy"
                trailing={<ArrowUpRightIcon class="w-4 h-4" />}
              />
            </>
          )}
          {page === "snapshots" && (
            <>
              <PageHeader
                title="Snapshots"
                subtitle={gameTitle}
                onBack={() => setPage("settings")}
              />
              <SnapshotList
                snapshots={snapshots}
                onRestore={restore}
                onDelete={onDeleteSnapshot}
              />
            </>
          )}
          {page === "games" && (
            <>
              <PageHeader
                title="Games"
                subtitle={gameTitle}
                onBack={() => setPage("settings")}
              />
              <GameList
                currentGame={currentGame}
                pendingGame={pendingGame}
                onSelect={selectGame}
                onRomFile={openRom}
              />
            </>
          )}
        </div>
      </Popover>
    </>
  );
}

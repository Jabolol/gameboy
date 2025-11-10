import { useEffect, useRef } from "preact/hooks";
import { ControlButton } from "../components/ControlButton.tsx";
import { Dock, DockDivider } from "../components/Dock.tsx";
import { GameSelector } from "../components/GameSelector.tsx";
import { GitHubIcon } from "../components/icons/GitHubIcon.tsx";
import { ThemeButton } from "../components/ThemeButton.tsx";
import { TilesIcon } from "../components/icons/TilesIcon.tsx";
import { VolumeControl } from "../components/VolumeControl.tsx";
import {
  CANVAS_DIMENSIONS,
  DEFAULT_DESKTOP_SCALE,
  DEFAULT_MOBILE_SCALE,
  DEFAULT_VOLUME,
  MOBILE_BREAKPOINT,
  STORAGE_KEYS,
} from "../constants.ts";
import { useCycler } from "../hooks/useCycler.ts";
import { usePersistedState } from "../hooks/usePersistedState.ts";
import { useThemeDetection } from "../hooks/useThemeDetection.ts";
import { useVolume } from "../hooks/useVolume.ts";
import type { Scale } from "../types/canvas.ts";
import { VALID_SCALES } from "../types/canvas.ts";
import type { Theme } from "../types/theme.ts";
import { THEME_CYCLE, THEMES } from "../types/theme.ts";
import {
  booleanStorage,
  createTypedStorage,
  numberStorage,
} from "../utils/storage.ts";
import { getCurrentGame } from "../utils/gameLoader.ts";
import { useGameboyInitializer } from "../hooks/useGameboyInitializer.ts";

const scaleStorage = createTypedStorage<Scale>(
  (v) => Number(v) as Scale,
  String,
  (v) => VALID_SCALES.includes(v),
);

const themeStorage = createTypedStorage<Theme>(
  (v) => v as Theme,
  String,
  (v) => THEMES.includes(v),
);

export default function Canvas() {
  const { loadedGame } = useGameboyInitializer();

  const [scale, setScale] = usePersistedState(
    STORAGE_KEYS.scale,
    scaleStorage,
    typeof self !== "undefined" && self.innerWidth < MOBILE_BREAKPOINT
      ? DEFAULT_MOBILE_SCALE
      : DEFAULT_DESKTOP_SCALE,
  );

  const [volume, setVolume] = usePersistedState(
    STORAGE_KEYS.volume,
    numberStorage,
    DEFAULT_VOLUME,
  );

  const [theme, setTheme] = usePersistedState(
    STORAGE_KEYS.theme,
    themeStorage,
    "light",
  );

  const [showTiles, setShowTiles] = usePersistedState(
    STORAGE_KEYS.tiles,
    booleanStorage,
    false,
  );

  const canvasRef = useRef<HTMLCanvasElement>(null);
  const detectedTheme = useThemeDetection(theme, canvasRef);

  const cycleScale = useCycler(VALID_SCALES, scale, setScale);
  const cycleTheme = useCycler(THEMES, theme, (t) => setTheme(THEME_CYCLE[t]));
  const { volumeDown, volumeUp } = useVolume(volume, setVolume);

  useEffect(() => {
    self.audioVolumeControl?.setVolume(volume);
  }, [volume]);

  const isColorDark = (hexColor: string): boolean => {
    if (hexColor === "light") return false;
    if (hexColor === "dark") return true;

    const hex = hexColor.replace("#", "");
    const r = parseInt(hex.substring(0, 2), 16);
    const g = parseInt(hex.substring(2, 4), 16);
    const b = parseInt(hex.substring(4, 6), 16);
    const brightness = (r * 299 + g * 587 + b * 114) / 1000;

    return brightness < 128;
  };

  useEffect(() => {
    const effectiveTheme = theme === "auto" ? detectedTheme : theme;
    const isDark = typeof effectiveTheme === "string" && effectiveTheme.startsWith("#")
      ? isColorDark(effectiveTheme)
      : effectiveTheme === "dark";

    document.documentElement.classList.toggle("dark", isDark);

    if (theme === "auto" && typeof detectedTheme === "string" && detectedTheme.startsWith("#")) {
      document.documentElement.style.setProperty("--bg-color", detectedTheme);
    } else {
      document.documentElement.style.removeProperty("--bg-color");
    }
  }, [theme, detectedTheme]);

  const visibleWidth = showTiles
    ? CANVAS_DIMENSIONS.canvasWidth
    : CANVAS_DIMENSIONS.gameScreenWidth;

  const currentGame = loadedGame ??
    (typeof self !== "undefined" ? getCurrentGame() : null);

  return (
    <div class="flex flex-col items-center gap-3 sm:gap-6 px-2 sm:px-0">
      <div
        class="overflow-hidden rounded-sm max-w-full relative"
        style={{
          width: `${visibleWidth * scale}px`,
          height: `${CANVAS_DIMENSIONS.canvasHeight * scale}px`,
        }}
      >
        <canvas
          ref={canvasRef}
          id="canvas"
          tabIndex={-1}
          width={CANVAS_DIMENSIONS.canvasWidth}
          height={CANVAS_DIMENSIONS.canvasHeight}
          onContextMenu={(evt: Event) => evt.preventDefault()}
          style={{
            width: `${CANVAS_DIMENSIONS.canvasWidth * scale}px`,
            height: `${CANVAS_DIMENSIONS.canvasHeight * scale}px`,
            imageRendering: "pixelated",
            display: "block",
            outline: "none",
          }}
        />
      </div>

      <Dock isAutoMode={theme === "auto"}>
        {currentGame && (
          <>
            <GameSelector currentGame={currentGame} />
            <DockDivider />
          </>
        )}

        <ControlButton onClick={cycleScale} label="Change scale" variant="text">
          <span class="whitespace-nowrap">{scale}×</span>
        </ControlButton>

        <DockDivider />

        <VolumeControl
          volume={volume}
          onVolumeDown={volumeDown}
          onVolumeUp={volumeUp}
        />

        <DockDivider />

        <ThemeButton theme={theme} onCycle={cycleTheme} />

        <DockDivider />

        <ControlButton
          onClick={() => setShowTiles(!showTiles)}
          label={showTiles ? "Hide tiles" : "Show tiles"}
        >
          <TilesIcon class="w-4 h-4 sm:w-5 sm:h-5" />
        </ControlButton>

        <DockDivider />

        <a
          href="https://github.com/Jabolol/gameboy"
          target="_blank"
          rel="noopener noreferrer"
          class="p-1 sm:p-1.5 text-gray-900 dark:text-gray-100 hover:text-gray-600 dark:hover:text-gray-400 transition-colors outline-none focus:outline-none shrink-0"
          aria-label="View on GitHub"
        >
          <GitHubIcon class="w-4 h-4 sm:w-5 sm:h-5" />
        </a>
      </Dock>
    </div>
  );
}

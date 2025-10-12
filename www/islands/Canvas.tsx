import { useEffect, useRef } from "preact/hooks";
import { ControlButton } from "../components/ControlButton.tsx";
import { Dock, DockDivider } from "../components/Dock.tsx";
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

  useEffect(() => {
    const effectiveTheme = theme === "auto" ? detectedTheme : theme;
    document.documentElement.classList.toggle(
      "dark",
      effectiveTheme === "dark",
    );
  }, [theme, detectedTheme]);

  const visibleWidth = showTiles
    ? CANVAS_DIMENSIONS.canvasWidth
    : CANVAS_DIMENSIONS.gameScreenWidth;

  return (
    <div class="flex flex-col items-center gap-6">
      <div
        class="overflow-hidden rounded-sm"
        style={{
          width: `${visibleWidth * scale}px`,
          height: `${CANVAS_DIMENSIONS.canvasHeight * scale}px`,
        }}
      >
        <canvas
          ref={canvasRef}
          id="canvas"
          width={CANVAS_DIMENSIONS.canvasWidth}
          height={CANVAS_DIMENSIONS.canvasHeight}
          onContextMenu={(evt: Event) => evt.preventDefault()}
          style={{
            width: `${CANVAS_DIMENSIONS.canvasWidth * scale}px`,
            height: `${CANVAS_DIMENSIONS.canvasHeight * scale}px`,
            imageRendering: "pixelated",
            display: "block",
          }}
        />
      </div>

      <Dock>
        <ControlButton onClick={cycleScale} label="Change scale" variant="text">
          {scale}×
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
          <TilesIcon class="w-5 h-5" />
        </ControlButton>

        <DockDivider />

        <a
          href="https://github.com/Jabolol/gameboy"
          target="_blank"
          rel="noopener noreferrer"
          class="p-1.5 text-gray-900 dark:text-gray-100 hover:text-gray-600 dark:hover:text-gray-400 transition-colors outline-none focus:outline-none"
          aria-label="View on GitHub"
        >
          <GitHubIcon class="w-5 h-5" />
        </a>
      </Dock>
    </div>
  );
}

import {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from "preact/hooks";
import { Dock, DockDivider, Toast } from "../components/Dock.tsx";
import { GameSelector } from "../components/GameSelector.tsx";
import {
  bezelClass,
  BezelFooter,
  BezelHeader,
  Branding,
  CaseBackdrop,
  CaseHeader,
  HANDHELD_INSET,
} from "../components/Handheld.tsx";
import { type Settings, SettingsMenu } from "../components/SettingsMenu.tsx";
import { TouchControls } from "../components/TouchControls.tsx";
import { VolumeControl } from "../components/VolumeControl.tsx";
import {
  CANVAS_DIMENSIONS,
  DEFAULT_VOLUME,
  STORAGE_KEYS,
} from "../constants.ts";
import { useFittedScale } from "../hooks/useFittedScale.ts";
import { useGamepad } from "../hooks/useGamepad.ts";
import { useMediaQuery } from "../hooks/useMediaQuery.ts";
import { useSnapshots } from "../hooks/useSnapshots.ts";
import { usePersistedState } from "../hooks/usePersistedState.ts";
import { usePlayback } from "../hooks/usePlayback.ts";
import { useShortcuts } from "../hooks/useShortcuts.ts";
import { usePageTheme } from "../hooks/usePageTheme.ts";
import type { Scale } from "../types/canvas.ts";
import { VALID_SCALES } from "../types/canvas.ts";
import type { Button, Palette } from "../types/emulator.ts";
import { PALETTES } from "../types/emulator.ts";
import {
  CASE_IDS,
  type CaseId,
  DYNAMIC_CONTROLS,
  findCase,
  SURROUND,
} from "../types/handheld.ts";
import type { Theme } from "../types/theme.ts";
import { THEMES } from "../types/theme.ts";
import {
  booleanStorage,
  createTypedStorage,
  numberStorage,
} from "../utils/storage.ts";
import { getCurrentGame } from "../utils/gameLoader.ts";
import { useGameboyInitializer } from "../hooks/useGameboyInitializer.ts";

const scaleStorage = createTypedStorage<Scale>(
  (v) => (v === "fit" ? v : Number(v)) as Scale,
  String,
  (v) => VALID_SCALES.includes(v),
);

const themeStorage = createTypedStorage<Theme>(
  (v) => v as Theme,
  String,
  (v) => THEMES.includes(v),
);

const paletteStorage = createTypedStorage<Palette>(
  (v) => v as Palette,
  String,
  (v) => PALETTES.includes(v),
);

const handheldStorage = createTypedStorage<CaseId>(
  (v) => v as CaseId,
  String,
  (v) => CASE_IDS.includes(v),
);

const NOTICE_MS = 1800;
const NO_INSET = { width: 0, height: 0 };
const VOLUME_STEP = 0.1;

type Panel = "games" | "settings" | null;

export default function Canvas() {
  const {
    loadedGame,
    pendingGame,
    session,
    failure,
    switchGame,
    loadRomFile,
  } = useGameboyInitializer();

  const [scale, setScale] = usePersistedState(
    STORAGE_KEYS.scale,
    scaleStorage,
    "fit",
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

  const [palette, setPalette] = usePersistedState(
    STORAGE_KEYS.palette,
    paletteStorage,
    "gray",
  );

  const [colorCorrection, setColorCorrection] = usePersistedState(
    STORAGE_KEYS.colorCorrection,
    booleanStorage,
    false,
  );

  const [handheld, setHandheld] = usePersistedState(
    STORAGE_KEYS.handheld,
    handheldStorage,
    "classic",
  );

  const [turbo, setTurbo] = useState(false);
  const [panel, setPanel] = useState<Panel>(null);
  const [notice, setNotice] = useState({ text: "", visible: false });

  const canvasRef = useRef<HTMLCanvasElement>(null);
  const stageRef = useRef<HTMLDivElement>(null);
  const controlsRef = useRef<HTMLDivElement>(null);
  const touch = useMediaQuery("(pointer: coarse)");
  const preset = touch ? findCase(handheld) : null;
  const pageTheme = touch && !preset ? "auto" : theme;
  const pageColor = preset ? SURROUND : undefined;
  const { snapshots, saveSnapshot, restoreSnapshot, deleteSnapshot } =
    useSnapshots(session, canvasRef);

  useGamepad(session, setTurbo);
  usePlayback(session);
  usePageTheme(pageTheme, canvasRef, pageColor);

  useEffect(() => {
    session?.configure("palette", PALETTES.indexOf(palette));
  }, [session, palette]);

  useEffect(() => {
    session?.configure("colorCorrection", colorCorrection);
  }, [session, colorCorrection]);

  useEffect(() => {
    session?.configure("turbo", turbo);
  }, [session, turbo]);

  useEffect(() => {
    session?.configure("tiles", showTiles && !touch);
  }, [session, showTiles, touch]);

  useEffect(() => {
    if (!notice.visible) return;
    const timeout = setTimeout(
      () => setNotice((current) => ({ ...current, visible: false })),
      NOTICE_MS,
    );
    return () => clearTimeout(timeout);
  }, [notice]);

  const announce = (text: string) => setNotice({ text, visible: true });

  useEffect(() => {
    if (failure) announce(failure);
  }, [failure]);

  const changeVolume = (step: number) =>
    setVolume((current) =>
      Math.min(1, Math.max(0, Math.round((current + step) * 10) / 10))
    );

  useShortcuts({
    " ": setTurbo,
    p: (pressed) =>
      pressed &&
      setPalette((current) =>
        PALETTES[(PALETTES.indexOf(current) + 1) % PALETTES.length]
      ),
    c: (pressed) => pressed && setColorCorrection((current) => !current),
    u: (pressed) => pressed && changeVolume(VOLUME_STEP),
    d: (pressed) => pressed && changeVolume(-VOLUME_STEP),
  });

  const handleSaveSnapshot = async () => {
    try {
      await saveSnapshot();
      announce("Snapshot saved");
    } catch {
      announce("Snapshot could not be saved");
    }
  };

  const handleRestoreSnapshot = (id: string) => {
    restoreSnapshot(id);
    announce("Snapshot restored");
  };

  const settings: Settings = {
    turbo,
    scale,
    theme,
    palette,
    colorCorrection,
    showTiles,
    handheld,
  };

  const setters: { [K in keyof Settings]: (value: Settings[K]) => void } = {
    turbo: setTurbo,
    scale: setScale,
    theme: setTheme,
    palette: setPalette,
    colorCorrection: setColorCorrection,
    showTiles: setShowTiles,
    handheld: setHandheld,
  };

  const changeSetting = <K extends keyof Settings>(
    key: K,
    value: Settings[K],
  ) => setters[key](value);

  const openPanel = (name: Exclude<Panel, null>) => (open: boolean) =>
    setPanel((current) => open ? name : current === name ? null : current);

  const handleButton = useCallback((button: Button, pressed: boolean) => {
    session?.button(button, pressed);
  }, [session]);

  const handleDrop = (event: DragEvent) => {
    event.preventDefault();
    const file = event.dataTransfer?.files[0];
    if (file) loadRomFile(file);
  };

  useEffect(() => {
    self.audioVolumeControl?.setVolume(volume);
  }, [volume]);

  const panelWidth = useMemo(
    () =>
      session?.isColor() === false
        ? CANVAS_DIMENSIONS.panelWidth / 2
        : CANVAS_DIMENSIONS.panelWidth,
    [session],
  );
  const visibleWidth = CANVAS_DIMENSIONS.gameScreenWidth +
    (showTiles && !touch ? panelWidth : 0);
  const fittedScale = useFittedScale(
    scale,
    visibleWidth,
    CANVAS_DIMENSIONS.canvasHeight,
    stageRef,
    controlsRef,
    touch ? HANDHELD_INSET : NO_INSET,
  );

  const currentGame = loadedGame ??
    (typeof self !== "undefined" ? getCurrentGame() : null);

  const settingsMenu = (
    <SettingsMenu
      settings={settings}
      touch={touch}
      currentGame={currentGame}
      pendingGame={pendingGame}
      onGameChange={switchGame}
      onRomFile={loadRomFile}
      volume={volume}
      onVolumeChange={setVolume}
      onChange={changeSetting}
      snapshots={snapshots}
      onSaveSnapshot={handleSaveSnapshot}
      onRestoreSnapshot={handleRestoreSnapshot}
      onDeleteSnapshot={deleteSnapshot}
      open={panel === "settings"}
      onOpenChange={openPanel("settings")}
    />
  );

  return (
    <div
      ref={stageRef}
      class={touch
        ? `relative flex flex-col items-center w-full app-screen px-4 pt-[calc(env(safe-area-inset-top)+10px)] pb-[calc(env(safe-area-inset-bottom)+12px)] ${
          preset ? "" : "handheld-glass"
        }`
        : "flex flex-col items-center justify-center w-full app-screen px-3 pt-[max(1rem,env(safe-area-inset-top))] pb-[max(1rem,env(safe-area-inset-bottom))]"}
      style={preset ? { "--case": preset.body.join(" ") } : undefined}
      onDragOver={(event) => event.preventDefault()}
      onDrop={handleDrop}
    >
      {touch && <CaseBackdrop preset={preset} canvasRef={canvasRef} />}
      {touch && (
        <CaseHeader preset={preset}>
          <Toast notice={notice.text} visible={notice.visible} side="below" />
          {settingsMenu}
        </CaseHeader>
      )}
      <div
        class={touch
          ? "relative flex flex-col items-center w-full"
          : "flex items-center justify-center w-full min-h-0"}
      >
        <div class={touch ? bezelClass(preset) : "contents"}>
          {touch && <BezelHeader preset={preset} />}
          <div
            class={`overflow-hidden rounded-sm shrink-0 transition-opacity duration-300 ${
              pendingGame ? "opacity-50" : ""
            }`}
            style={{
              width: `${visibleWidth * fittedScale}px`,
              height: `${CANVAS_DIMENSIONS.canvasHeight * fittedScale}px`,
            }}
          >
            <canvas
              ref={canvasRef}
              id="canvas"
              role="img"
              aria-label="Game screen"
              tabIndex={-1}
              width={CANVAS_DIMENSIONS.canvasWidth}
              height={CANVAS_DIMENSIONS.canvasHeight}
              onContextMenu={(evt: Event) => evt.preventDefault()}
              style={{
                width: `${CANVAS_DIMENSIONS.canvasWidth * fittedScale}px`,
                height: `${CANVAS_DIMENSIONS.canvasHeight * fittedScale}px`,
                maxWidth: "none",
                imageRendering: "pixelated",
                display: "block",
                outline: "none",
              }}
            />
          </div>
          {touch && <BezelFooter preset={preset} />}
        </div>
        {touch && <Branding preset={preset} />}
      </div>

      <div
        ref={controlsRef}
        class={touch
          ? "relative flex flex-col items-center w-full shrink-0 my-auto pb-6"
          : "flex flex-col items-center gap-4 pt-5 shrink-0 max-w-full"}
      >
        <TouchControls
          onButton={handleButton}
          preset={preset ?? DYNAMIC_CONTROLS}
        />

        {!touch && (
          <Dock notice={notice.text} showNotice={notice.visible}>
            <GameSelector
              currentGame={currentGame}
              pendingGame={pendingGame}
              onGameChange={switchGame}
              onRomFile={loadRomFile}
              open={panel === "games"}
              onOpenChange={openPanel("games")}
            />
            <DockDivider />

            <VolumeControl volume={volume} onVolumeChange={setVolume} />

            <DockDivider />

            {settingsMenu}
          </Dock>
        )}
      </div>
    </div>
  );
}

import { useCallback, useEffect, useRef, useState } from "preact/hooks";
import { setupVolumeControl } from "../utils/audioSetup.ts";
import { installRom, isRomFile, sanitizeRomName } from "../utils/filesystem.ts";
import {
  fetchRom,
  formatGameName,
  getGameToLoad,
  isValidGame,
} from "../utils/gameLoader.ts";
import { GameboySession } from "../utils/session.ts";
import { useEmscriptenModule } from "./useEmscriptenModule.ts";

const nextFrame = () =>
  new Promise((resolve) => requestAnimationFrame(resolve));

export function useGameboyInitializer() {
  const [canvas, setCanvas] = useState<HTMLCanvasElement | null>(null);
  const [currentGame, setCurrentGame] = useState<string | null>(null);
  const [session, setSession] = useState<GameboySession | null>(null);
  const [pendingGame, setPendingGame] = useState<string | null>(null);
  const [failure, setFailure] = useState<string | null>(null);
  const sessionRef = useRef<GameboySession | null>(null);

  const { instance, error } = useEmscriptenModule(canvas);

  useEffect(() => {
    setupVolumeControl();
    setCanvas(document.getElementById("canvas") as HTMLCanvasElement | null);
  }, []);

  const loadGame = useCallback(async (name: string, data?: Uint8Array) => {
    if (!instance) return;
    if (!isRomFile(name)) {
      setFailure("Only .gb and .gbc files can be opened");
      return;
    }

    setPendingGame(name);
    setFailure(null);
    try {
      const rom = data ?? (isValidGame(name) ? await fetchRom(name) : null);
      if (!rom) return;

      if (sessionRef.current) {
        sessionRef.current.destroy();
        sessionRef.current = null;
        setSession(null);
        await nextFrame();
      }

      const paths = installRom(instance, name, rom);
      const next = GameboySession.create(instance, paths.rom, paths.save);
      if (!next) throw new Error(`${name} is not a valid ROM`);

      next.start();
      sessionRef.current = next;
      setSession(next);
      setCurrentGame(name);
    } catch (err) {
      console.error("Failed to load game:", err);
      setFailure(`${formatGameName(name)} could not be loaded`);
    } finally {
      setPendingGame(null);
    }
  }, [instance]);

  const loadRomFile = useCallback(async (file: File) => {
    await loadGame(
      sanitizeRomName(file.name),
      new Uint8Array(await file.arrayBuffer()),
    );
  }, [loadGame]);

  useEffect(() => {
    if (instance && !currentGame) loadGame(getGameToLoad());
  }, [instance, currentGame, loadGame]);

  const switchGame = useCallback((game: string) => {
    if (!pendingGame && game !== currentGame) loadGame(game);
  }, [currentGame, pendingGame, loadGame]);

  return {
    loadedGame: currentGame ?? undefined,
    pendingGame: pendingGame ?? undefined,
    session,
    failure: error ? "The emulator could not start" : failure,
    switchGame,
    loadRomFile,
  };
}

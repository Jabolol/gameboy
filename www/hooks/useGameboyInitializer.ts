import { useCallback, useEffect, useRef, useState } from "preact/hooks";
import { setupVolumeControl } from "../utils/audioSetup.ts";
import { getGameToLoad } from "../utils/gameLoader.ts";
import type { GameName } from "../utils/gameLoader.ts";
import { useEmscriptenModule } from "./useEmscriptenModule.ts";

const isUnwindError = (err: unknown) => err === "unwind";

const CLEANUP_DELAY_MS = 100;

export function useGameboyInitializer() {
  const [canvas, setCanvas] = useState<HTMLCanvasElement | null>(null);
  const [currentGame, setCurrentGame] = useState<GameName | null>(null);
  const [isInitializing, setIsInitializing] = useState(false);
  const instanceRef = useRef<number | null>(null);

  const { instance, loading, error } = useEmscriptenModule(canvas);

  useEffect(() => {
    setupVolumeControl();
    setCanvas(document.getElementById("canvas") as HTMLCanvasElement | null);
  }, []);

  const loadGame = useCallback((game: GameName) => {
    if (!instance) return false;

    try {
      const ptr = instance.ccall("gameboy_create", "number", ["string"], [
        `ROMs/${game}`,
      ]) as number;

      if (!ptr) return false;

      instanceRef.current = ptr;

      try {
        instance.ccall("gameboy_start", null, ["number"], [ptr]);
      } catch (err) {
        if (!isUnwindError(err)) throw err;
      }

      return true;
    } catch (err) {
      console.error("Failed to load game:", err);
      return false;
    }
  }, [instance]);

  const destroyInstance = useCallback(async () => {
    if (!instance || instanceRef.current === null) return;

    instance.ccall("gameboy_destroy", null, ["number"], [instanceRef.current]);
    instanceRef.current = null;
    await new Promise((resolve) => setTimeout(resolve, CLEANUP_DELAY_MS));
  }, [instance]);

  useEffect(() => {
    if (!instance || currentGame) return;

    setIsInitializing(true);
    const initialGame = getGameToLoad();

    if (loadGame(initialGame)) {
      setCurrentGame(initialGame);
    }

    setIsInitializing(false);
  }, [instance, currentGame, loadGame]);

  const switchGame = useCallback(async (newGame: GameName) => {
    if (!instance || isInitializing || newGame === currentGame) return;

    setIsInitializing(true);

    await destroyInstance();

    if (loadGame(newGame)) {
      setCurrentGame(newGame);
    }

    setIsInitializing(false);
  }, [instance, currentGame, isInitializing, destroyInstance, loadGame]);

  return {
    initialized: !!instance && !!currentGame,
    scriptLoaded: !!instance,
    loadedGame: currentGame ?? undefined,
    loading: loading || isInitializing,
    error,
    switchGame,
  };
}

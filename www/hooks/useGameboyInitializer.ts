import { useEffect, useMemo, useState } from "preact/hooks";
import { setupVolumeControl } from "../utils/audioSetup.ts";
import { getGameToLoad } from "../utils/gameLoader.ts";
import type { GameName } from "../utils/gameLoader.ts";
import { useEmscriptenModule } from "./useEmscriptenModule.ts";

export function useGameboyInitializer() {
  const [canvas, setCanvas] = useState<HTMLCanvasElement | null>(null);
  const [game, setGame] = useState<GameName | null>(null);

  useEffect(() => {
    setupVolumeControl();
    const canvasEl = document.getElementById("canvas") as
      | HTMLCanvasElement
      | null;
    const selectedGame = getGameToLoad();

    setCanvas(canvasEl);
    setGame(selectedGame);
  }, []);

  const config = useMemo(() => {
    if (!canvas || !game) return null;

    return {
      canvas,
      arguments: [`ROMs/${game}`],
      locateFile: (path: string) => `/${path}`,
    };
  }, [canvas, game]);

  const { instance, loading, error } = useEmscriptenModule(config);

  return {
    initialized: instance !== null,
    scriptLoaded: instance !== null,
    loadedGame: game ?? undefined,
    loading,
    error,
  };
}

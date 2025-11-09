import { useEffect } from "preact/hooks";
import { setupVolumeControl } from "../utils/audioSetup.ts";
import { initializeGameboyModule } from "../utils/gameLoader.ts";
import type { GameName } from "../utils/gameLoader.ts";
import { gameboyState, scriptManager } from "../utils/scriptManager.ts";

async function initializeGameboy(): Promise<GameName | null> {
  setupVolumeControl();
  const game = initializeGameboyModule();
  await new Promise((resolve) => setTimeout(resolve, 0));
  return game;
}

export function useGameboyInitializer() {
  useEffect(() => {
    if (gameboyState.value.initialized) return;

    const initialize = async () => {
      try {
        const loadedGame = await initializeGameboy();

        gameboyState.value = {
          ...gameboyState.value,
          initialized: true,
          loadedGame: loadedGame ?? undefined,
        };

        await scriptManager.load("/gameboy.js", { async: true });

        gameboyState.value = {
          ...gameboyState.value,
          scriptLoaded: true,
        };

        console.log("Gameboy initialized successfully");
      } catch (error) {
        gameboyState.value = {
          ...gameboyState.value,
          error: error instanceof Error ? error : new Error(String(error)),
        };
        console.error("Gameboy initialization failed:", error);
      }
    };

    initialize();
  }, []);

  return gameboyState.value;
}

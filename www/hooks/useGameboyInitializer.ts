import { useEffect } from "preact/hooks";
import { setupVolumeControl } from "../utils/audioSetup.ts";
import { initializeGameboyModule } from "../utils/gameLoader.ts";
import { gameboyState, scriptManager } from "../utils/scriptManager.ts";

async function initializeGameboy(): Promise<void> {
  setupVolumeControl();
  initializeGameboyModule();
  await new Promise((resolve) => setTimeout(resolve, 0));
}

export function useGameboyInitializer() {
  useEffect(() => {
    if (gameboyState.value.initialized) return;

    const initialize = async () => {
      try {
        await initializeGameboy();

        gameboyState.value = {
          ...gameboyState.value,
          initialized: true,
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

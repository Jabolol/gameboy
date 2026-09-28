import { useEffect, useState } from "preact/hooks";
import type { EmscriptenModule } from "../types/emulator.ts";
import { prepareFilesystem } from "../utils/filesystem.ts";

type EmscriptenFactory = (
  module?: Partial<EmscriptenModule>,
) => Promise<EmscriptenModule>;

interface UseEmscriptenModuleResult {
  instance: EmscriptenModule | null;
  error: Error | null;
}

export function useEmscriptenModule(
  canvas: HTMLCanvasElement | null,
): UseEmscriptenModuleResult {
  const [instance, setInstance] = useState<EmscriptenModule | null>(null);
  const [error, setError] = useState<Error | null>(null);

  useEffect(() => {
    if (!canvas || instance) return;

    let cancelled = false;

    (async () => {
      setError(null);

      try {
        const module = await import("../static/gameboy.js");
        if (cancelled) return;

        const moduleInstance = await (module.default as EmscriptenFactory)({
          canvas,
          locateFile: (path) => `/${path}`,
        });
        await prepareFilesystem(moduleInstance);

        if (!cancelled) {
          setInstance(moduleInstance);
        }
      } catch (err) {
        if (!cancelled) {
          const error = err instanceof Error ? err : new Error(String(err));
          setError(error);
          console.error("Failed to load Emscripten module:", error);
        }
      }
    })();

    return () => {
      cancelled = true;
    };
  }, [canvas, instance]);

  return { instance, error };
}

import { useEffect, useState } from "preact/hooks";

interface EmscriptenModule {
  canvas: HTMLCanvasElement | null;
  arguments: string[];
  locateFile?: (path: string, prefix: string) => string;
}

type EmscriptenFactory = (
  module?: Partial<EmscriptenModule>,
) => Promise<unknown>;

interface UseEmscriptenModuleResult {
  instance: unknown | null;
  loading: boolean;
  error: Error | null;
}

export function useEmscriptenModule(
  config: EmscriptenModule | null,
): UseEmscriptenModuleResult {
  const [instance, setInstance] = useState<unknown | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<Error | null>(null);

  useEffect(() => {
    if (!config) return;
    if (instance) return;

    let cancelled = false;

    const loadModule = async () => {
      setLoading(true);
      setError(null);

      try {
        const module = await import("../static/gameboy.js");
        const factory = module.default as EmscriptenFactory;

        if (cancelled) return;

        const moduleInstance = await factory(config);

        if (cancelled) return;

        setInstance(moduleInstance);
      } catch (err) {
        if (cancelled) return;

        const error = err instanceof Error ? err : new Error(String(err));
        setError(error);
        console.error("Failed to load Emscripten module:", error);
      } finally {
        if (!cancelled) {
          setLoading(false);
        }
      }
    };

    loadModule();

    return () => {
      cancelled = true;
    };
  }, [config, instance]);

  return { instance, loading, error };
}

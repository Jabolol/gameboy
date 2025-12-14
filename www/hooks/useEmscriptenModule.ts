import { useEffect, useState } from "preact/hooks";

interface EmscriptenModule {
  canvas: HTMLCanvasElement | null;
  locateFile?: (path: string, prefix: string) => string;
  ccall: (
    name: string,
    returnType: string | null,
    argTypes: string[],
    args: unknown[],
  ) => unknown;
}

type EmscriptenFactory = (
  module?: Partial<EmscriptenModule>,
) => Promise<EmscriptenModule>;

interface UseEmscriptenModuleResult {
  instance: EmscriptenModule | null;
  loading: boolean;
  error: Error | null;
}

export function useEmscriptenModule(
  canvas: HTMLCanvasElement | null,
): UseEmscriptenModuleResult {
  const [instance, setInstance] = useState<EmscriptenModule | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<Error | null>(null);

  useEffect(() => {
    if (!canvas || instance) return;

    let cancelled = false;

    (async () => {
      setLoading(true);
      setError(null);

      try {
        const module = await import("../static/gameboy.js");
        if (cancelled) return;

        const moduleInstance = await (module.default as EmscriptenFactory)({
          canvas,
          locateFile: (path) => `/${path}`,
        });

        if (!cancelled) {
          setInstance(moduleInstance);
        }
      } catch (err) {
        if (!cancelled) {
          const error = err instanceof Error ? err : new Error(String(err));
          setError(error);
          console.error("Failed to load Emscripten module:", error);
        }
      } finally {
        if (!cancelled) {
          setLoading(false);
        }
      }
    })();

    return () => {
      cancelled = true;
    };
  }, [canvas, instance]);

  return { instance, loading, error };
}

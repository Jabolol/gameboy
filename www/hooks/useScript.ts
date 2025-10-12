import { useEffect, useState } from "preact/hooks";

type ScriptStatus = "idle" | "loading" | "ready" | "error";

interface UseScriptOptions {
  async?: boolean;
  defer?: boolean;
  onLoad?: () => void;
  onError?: (error: Error) => void;
}

export function useScript(
  src: string | null,
  options: UseScriptOptions = {},
): ScriptStatus {
  const [status, setStatus] = useState<ScriptStatus>(
    src ? "loading" : "idle",
  );

  useEffect(() => {
    if (!src) {
      setStatus("idle");
      return;
    }

    const existingScript = document.querySelector<HTMLScriptElement>(
      `script[src="${src}"]`,
    );

    if (existingScript) {
      setStatus(
        existingScript.getAttribute("data-status") as ScriptStatus ?? "ready",
      );
      return;
    }

    const script = document.createElement("script");
    script.src = src;
    script.async = options.async ?? true;
    script.defer = options.defer ?? false;
    script.setAttribute("data-status", "loading");

    const handleLoad = () => {
      script.setAttribute("data-status", "ready");
      setStatus("ready");
      options.onLoad?.();
    };

    const handleError = () => {
      script.setAttribute("data-status", "error");
      const error = new Error(`Failed to load script: ${src}`);
      setStatus("error");
      options.onError?.(error);
    };

    script.addEventListener("load", handleLoad);
    script.addEventListener("error", handleError);

    document.body.appendChild(script);

    return () => {
      script.removeEventListener("load", handleLoad);
      script.removeEventListener("error", handleError);
    };
  }, [src, options.async, options.defer]);

  return status;
}

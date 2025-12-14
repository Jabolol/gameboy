import { signal } from "@preact/signals";
import type { GameName } from "./gameLoader.ts";

type ScriptStatus = "idle" | "loading" | "ready" | "error";

interface ScriptState {
  status: ScriptStatus;
  error?: Error;
}

class ScriptManager {
  private scripts = new Map<string, ScriptState>();
  private scriptElements = new Map<string, HTMLScriptElement>();

  getStatus(src: string): ScriptStatus {
    return this.scripts.get(src)?.status ?? "idle";
  }

  load(src: string, options: { async?: boolean } = {}): Promise<void> {
    const existing = this.scripts.get(src);

    if (existing?.status === "ready") return Promise.resolve();
    if (existing?.status === "loading") {
      return this.waitForScript(src);
    }

    this.scripts.set(src, { status: "loading" });

    const existingElement = document.querySelector<HTMLScriptElement>(
      `script[src="${src}"]`,
    );

    if (existingElement) {
      this.scriptElements.set(src, existingElement);
      this.scripts.set(src, { status: "ready" });
      return Promise.resolve();
    }

    return new Promise((resolve, reject) => {
      const script = document.createElement("script");
      script.src = src;
      script.async = options.async ?? true;

      const handleLoad = () => {
        this.scripts.set(src, { status: "ready" });
        resolve();
      };

      const handleError = () => {
        const error = new Error(`Failed to load script: ${src}`);
        this.scripts.set(src, { status: "error", error });
        reject(error);
      };

      script.addEventListener("load", handleLoad);
      script.addEventListener("error", handleError);

      this.scriptElements.set(src, script);
      document.body.appendChild(script);
    });
  }

  private waitForScript(src: string): Promise<void> {
    return new Promise((resolve, reject) => {
      const checkStatus = () => {
        const state = this.scripts.get(src);
        if (state?.status === "ready") {
          resolve();
        } else if (state?.status === "error") {
          reject(state.error);
        } else {
          setTimeout(checkStatus, 50);
        }
      };
      checkStatus();
    });
  }

  cleanup(src: string): void {
    const element = this.scriptElements.get(src);
    if (element?.parentNode) {
      element.parentNode.removeChild(element);
    }
    this.scripts.delete(src);
    this.scriptElements.delete(src);
  }
}

export const scriptManager = new ScriptManager();

export const gameboyState = signal<{
  initialized: boolean;
  scriptLoaded: boolean;
  loadedGame?: GameName;
  error?: Error;
}>({
  initialized: false,
  scriptLoaded: false,
});

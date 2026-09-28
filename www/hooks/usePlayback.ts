import { useEffect } from "preact/hooks";
import type { GameboySession } from "../utils/session.ts";

export function usePlayback(session: GameboySession | null) {
  useEffect(() => {
    if (!session) return;

    let wakeLock: Promise<WakeLockSentinel | null> | null = null;

    const release = () => {
      wakeLock?.then((sentinel) => sentinel?.release());
      wakeLock = null;
    };

    const update = () => {
      session.configure("paused", document.hidden);
      if (document.hidden) {
        release();
      } else if (!wakeLock && "wakeLock" in navigator) {
        wakeLock = navigator.wakeLock.request("screen").catch(() => null);
      }
    };

    update();
    document.addEventListener("visibilitychange", update);
    return () => {
      document.removeEventListener("visibilitychange", update);
      release();
    };
  }, [session]);
}

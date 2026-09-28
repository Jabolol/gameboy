import { useEffect, useRef } from "preact/hooks";

export type Shortcuts = Record<string, (pressed: boolean) => void>;

export function useShortcuts(shortcuts: Shortcuts) {
  const current = useRef(shortcuts);
  current.current = shortcuts;

  useEffect(() => {
    const onKey = (event: KeyboardEvent) => {
      if (event.repeat || event.metaKey || event.ctrlKey || event.altKey) {
        return;
      }

      const shortcut = current.current[event.key.toLowerCase()];
      if (!shortcut) return;

      event.preventDefault();
      shortcut(event.type === "keydown");
    };

    self.addEventListener("keydown", onKey);
    self.addEventListener("keyup", onKey);
    return () => {
      self.removeEventListener("keydown", onKey);
      self.removeEventListener("keyup", onKey);
    };
  }, []);
}

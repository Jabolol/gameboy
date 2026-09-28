import type { RefObject } from "preact";
import { useEffect } from "preact/hooks";

export function useDismiss(
  open: boolean,
  close: () => void,
  refs: RefObject<HTMLElement>[],
) {
  useEffect(() => {
    if (!open) return;

    const onPointerDown = (event: PointerEvent) => {
      const target = event.target as Node;
      if (!refs.some((ref) => ref.current?.contains(target))) close();
    };

    const onKeyDown = (event: KeyboardEvent) => {
      if (event.key === "Escape") close();
    };

    document.addEventListener("pointerdown", onPointerDown);
    document.addEventListener("keydown", onKeyDown, true);
    return () => {
      document.removeEventListener("pointerdown", onPointerDown);
      document.removeEventListener("keydown", onKeyDown, true);
    };
  }, [open, close]);
}

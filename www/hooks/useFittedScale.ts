import type { RefObject } from "preact";
import type { Scale } from "../types/canvas.ts";
import { type Size, useElementSize } from "./useElementSize.ts";

export function useFittedScale(
  scale: Scale,
  width: number,
  height: number,
  stageRef: RefObject<HTMLElement>,
  controlsRef: RefObject<HTMLElement>,
  inset: Size,
): number {
  const stage = useElementSize(stageRef);
  const controls = useElementSize(controlsRef);

  if (!stage || !controls) return scale === "fit" ? 1 : scale;

  const pixels = width * (self.devicePixelRatio || 1);
  const ratio = Math.min(
    (stage.width - inset.width) / width,
    (stage.height - controls.height - inset.height) / height,
  );
  const fitting = Math.floor(ratio * pixels) / pixels;
  return scale === "fit" ? fitting : Math.min(scale, fitting);
}

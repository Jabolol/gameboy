import { CANVAS_DIMENSIONS } from "../constants.ts";

export function drawFrame(
  source: HTMLCanvasElement,
  target: HTMLCanvasElement,
): CanvasRenderingContext2D {
  const context = target.getContext("2d", { willReadFrequently: true })!;
  const width = source.width * CANVAS_DIMENSIONS.gameScreenWidth /
    CANVAS_DIMENSIONS.canvasWidth;

  context.imageSmoothingEnabled = false;
  context.drawImage(
    source,
    0,
    0,
    width,
    source.height,
    0,
    0,
    target.width,
    target.height,
  );
  return context;
}

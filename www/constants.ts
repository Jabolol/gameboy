import type { CanvasDimensions } from "./types/canvas.ts";

export const CANVAS_DIMENSIONS: CanvasDimensions = {
  gameScreenWidth: 320,
  panelWidth: 256,
  canvasWidth: 576,
  canvasHeight: 288,
} as const;

export const DEFAULT_VOLUME = 0.7;

export const STORAGE_KEYS = {
  scale: "gb-scale",
  volume: "gb-volume",
  theme: "gb-theme",
  tiles: "gb-tiles",
  palette: "gb-palette",
  colorCorrection: "gb-color-correction",
  handheld: "gb-handheld",
} as const;

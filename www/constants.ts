import type { CanvasDimensions, Scale } from "./types/canvas.ts";

export const CANVAS_DIMENSIONS: CanvasDimensions = {
  gameScreenWidth: 320,
  canvasWidth: 448,
  canvasHeight: 256,
} as const;

export const DEFAULT_MOBILE_SCALE: Scale = 2;
export const DEFAULT_DESKTOP_SCALE: Scale = 3;
export const MOBILE_BREAKPOINT = 768;

export const DEFAULT_VOLUME = 0.7;
export const VOLUME_STEP = 0.1;
export const MIN_VOLUME = 0;
export const MAX_VOLUME = 1;

export const SAMPLING_CONFIG = {
  interval: 50,
  historySize: 3,
  consensusThreshold: 0.67,
  borderThickness: 40,
  brightnessThreshold: 127,
  borderDominanceThreshold: 1.5,
} as const;

export const STORAGE_KEYS = {
  scale: "gb-scale",
  volume: "gb-volume",
  theme: "gb-theme",
  tiles: "gb-tiles",
} as const;

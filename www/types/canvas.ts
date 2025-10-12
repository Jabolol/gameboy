export const VALID_SCALES = [1, 2, 3] as const;
export type Scale = typeof VALID_SCALES[number];

export interface CanvasDimensions {
  readonly gameScreenWidth: number;
  readonly canvasWidth: number;
  readonly canvasHeight: number;
}

export interface SamplingConfig {
  readonly width: number;
  readonly height: number;
  readonly borderThickness: number;
  readonly brightnessThreshold: number;
}

export interface PixelAnalysis {
  readonly borderLight: number;
  readonly borderDark: number;
  readonly innerLight: number;
  readonly innerDark: number;
}

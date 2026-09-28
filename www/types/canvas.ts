export const VALID_SCALES = ["fit", 1, 2, 3] as const;
export type Scale = typeof VALID_SCALES[number];

export interface CanvasDimensions {
  readonly gameScreenWidth: number;
  readonly panelWidth: number;
  readonly canvasWidth: number;
  readonly canvasHeight: number;
}

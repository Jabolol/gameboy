import type { PixelAnalysis, SamplingConfig } from "../types/canvas.ts";
import type { DetectedTheme } from "../types/theme.ts";

interface PixelCoord {
  x: number;
  y: number;
}

function isBorderPixel(
  coord: PixelCoord,
  width: number,
  height: number,
  borderThickness: number,
): boolean {
  return (
    coord.x < borderThickness ||
    coord.x >= width - borderThickness ||
    coord.y < borderThickness ||
    coord.y >= height - borderThickness
  );
}

function calculateBrightness(r: number, g: number, b: number): number {
  return (r + g + b) / 3;
}

export function analyzePixels(
  imageData: ImageData,
  config: SamplingConfig,
): PixelAnalysis {
  const { width, height, borderThickness, brightnessThreshold } = config;
  const pixels = imageData.data;

  let borderLight = 0;
  let borderDark = 0;
  let innerLight = 0;
  let innerDark = 0;

  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const i = (y * width + x) * 4;
      const brightness = calculateBrightness(
        pixels[i],
        pixels[i + 1],
        pixels[i + 2],
      );

      const isLight = brightness > brightnessThreshold;
      const isBorder = isBorderPixel({ x, y }, width, height, borderThickness);

      if (isBorder) {
        isLight ? borderLight++ : borderDark++;
      } else {
        isLight ? innerLight++ : innerDark++;
      }
    }
  }

  return { borderLight, borderDark, innerLight, innerDark };
}

export function detectThemeFromAnalysis(
  analysis: PixelAnalysis,
  borderDominanceThreshold: number,
): DetectedTheme {
  const { borderDark, borderLight, innerDark, innerLight } = analysis;

  if (borderDark > borderLight * borderDominanceThreshold) return "dark";
  if (borderLight > borderDark * borderDominanceThreshold) return "light";

  return innerLight > innerDark ? "light" : "dark";
}

import type { PixelAnalysis, SamplingConfig } from "../types/canvas.ts";
import type { DetectedTheme } from "../types/theme.ts";

interface PixelCoord {
  x: number;
  y: number;
}

interface ColorCount {
  r: number;
  g: number;
  b: number;
  count: number;
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

function quantizeColor(r: number, g: number, b: number, binSize = 32): string {
  const qr = Math.floor(r / binSize) * binSize;
  const qg = Math.floor(g / binSize) * binSize;
  const qb = Math.floor(b / binSize) * binSize;
  return `${qr},${qg},${qb}`;
}

function rgbToHex(r: number, g: number, b: number): string {
  return `#${[r, g, b].map(x => x.toString(16).padStart(2, '0')).join('')}`;
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

export function detectDominantColor(
  imageData: ImageData,
  config: SamplingConfig,
): string {
  const { width, height } = config;
  const pixels = imageData.data;
  const colorMap = new Map<string, ColorCount>();
  const borderPercentage = 0.5;
  const borderThicknessX = Math.floor(width * borderPercentage);
  const borderThicknessY = Math.floor(height * borderPercentage);

  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const isBorder =
        x < borderThicknessX ||
        x >= width - borderThicknessX ||
        y < borderThicknessY ||
        y >= height - borderThicknessY;

      if (isBorder) {
        const i = (y * width + x) * 4;
        const r = pixels[i];
        const g = pixels[i + 1];
        const b = pixels[i + 2];
        const a = pixels[i + 3];

        if (a < 128) continue;

        const colorKey = quantizeColor(r, g, b);

        const existing = colorMap.get(colorKey);
        if (existing) {
          existing.count++;
          existing.r += r;
          existing.g += g;
          existing.b += b;
        } else {
          colorMap.set(colorKey, { r, g, b, count: 1 });
        }
      }
    }
  }

  let maxCount = 0;
  let dominantColor: ColorCount = { r: 255, g: 255, b: 255, count: 0 };

  for (const color of colorMap.values()) {
    if (color.count > maxCount) {
      maxCount = color.count;
      dominantColor = color;
    }
  }

  const avgR = Math.round(dominantColor.r / dominantColor.count);
  const avgG = Math.round(dominantColor.g / dominantColor.count);
  const avgB = Math.round(dominantColor.b / dominantColor.count);

  return rgbToHex(avgR, avgG, avgB);
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

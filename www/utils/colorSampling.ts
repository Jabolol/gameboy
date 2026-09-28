export type Color = [number, number, number];

const BIN_SHIFT = 4;
const LIGHT_LABEL: Color = [245, 245, 247];
const DARK_LABEL: Color = [29, 29, 31];
const ACCENT_MIN_SATURATION = 12;
const ACCENT_FALLBACK_HUE = 345;

function luminance(color: Color): number {
  const [r, g, b] = color.map((channel) => {
    const value = channel / 255;
    return value <= 0.04045
      ? value / 12.92
      : Math.pow((value + 0.055) / 1.055, 2.4);
  });
  return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

function contrast(a: Color, b: Color): number {
  const [light, dark] = [luminance(a), luminance(b)].sort((x, y) => y - x);
  return (light + 0.05) / (dark + 0.05);
}

export function prefersLightText(background: Color): boolean {
  return contrast(background, LIGHT_LABEL) > contrast(background, DARK_LABEL);
}

function hueAndSaturation([r, g, b]: Color): [number, number] {
  const max = Math.max(r, g, b);
  const min = Math.min(r, g, b);
  const delta = max - min;
  if (!delta) return [0, 0];

  const hue = max === r
    ? ((g - b) / delta + 6) % 6
    : max === g
    ? (b - r) / delta + 2
    : (r - g) / delta + 4;
  const lightness = (max + min) / 510;
  const saturation = delta / 255 / (1 - Math.abs(2 * lightness - 1));
  return [hue * 60, saturation * 100];
}

export function contrastAccent(background: Color): [string, string] {
  const [hue, saturation] = hueAndSaturation(background);

  if (prefersLightText(background)) {
    return [`hsl(${hue} ${Math.min(saturation, 30)}% 93%)`, "rgb(0 0 0 / 0.3)"];
  }

  const accent = saturation < ACCENT_MIN_SATURATION
    ? ACCENT_FALLBACK_HUE
    : (hue + 180) % 360;
  return [`hsl(${accent} 68% 40%)`, "rgb(255 255 255 / 0.3)"];
}

export function edgeColor(image: ImageData, ring: number): Color {
  const { width, height, data } = image;
  const bins = new Map<number, [number, number, number, number]>();
  let dominant: [number, number, number, number] = [255, 255, 255, 1];

  for (let y = 0; y < height; y++) {
    const edgeRow = y < ring || y >= height - ring;
    for (let x = 0; x < width; x++) {
      if (!edgeRow && x >= ring && x < width - ring) continue;

      const i = (y * width + x) * 4;
      const [r, g, b] = [data[i], data[i + 1], data[i + 2]];
      const key = (r >> BIN_SHIFT) << 16 | (g >> BIN_SHIFT) << 8 |
        b >> BIN_SHIFT;
      const bin = bins.get(key) ?? [0, 0, 0, 0];

      bin[0] += r;
      bin[1] += g;
      bin[2] += b;
      bin[3]++;
      bins.set(key, bin);
      if (bin[3] > dominant[3]) dominant = bin;
    }
  }

  const [r, g, b, count] = dominant;
  return [
    Math.round(r / count),
    Math.round(g / count),
    Math.round(b / count),
  ];
}

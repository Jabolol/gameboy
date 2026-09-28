export interface GlassShape {
  width: number;
  height: number;
  radius: number;
  bezel: number;
  arm?: number;
}

export interface GlassMaps {
  displacement: string;
  specular: string;
  scale: number;
}

interface Rect {
  x: number;
  y: number;
  width: number;
  height: number;
  radius: number;
}

type Pixel = [number, number, number, number];
type Edge = [number, number, number];

const REFRACTIVE_INDEX = 1.5;
const THICKNESS = 0.6;
const BACKDROP_GAP = 0.6;
const SAMPLES = 128;
const LIGHT_ANGLE = (-60 * Math.PI) / 180;
const RIM_WIDTH = 1.5;
const NEUTRAL: Pixel = [128, 128, 128, 255];
const CLEAR: Pixel = [255, 255, 255, 0];

const cache = new Map<string, GlassMaps>();

function surface(x: number): number {
  return Math.pow(1 - Math.pow(1 - x, 4), 1 / 4);
}

function refractionProfile(bezel: number): number[] {
  const delta = 0.001;

  return Array.from({ length: SAMPLES }, (_, index) => {
    const x = Math.min(Math.max(index / (SAMPLES - 1), delta), 1 - delta);
    const slope = (surface(x + delta) - surface(x - delta)) / (2 * delta);
    const incidence = Math.atan(slope * THICKNESS);
    const refracted = Math.asin(Math.sin(incidence) / REFRACTIVE_INDEX);
    const depth = (surface(x) * THICKNESS + BACKDROP_GAP) * bezel;
    return depth * Math.tan(incidence - refracted);
  });
}

function rectEdge(x: number, y: number, rect: Rect): Edge {
  const { width, height, radius } = rect;
  const cx = x - rect.x - width / 2;
  const cy = y - rect.y - height / 2;
  const qx = Math.abs(cx) - (width / 2 - radius);
  const qy = Math.abs(cy) - (height / 2 - radius);
  const sx = Math.sign(cx) || 1;
  const sy = Math.sign(cy) || 1;

  if (qx > 0 && qy > 0) {
    const length = Math.hypot(qx, qy);
    return [radius - length, (qx / length) * sx, (qy / length) * sy];
  }

  return qx > qy ? [radius - qx, sx, 0] : [radius - qy, 0, sy];
}

function edge(x: number, y: number, shape: GlassShape): Edge {
  const { width, height, radius, arm } = shape;
  if (!arm) return rectEdge(x, y, { x: 0, y: 0, width, height, radius });

  const horizontal = rectEdge(x, y, {
    x: 0,
    y: (height - arm) / 2,
    width,
    height: arm,
    radius,
  });
  const vertical = rectEdge(x, y, {
    x: (width - arm) / 2,
    y: 0,
    width: arm,
    height,
    radius,
  });
  return horizontal[0] > vertical[0] ? horizontal : vertical;
}

function paint(
  width: number,
  height: number,
  density: number,
  pixel: (x: number, y: number) => Pixel,
): string {
  const canvas = document.createElement("canvas");
  canvas.width = Math.ceil(width * density);
  canvas.height = Math.ceil(height * density);

  const context = canvas.getContext("2d")!;
  const image = context.createImageData(canvas.width, canvas.height);

  for (let y = 0; y < canvas.height; y++) {
    for (let x = 0; x < canvas.width; x++) {
      image.data.set(
        pixel((x + 0.5) / density, (y + 0.5) / density),
        (y * canvas.width + x) * 4,
      );
    }
  }

  context.putImageData(image, 0, 0);
  return canvas.toDataURL();
}

export function crossPath(
  width: number,
  height: number,
  arm: number,
  radius: number,
): string {
  const x = (width - arm) / 2;
  const y = (height - arm) / 2;
  const r = `A ${radius} ${radius} 0 0 1`;

  return [
    `M ${x + radius} 0 H ${x + arm - radius} ${r} ${x + arm} ${radius}`,
    `V ${y} H ${width - radius} ${r} ${width} ${y + radius}`,
    `V ${y + arm - radius} ${r} ${width - radius} ${y + arm}`,
    `H ${x + arm} V ${height - radius} ${r} ${x + arm - radius} ${height}`,
    `H ${x + radius} ${r} ${x} ${height - radius} V ${y + arm}`,
    `H ${radius} ${r} 0 ${y + arm - radius} V ${
      y + radius
    } ${r} ${radius} ${y}`,
    `H ${x} V ${radius} ${r} ${x + radius} 0 Z`,
  ].join(" ");
}

export function createGlassMaps(input: GlassShape): GlassMaps {
  const thickness = input.arm ?? Math.min(input.width, input.height);
  const radius = Math.min(input.radius, thickness / 2);
  const bezel = Math.min(input.bezel, thickness / 2);
  const shape = { ...input, radius, bezel };
  const density = Math.min(self.devicePixelRatio || 1, 2);
  const key = [
    shape.width,
    shape.height,
    radius,
    bezel,
    shape.arm,
    density,
  ].join();

  const cached = cache.get(key);
  if (cached) return cached;

  const profile = refractionProfile(shape.bezel);
  const peak = Math.max(...profile.map(Math.abs)) || 1;
  const lightX = Math.cos(LIGHT_ANGLE);
  const lightY = Math.sin(LIGHT_ANGLE);

  const displacement = paint(shape.width, shape.height, 1, (x, y) => {
    const [distance, nx, ny] = edge(x, y, shape);
    if (distance <= 0 || distance >= shape.bezel) return NEUTRAL;

    const index = Math.round((distance / shape.bezel) * (SAMPLES - 1));
    const magnitude = (profile[index] / peak) * 127;
    return [128 - nx * magnitude, 128 - ny * magnitude, 128, 255];
  });

  const specular = paint(shape.width, shape.height, density, (x, y) => {
    const [distance, nx, ny] = edge(x, y, shape);
    if (distance <= 0 || distance >= shape.bezel) return CLEAR;

    const facing = Math.abs(nx * lightX + ny * lightY);
    const rim = Math.max(0, 1 - distance / RIM_WIDTH);
    const glow = Math.pow(1 - distance / shape.bezel, 3) * 0.35;
    const alpha = Math.min(1, (rim * 0.9 + glow) * (0.25 + facing * 0.75));
    return [255, 255, 255, Math.round(alpha * 255)];
  });

  const maps = { displacement, specular, scale: peak * 2 };
  cache.set(key, maps);
  return maps;
}

export function supportsRefraction(): boolean {
  return typeof navigator !== "undefined" && "userAgentData" in navigator;
}

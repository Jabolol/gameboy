export interface EmscriptenFS {
  mkdir(path: string): void;
  mount(
    type: unknown,
    options: Record<string, unknown>,
    mountpoint: string,
  ): void;
  syncfs(populate: boolean, callback: (error: unknown) => void): void;
  writeFile(path: string, data: Uint8Array | string): void;
  readFile(path: string): Uint8Array;
  readFile(path: string, options: { encoding: "utf8" }): string;
  unlink(path: string): void;
  stat(path: string): { mtime: Date };
  analyzePath(path: string): { exists: boolean };
  filesystems: Record<string, unknown>;
}

export interface EmscriptenModule {
  canvas: HTMLCanvasElement | null;
  locateFile?: (path: string, prefix: string) => string;
  ccall: (
    name: string,
    returnType: string | null,
    argTypes: string[],
    args: unknown[],
  ) => unknown;
  FS: EmscriptenFS;
}

export const BUTTONS = {
  a: 1,
  b: 2,
  select: 4,
  start: 8,
  right: 16,
  left: 32,
  up: 64,
  down: 128,
} as const;
export type Button = keyof typeof BUTTONS;

export const REQUESTS = { saveState: 1, loadState: 2 } as const;
export type Request = keyof typeof REQUESTS;

export const SETTINGS = {
  turbo: 0,
  palette: 1,
  colorCorrection: 2,
  tiles: 3,
  paused: 4,
} as const;
export type Setting = keyof typeof SETTINGS;

export const PALETTES = ["gray", "green"] as const;
export type Palette = typeof PALETTES[number];

import type { EmscriptenModule } from "../types/emulator.ts";
import { isValidGame } from "./gameLoader.ts";

const ROMS_DIRECTORY = "/roms";
const SAVES_DIRECTORY = "/saves";
const ROM_EXTENSION = /\.gbc?$/i;
const TITLE_START = 0x134;
const TITLE_END = 0x144;
const CHECKSUM = 0x14e;

function romIdentity(data: Uint8Array): string {
  const title = String.fromCharCode(...data.subarray(TITLE_START, TITLE_END))
    .replace(/[^\x20-\x7e]/g, "")
    .trim();
  const checksum = ((data[CHECKSUM] << 8) | data[CHECKSUM + 1])
    .toString(16)
    .padStart(4, "0");
  return `${sanitizeRomName(title) || "rom"}-${checksum}`;
}

export async function prepareFilesystem(module: EmscriptenModule) {
  const { FS } = module;

  FS.mkdir(ROMS_DIRECTORY);
  FS.mkdir(SAVES_DIRECTORY);
  FS.mount(FS.filesystems.IDBFS, { autoPersist: true }, SAVES_DIRECTORY);

  await new Promise<void>((resolve, reject) =>
    FS.syncfs(true, (error) => error ? reject(error) : resolve())
  );
}

export function installRom(
  module: EmscriptenModule,
  name: string,
  data: Uint8Array,
) {
  const rom = `${ROMS_DIRECTORY}/${name}`;
  const save = isValidGame(name) ? name : romIdentity(data);
  module.FS.writeFile(rom, data);
  return { rom, save: `${SAVES_DIRECTORY}/${save}` };
}

export function isRomFile(name: string): boolean {
  return ROM_EXTENSION.test(name);
}

export function sanitizeRomName(name: string): string {
  return name.toLowerCase().replace(/[^a-z0-9.]+/g, "-");
}

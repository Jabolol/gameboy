import {
  type Button,
  BUTTONS,
  type EmscriptenModule,
  type Request,
  REQUESTS,
  type Setting,
  SETTINGS,
} from "../types/emulator.ts";
import {
  activateSnapshot,
  deleteSnapshot,
  readSnapshots,
  type Snapshot,
  stateModifiedAt,
  storeSnapshot,
  waitForState,
} from "./snapshots.ts";

const isUnwindError = (err: unknown) => err === "unwind";

export class GameboySession {
  private constructor(
    private readonly module: EmscriptenModule,
    private readonly pointer: number,
    private readonly save: string,
  ) {}

  static create(
    module: EmscriptenModule,
    rom: string,
    save: string,
  ): GameboySession | null {
    const pointer = module.ccall("gameboy_create", "number", [
      "string",
      "string",
    ], [rom, save]) as number;
    return pointer ? new GameboySession(module, pointer, save) : null;
  }

  start(): void {
    try {
      this.call("gameboy_start");
    } catch (err) {
      if (!isUnwindError(err)) throw err;
    }
  }

  button(button: Button, pressed: boolean): void {
    this.call("gameboy_button", BUTTONS[button], Number(pressed));
  }

  request(request: Request): void {
    this.call("gameboy_request", REQUESTS[request]);
  }

  configure(setting: Setting, value: number | boolean): void {
    this.call("gameboy_configure", SETTINGS[setting], Number(value));
  }

  snapshots(): Snapshot[] {
    return readSnapshots(this.module, this.save);
  }

  async takeSnapshot(thumbnail: string): Promise<Snapshot[]> {
    const since = stateModifiedAt(this.module, this.save);
    this.request("saveState");
    await waitForState(this.module, this.save, since);
    return storeSnapshot(this.module, this.save, thumbnail);
  }

  restoreSnapshot(id: string): void {
    activateSnapshot(this.module, this.save, id);
    this.request("loadState");
  }

  deleteSnapshot(id: string): Snapshot[] {
    return deleteSnapshot(this.module, this.save, id);
  }

  rumble(): boolean {
    return Boolean(this.call("gameboy_rumble"));
  }

  isColor(): boolean {
    return Boolean(this.call("gameboy_color"));
  }

  destroy(): void {
    this.call("gameboy_destroy");
  }

  private call(name: string, ...args: number[]): unknown {
    return this.module.ccall(
      name,
      "number",
      ["number", ...args.map(() => "number")],
      [this.pointer, ...args],
    );
  }
}

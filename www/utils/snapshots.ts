import type { EmscriptenModule } from "../types/emulator.ts";

export interface Snapshot {
  id: string;
  time: number;
  thumbnail: string;
}

const SNAPSHOT_LIMIT = 12;
const WRITE_TIMEOUT_MS = 2000;

const statePath = (save: string, id?: string) =>
  id ? `${save}.${id}.state` : `${save}.state`;

const indexPath = (save: string) => `${save}.snapshots.json`;

function exists(module: EmscriptenModule, path: string): boolean {
  return module.FS.analyzePath(path).exists;
}

function writeSnapshots(
  module: EmscriptenModule,
  save: string,
  snapshots: Snapshot[],
): Snapshot[] {
  module.FS.writeFile(indexPath(save), JSON.stringify(snapshots));
  return snapshots;
}

export function readSnapshots(
  module: EmscriptenModule,
  save: string,
): Snapshot[] {
  if (!exists(module, indexPath(save))) return [];

  try {
    return JSON.parse(
      module.FS.readFile(indexPath(save), { encoding: "utf8" }),
    );
  } catch {
    return [];
  }
}

export function stateModifiedAt(
  module: EmscriptenModule,
  save: string,
): number {
  const path = statePath(save);
  return exists(module, path) ? module.FS.stat(path).mtime.getTime() : 0;
}

export async function waitForState(
  module: EmscriptenModule,
  save: string,
  since: number,
): Promise<void> {
  const deadline = performance.now() + WRITE_TIMEOUT_MS;

  while (stateModifiedAt(module, save) <= since) {
    if (performance.now() > deadline) {
      throw new Error("The emulator did not write the snapshot");
    }
    await new Promise((resolve) => requestAnimationFrame(resolve));
  }
}

export function storeSnapshot(
  module: EmscriptenModule,
  save: string,
  thumbnail: string,
): Snapshot[] {
  const time = Date.now();
  const id = time.toString(36);
  module.FS.writeFile(statePath(save, id), module.FS.readFile(statePath(save)));

  const snapshots = [{ id, time, thumbnail }, ...readSnapshots(module, save)];
  for (const { id } of snapshots.slice(SNAPSHOT_LIMIT)) {
    module.FS.unlink(statePath(save, id));
  }

  return writeSnapshots(module, save, snapshots.slice(0, SNAPSHOT_LIMIT));
}

export function activateSnapshot(
  module: EmscriptenModule,
  save: string,
  id: string,
): void {
  module.FS.writeFile(statePath(save), module.FS.readFile(statePath(save, id)));
}

export function deleteSnapshot(
  module: EmscriptenModule,
  save: string,
  id: string,
): Snapshot[] {
  module.FS.unlink(statePath(save, id));
  return writeSnapshots(
    module,
    save,
    readSnapshots(module, save).filter((snapshot) => snapshot.id !== id),
  );
}

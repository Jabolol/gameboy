import type { RefObject } from "preact";
import { useEffect, useState } from "preact/hooks";
import { drawFrame } from "../utils/frame.ts";
import type { GameboySession } from "../utils/session.ts";
import type { Snapshot } from "../utils/snapshots.ts";

const THUMBNAIL_WIDTH = 160;
const THUMBNAIL_HEIGHT = 144;

function captureThumbnail(canvas: HTMLCanvasElement): string {
  const thumbnail = document.createElement("canvas");
  thumbnail.width = THUMBNAIL_WIDTH;
  thumbnail.height = THUMBNAIL_HEIGHT;

  drawFrame(canvas, thumbnail);
  return thumbnail.toDataURL();
}

export function useSnapshots(
  session: GameboySession | null,
  canvasRef: RefObject<HTMLCanvasElement>,
) {
  const [snapshots, setSnapshots] = useState<Snapshot[]>([]);

  useEffect(() => {
    setSnapshots(session?.snapshots() ?? []);
  }, [session]);

  const saveSnapshot = async () => {
    if (!session || !canvasRef.current) return;
    setSnapshots(
      await session.takeSnapshot(captureThumbnail(canvasRef.current)),
    );
  };

  const restoreSnapshot = (id: string) => session?.restoreSnapshot(id);

  const deleteSnapshot = (id: string) => {
    if (session) setSnapshots(session.deleteSnapshot(id));
  };

  return { snapshots, saveSnapshot, restoreSnapshot, deleteSnapshot };
}

import { useState } from "preact/hooks";
import type { Snapshot } from "../utils/snapshots.ts";
import { CloseIcon } from "./icons/CloseIcon.tsx";

interface SnapshotListProps {
  snapshots: Snapshot[];
  onRestore: (id: string) => void;
  onDelete: (id: string) => void;
}

const TIME_FORMAT = new Intl.DateTimeFormat(undefined, {
  hour: "numeric",
  minute: "2-digit",
});

const DATE_FORMAT = new Intl.DateTimeFormat(undefined, {
  month: "short",
  day: "numeric",
  hour: "numeric",
  minute: "2-digit",
});

function formatTime(time: number): string {
  const date = new Date(time);
  return date.toDateString() === new Date().toDateString()
    ? TIME_FORMAT.format(date)
    : DATE_FORMAT.format(date);
}

export function SnapshotList(
  { snapshots, onRestore, onDelete }: SnapshotListProps,
) {
  const [previewId, setPreviewId] = useState<string | null>(null);
  const preview = snapshots.find(({ id }) => id === previewId) ??
    snapshots[0];

  if (!preview) {
    return (
      <p class="px-6 py-8 text-center text-[14px] text-[var(--label-secondary)]">
        No snapshots of this game yet. Save one to come back to this moment
        later.
      </p>
    );
  }

  return (
    <>
      <button
        type="button"
        class="block w-[calc(100%-1.5rem)] mx-3 mt-2 rounded-2xl overflow-hidden shadow-[0_0_0_0.5px_rgb(0_0_0/0.15),0_4px_14px_rgb(0_0_0/0.18)]"
        aria-label={`Load the snapshot from ${formatTime(preview.time)}`}
        onClick={() => onRestore(preview.id)}
      >
        <img
          src={preview.thumbnail}
          alt=""
          width={160}
          height={144}
          class="w-full h-auto [image-rendering:pixelated]"
        />
      </button>
      <p class="px-3 pt-1.5 text-center text-[12px] text-[var(--label-secondary)]">
        {formatTime(preview.time)}
      </p>
      <ul
        aria-label="Snapshots"
        class="grid grid-cols-3 gap-x-3 gap-y-2.5 px-3 pt-2 pb-3"
      >
        {snapshots.map(({ id, time, thumbnail }) => (
          <li
            key={id}
            data-previewed={id === preview.id || undefined}
            class="relative group"
            onPointerEnter={() => setPreviewId(id)}
            onFocusIn={() => setPreviewId(id)}
          >
            <button
              type="button"
              class="flex flex-col items-center gap-1 w-full rounded-xl"
              aria-label={`Load the snapshot from ${formatTime(time)}`}
              onClick={() => onRestore(id)}
            >
              <img
                src={thumbnail}
                alt=""
                width={160}
                height={144}
                class="w-full h-auto rounded-[10px] shadow-[0_0_0_0.5px_rgb(0_0_0/0.15),0_2px_6px_rgb(0_0_0/0.15)] [image-rendering:pixelated] outline-2 outline-offset-2 outline-transparent group-data-previewed:outline-[var(--label-secondary)]"
              />
              <span class="text-[12px] text-[var(--label-secondary)]">
                {formatTime(time)}
              </span>
            </button>
            <button
              type="button"
              class="absolute -top-1.5 -right-1.5 grid place-items-center w-5 h-5 rounded-full bg-black/70 text-white opacity-0 group-hover:opacity-100 focus-visible:opacity-100 pointer-coarse:opacity-100 transition-opacity"
              aria-label={`Delete the snapshot from ${formatTime(time)}`}
              onClick={() => onDelete(id)}
            >
              <CloseIcon class="w-2.5 h-2.5" />
            </button>
          </li>
        ))}
      </ul>
    </>
  );
}

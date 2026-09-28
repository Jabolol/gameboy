import { useRef } from "preact/hooks";
import { DEFAULT_VOLUME } from "../constants.ts";
import { DockButton } from "./Dock.tsx";
import { SpeakerIcon } from "./icons/SpeakerIcon.tsx";

interface VolumeControlProps {
  volume: number;
  onVolumeChange: (volume: number) => void;
  wide?: boolean;
}

export function VolumeControl(
  { volume, onVolumeChange, wide = false }: VolumeControlProps,
) {
  const restoreTo = useRef(volume || DEFAULT_VOLUME);
  const percent = Math.round(volume * 100);

  const toggleMute = () => {
    if (volume > 0) {
      restoreTo.current = volume;
      onVolumeChange(0);
    } else {
      onVolumeChange(restoreTo.current);
    }
  };

  return (
    <div class={`flex items-center gap-1 pr-2 ${wide ? "w-full" : ""}`}>
      <DockButton label={volume > 0 ? "Mute" : "Unmute"} onClick={toggleMute}>
        <SpeakerIcon
          level={volume === 0 ? 0 : volume < 0.5 ? 1 : 2}
          class="w-5 h-5"
        />
      </DockButton>
      <input
        type="range"
        min={0}
        max={100}
        step={5}
        value={percent}
        class={`slider ${wide ? "flex-1" : "w-16 sm:w-24"}`}
        style={{ "--value": volume }}
        aria-label="Volume"
        aria-valuetext={`${percent}%`}
        title={`Volume ${percent}%`}
        onPointerUp={(event) => (event.currentTarget as HTMLElement).blur()}
        onInput={(event) =>
          onVolumeChange(
            Number((event.target as HTMLInputElement).value) / 100,
          )}
      />
    </div>
  );
}

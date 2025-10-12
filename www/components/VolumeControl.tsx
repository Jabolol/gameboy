import { ControlButton } from "./ControlButton.tsx";
import { MinusIcon } from "./icons/MinusIcon.tsx";
import { PlusIcon } from "./icons/PlusIcon.tsx";

interface VolumeControlProps {
  volume: number;
  onVolumeDown: () => void;
  onVolumeUp: () => void;
}

export function VolumeControl({
  volume,
  onVolumeDown,
  onVolumeUp,
}: VolumeControlProps) {
  return (
    <>
      <ControlButton onClick={onVolumeDown} label="Volume down">
        <MinusIcon class="w-5 h-5" />
      </ControlButton>

      <span class="px-2 text-sm font-medium text-gray-900 dark:text-gray-100 w-[3.5rem] text-center">
        {Math.round(volume * 100)}%
      </span>

      <ControlButton onClick={onVolumeUp} label="Volume up">
        <PlusIcon class="w-5 h-5" />
      </ControlButton>
    </>
  );
}

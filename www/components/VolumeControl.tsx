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
        <MinusIcon class="w-4 h-4 sm:w-5 sm:h-5" />
      </ControlButton>

      <span class="px-1 sm:px-2 text-xs sm:text-sm font-medium text-gray-900 dark:text-gray-100 w-10 sm:w-14 text-center shrink-0">
        {Math.round(volume * 100)}%
      </span>

      <ControlButton onClick={onVolumeUp} label="Volume up">
        <PlusIcon class="w-4 h-4 sm:w-5 sm:h-5" />
      </ControlButton>
    </>
  );
}

import { useCallback } from "preact/hooks";
import { MAX_VOLUME, MIN_VOLUME, VOLUME_STEP } from "../constants.ts";

export function useVolume(
  volume: number,
  setVolume: (volume: number) => void,
) {
  const volumeDown = useCallback(() => {
    setVolume(Math.max(MIN_VOLUME, volume - VOLUME_STEP));
  }, [volume, setVolume]);

  const volumeUp = useCallback(() => {
    setVolume(Math.min(MAX_VOLUME, volume + VOLUME_STEP));
  }, [volume, setVolume]);

  return { volumeDown, volumeUp };
}

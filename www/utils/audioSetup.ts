interface AudioVolumeControl {
  gainNode: GainNode;
  setVolume: (volume: number) => void;
}

declare global {
  var audioVolumeControl: AudioVolumeControl | undefined;
  var webkitAudioContext: typeof AudioContext | undefined;
}

const DEFAULT_VOLUME = 0.7;
const VOLUME_STORAGE_KEY = "gb-volume";

function getSavedVolume(): number {
  const savedVolume = localStorage.getItem(VOLUME_STORAGE_KEY);
  return savedVolume ? parseFloat(savedVolume) : DEFAULT_VOLUME;
}

function createAudioContextProxy(
  OriginalAudioContext: typeof AudioContext,
): typeof AudioContext {
  return new Proxy(OriginalAudioContext, {
    construct(
      target,
      args: ConstructorParameters<typeof AudioContext>,
    ) {
      const audioContext = Reflect.construct(
        target,
        args,
      );
      const volume = getSavedVolume();

      const gainNode = audioContext.createGain();
      gainNode.gain.value = volume;
      const realDestination = audioContext.destination;
      gainNode.connect(realDestination);

      self.audioVolumeControl = {
        gainNode,
        setVolume: (v: number) => {
          gainNode.gain.value = v;
        },
      };

      Object.defineProperty(audioContext, "destination", {
        get: () => gainNode,
        configurable: true,
      });

      return audioContext;
    },
  });
}

export function setupVolumeControl(): void {
  if (typeof self === "undefined") return;

  const OriginalAudioContext = self.AudioContext ?? self.webkitAudioContext;
  if (!OriginalAudioContext) return;

  const ProxiedAudioContext = createAudioContextProxy(OriginalAudioContext);

  self.AudioContext = ProxiedAudioContext;
  if (self.webkitAudioContext) {
    self.webkitAudioContext = ProxiedAudioContext;
  }
}

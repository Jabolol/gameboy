import { DEFAULT_VOLUME, STORAGE_KEYS } from "../constants.ts";
import { numberStorage } from "./storage.ts";

interface AudioVolumeControl {
  gainNode: GainNode;
  setVolume: (volume: number) => void;
}

interface AudioSession {
  type: "auto" | "playback" | "ambient" | "transient" | "play-and-record";
}

declare global {
  var audioVolumeControl: AudioVolumeControl | undefined;
  var webkitAudioContext: typeof AudioContext | undefined;

  interface Navigator {
    audioSession?: AudioSession;
  }
}

const UNLOCK_EVENTS = ["pointerup", "touchend", "keydown", "click"];

function resumeOnInteraction(audioContext: AudioContext) {
  const resume = () => {
    audioContext.resume().then(() => {
      if (audioContext.state !== "running") return;
      for (const type of UNLOCK_EVENTS) {
        self.removeEventListener(type, resume, true);
      }
    });
  };

  const arm = () => {
    if (audioContext.state === "running") return;
    for (const type of UNLOCK_EVENTS) {
      self.addEventListener(type, resume, true);
    }
  };

  arm();
  audioContext.addEventListener("statechange", arm);
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
      const volume = numberStorage.get(STORAGE_KEYS.volume, DEFAULT_VOLUME);

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

      resumeOnInteraction(audioContext);
      return audioContext;
    },
  });
}

export function setupVolumeControl(): void {
  if (typeof self === "undefined") return;

  if (navigator.audioSession) navigator.audioSession.type = "playback";

  const OriginalAudioContext = self.AudioContext ?? self.webkitAudioContext;
  if (!OriginalAudioContext) return;

  const ProxiedAudioContext = createAudioContextProxy(OriginalAudioContext);

  self.AudioContext = ProxiedAudioContext;
  if (self.webkitAudioContext) {
    self.webkitAudioContext = ProxiedAudioContext;
  }
}

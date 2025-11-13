import { type RefObject } from "preact";
import { useEffect, useRef, useState } from "preact/hooks";
import { SAMPLING_CONFIG } from "../constants.ts";
import type { SamplingConfig } from "../types/canvas.ts";
import type { DetectedTheme, Theme } from "../types/theme.ts";
import { detectDominantColor } from "../utils/colorSampling.ts";
import { ConsensusTracker } from "../utils/consensus.ts";

export function useThemeDetection(
  theme: Theme,
  canvasRef: RefObject<HTMLCanvasElement>,
): DetectedTheme {
  const [detectedTheme, setDetectedTheme] = useState<DetectedTheme>("#ffffff");
  const consensusRef = useRef(
    new ConsensusTracker<DetectedTheme>(
      SAMPLING_CONFIG.historySize,
      SAMPLING_CONFIG.consensusThreshold,
    ),
  );

  useEffect(() => {
    if (theme !== "auto") return;

    let animationFrameId: number;
    let lastSampleTime = 0;

    const samplingConfig: SamplingConfig = {
      width: 160,
      height: 144,
      borderThickness: SAMPLING_CONFIG.borderThickness,
      brightnessThreshold: SAMPLING_CONFIG.brightnessThreshold,
    };

    const sampleColors = () => {
      const canvas = canvasRef.current;
      const ctx = canvas?.getContext("2d", { willReadFrequently: true });

      if (!canvas || !ctx) {
        animationFrameId = requestAnimationFrame(sampleColors);
        return;
      }

      const now = performance.now();
      const shouldSample = now - lastSampleTime >= SAMPLING_CONFIG.interval;

      if (!shouldSample) {
        animationFrameId = requestAnimationFrame(sampleColors);
        return;
      }

      lastSampleTime = now;

      const imageData = ctx.getImageData(
        0,
        0,
        samplingConfig.width,
        samplingConfig.height,
      );
      const detected = detectDominantColor(imageData, samplingConfig);

      consensusRef.current.add(detected);

      if (consensusRef.current.isReady()) {
        const newTheme = consensusRef.current.getConsensus(detectedTheme);
        if (newTheme !== detectedTheme) {
          setDetectedTheme(newTheme);
        }
      }

      animationFrameId = requestAnimationFrame(sampleColors);
    };

    animationFrameId = requestAnimationFrame(sampleColors);

    return () => {
      cancelAnimationFrame(animationFrameId);
    };
  }, [theme, canvasRef, detectedTheme]);

  return detectedTheme;
}

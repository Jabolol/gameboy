import type { RefObject } from "preact";
import { useEffect } from "preact/hooks";
import type { Theme } from "../types/theme.ts";
import {
  type Color,
  contrastAccent,
  edgeColor,
  prefersLightText,
} from "../utils/colorSampling.ts";
import { drawFrame } from "../utils/frame.ts";

const FRAME_WIDTH = 160;
const FRAME_HEIGHT = 144;
const EDGE_RING = 4;

function paintPage(dark: boolean, background?: Color) {
  const root = document.documentElement;

  root.classList.toggle("dark", dark);
  if (background) {
    const [accent, label] = contrastAccent(background);
    root.style.setProperty("--bg-color", background.join(" "));
    root.style.setProperty("--accent", accent);
    root.style.setProperty("--accent-label", label);
  } else {
    root.style.removeProperty("--bg-color");
    root.style.removeProperty("--accent");
    root.style.removeProperty("--accent-label");
  }

  const color = getComputedStyle(root).getPropertyValue("--bg-color").trim();
  document.querySelector('meta[name="theme-color"]')?.setAttribute(
    "content",
    `rgb(${color})`,
  );
}

export function usePageTheme(
  theme: Theme,
  canvasRef: RefObject<HTMLCanvasElement>,
  fixed?: Color,
) {
  useEffect(() => {
    if (fixed) {
      paintPage(prefersLightText(fixed), fixed);
      return;
    }
    if (theme !== "auto") {
      paintPage(theme === "dark");
      return;
    }

    const frame = document.createElement("canvas");
    frame.width = FRAME_WIDTH;
    frame.height = FRAME_HEIGHT;

    let painted = "";
    let request = 0;

    const sample = () => {
      request = requestAnimationFrame(sample);

      const canvas = canvasRef.current;
      if (!canvas?.width || !canvas.height) return;

      const image = drawFrame(canvas, frame).getImageData(
        0,
        0,
        FRAME_WIDTH,
        FRAME_HEIGHT,
      );
      const color = edgeColor(image, EDGE_RING);
      const key = color.join();

      if (key !== painted) {
        painted = key;
        paintPage(prefersLightText(color), color);
      }
    };

    request = requestAnimationFrame(sample);
    return () => cancelAnimationFrame(request);
  }, [theme, canvasRef, fixed]);
}

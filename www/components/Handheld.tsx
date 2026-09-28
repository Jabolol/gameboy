import type { ComponentChildren, RefObject } from "preact";
import { useEffect, useRef } from "preact/hooks";
import type { CasePreset } from "../types/handheld.ts";
import { drawFrame } from "../utils/frame.ts";
import { Glass } from "./Glass.tsx";

interface CaseProps {
  preset: CasePreset | null;
}

interface CaseBackdropProps extends CaseProps {
  canvasRef: RefObject<HTMLCanvasElement>;
}

interface CaseHeaderProps extends CaseProps {
  children: ComponentChildren;
}

interface AmbientProps {
  canvasRef: RefObject<HTMLCanvasElement>;
}

export const HANDHELD_INSET = { width: 72, height: 150 };

const AMBIENT_WIDTH = 32;
const AMBIENT_HEIGHT = 29;
const SPEAKER_SLOTS = 6;
const GLASS_RADIUS = 44;
const COLOR_LETTERS = [
  { letter: "C", color: "#e5484d", tilt: -10, small: false },
  { letter: "o", color: "#9b5fc0", tilt: 8, small: true },
  { letter: "L", color: "#5bb74a", tilt: -6, small: false },
  { letter: "o", color: "#f2c318", tilt: 10, small: true },
  { letter: "R", color: "#3d86d6", tilt: -4, small: false },
];

function Ambient({ canvasRef }: AmbientProps) {
  const ref = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    let request = 0;

    const draw = () => {
      request = requestAnimationFrame(draw);
      const source = canvasRef.current;
      if (source?.width && source.height && ref.current) {
        drawFrame(source, ref.current);
      }
    };

    request = requestAnimationFrame(draw);
    return () => cancelAnimationFrame(request);
  }, [canvasRef]);

  return (
    <canvas
      ref={ref}
      width={AMBIENT_WIDTH}
      height={AMBIENT_HEIGHT}
      class="ambient"
    />
  );
}

function Speaker({ preset }: CaseProps) {
  if (preset?.model !== "dmg") return <span class="speaker-grille" />;

  return (
    <div class="speaker-slots">
      {Array.from(
        { length: SPEAKER_SLOTS },
        (_, index) => <span key={index} class="speaker-slot" />,
      )}
    </div>
  );
}

export function CaseBackdrop({ preset, canvasRef }: CaseBackdropProps) {
  if (!preset) {
    return (
      <div class="case-surround" aria-hidden="true">
        <div class="case case-glass">
          <Ambient canvasRef={canvasRef} />
          <span class="case-tint" />
          <Glass
            radius={GLASS_RADIUS}
            bezel={30}
            blur={18}
            class="absolute inset-0"
          >
            <span />
          </Glass>
          <Speaker preset={preset} />
        </div>
      </div>
    );
  }

  return (
    <div class="case-surround" aria-hidden="true">
      <div
        class={`case case-plastic case-${preset.model} ${
          preset.translucent ? "case-clear" : ""
        }`}
      >
        {preset.model === "dmg" && <span class="case-groove" />}
        <Speaker preset={preset} />
      </div>
    </div>
  );
}

export function CaseHeader({ preset, children }: CaseHeaderProps) {
  return (
    <div class="relative z-20 flex items-center justify-between w-full h-9 px-1 mb-2 shrink-0">
      {preset?.model === "dmg"
        ? <span class="power-switch">◁OFF•ON▷</span>
        : <span />}
      {children}
    </div>
  );
}

export function bezelClass(preset: CasePreset | null): string {
  if (!preset) return "bezel bezel-glass";
  return preset.model === "dmg" ? "bezel bezel-dmg" : "bezel bezel-gbc";
}

export function BezelHeader({ preset }: CaseProps) {
  if (preset?.model !== "dmg") {
    return (
      <span class="power-light">
        <span class="flex items-center gap-[2px]">
          <span class="led" />
          <span class="led-wave" />
          <span class="led-wave" />
          <span class="led-wave" />
        </span>
        POWER
      </span>
    );
  }

  return (
    <>
      <div class="bezel-title">
        <span class="bezel-stripes flex-1" />
        <span>DOT MATRIX WITH STEREO SOUND</span>
        <span class="bezel-stripes w-4" />
      </div>
      <span class="battery-light">
        <span class="led" />
        BATTERY
      </span>
    </>
  );
}

export function BezelFooter({ preset }: CaseProps) {
  if (preset?.model === "dmg") return null;

  return (
    <p class="bezel-logo">
      <span class="logo-wordmark text-[#dfe2e8]">GAME BOY</span>
      <span class="logo-color">
        {COLOR_LETTERS.map(({ letter, color, tilt, small }, index) => (
          <span
            key={index}
            class={small ? "text-[12px]" : "text-[16px]"}
            style={{ color, transform: `rotate(${tilt}deg)` }}
          >
            {letter}
          </span>
        ))}
      </span>
    </p>
  );
}

export function Branding({ preset }: CaseProps) {
  if (preset?.model !== "dmg") return null;

  return (
    <p class="brand-dmg logo-wordmark" style={{ color: preset.ink }}>
      GAME BOY<span class="text-[8px] not-italic ml-0.5">TM</span>
    </p>
  );
}

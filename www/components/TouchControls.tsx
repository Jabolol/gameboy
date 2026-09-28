import type { ComponentChildren } from "preact";
import { useRef, useState } from "preact/hooks";
import type { Button } from "../types/emulator.ts";
import type { CasePreset } from "../types/handheld.ts";
import { crossPath } from "../utils/glass.ts";
import { TriangleIcon } from "./icons/TriangleIcon.tsx";

interface TouchControlsProps {
  onButton: (button: Button, pressed: boolean) => void;
  preset: CasePreset;
}

interface PadButtonProps extends TouchControlsProps {
  button: Button;
  label: string;
  width: number;
  height: number;
  color: string;
  engraved?: boolean;
}

interface WellProps {
  preset: CasePreset;
  class?: string;
  children: ComponentChildren;
}

interface PlasticCrossProps {
  color: string;
}

interface CaptionProps {
  preset: CasePreset;
  children: ComponentChildren;
}

type Direction = "right" | "down" | "left" | "up";

const DPAD_SIZE = 120;
const DPAD_ARM = 40;
const DPAD_RADIUS = 8;
const TILT_DEGREES = 8;
const DEAD_ZONE = 0.2;
const HAPTIC_MS = 8;

const DIRECTIONS: Direction[] = ["right", "down", "left", "up"];

const TILTS: Record<Direction, string> = {
  right: `rotateY(${TILT_DEGREES}deg)`,
  down: `rotateX(-${TILT_DEGREES}deg)`,
  left: `rotateY(-${TILT_DEGREES}deg)`,
  up: `rotateX(${TILT_DEGREES}deg)`,
};

const SECTORS: Direction[][] = [
  ["right"],
  ["right", "down"],
  ["down"],
  ["down", "left"],
  ["left"],
  ["left", "up"],
  ["up"],
  ["up", "right"],
];

function directionsAt(event: PointerEvent): Direction[] {
  const rect = (event.currentTarget as HTMLElement).getBoundingClientRect();
  const x = event.clientX - rect.left - rect.width / 2;
  const y = event.clientY - rect.top - rect.height / 2;

  if (Math.hypot(x, y) < (rect.width / 2) * DEAD_ZONE) return [];

  const sector = Math.round(Math.atan2(y, x) / (Math.PI / 4));
  return SECTORS[(sector + 8) % 8];
}

function PlasticCross({ color }: PlasticCrossProps) {
  const outline = crossPath(DPAD_SIZE, DPAD_SIZE, DPAD_ARM, DPAD_RADIUS);

  return (
    <svg
      class="absolute inset-0 overflow-visible"
      width={DPAD_SIZE}
      height={DPAD_SIZE}
    >
      <defs>
        <linearGradient id="dpad-sheen" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0" stop-color="#fff" stop-opacity="0.16" />
          <stop offset="1" stop-color="#000" stop-opacity="0.22" />
        </linearGradient>
      </defs>
      <path d={outline} style={{ fill: color }} class="plastic-cross" />
      <path d={outline} fill="url(#dpad-sheen)" />
    </svg>
  );
}

function DirectionalPad({ onButton, preset }: TouchControlsProps) {
  const [pressed, setPressed] = useState<Direction[]>([]);
  const current = useRef<Direction[]>([]);

  const press = (next: Direction[]) => {
    if (next.join() === current.current.join()) return;

    for (const direction of DIRECTIONS) {
      const down = next.includes(direction);
      if (down !== current.current.includes(direction)) {
        onButton(direction, down);
      }
    }
    if (next.length) navigator.vibrate?.(HAPTIC_MS);

    current.current = next;
    setPressed(next);
  };

  const release = () => press([]);

  const face = (
    <div
      class="relative"
      style={{ width: `${DPAD_SIZE}px`, height: `${DPAD_SIZE}px` }}
    >
      {DIRECTIONS.map((direction, index) => (
        <div
          key={direction}
          data-pressed={pressed.includes(direction) || undefined}
          class="group absolute inset-0"
          style={{ transform: `rotate(${index * 90}deg)` }}
        >
          <span
            class="absolute right-0 inset-y-0 my-auto bg-[var(--pad-press)] opacity-0 transition-opacity duration-100 group-data-pressed:opacity-100"
            style={{
              width: `${(DPAD_SIZE - DPAD_ARM) / 2}px`,
              height: `${DPAD_ARM}px`,
              borderRadius: `0 ${DPAD_RADIUS}px ${DPAD_RADIUS}px 0`,
            }}
          />
          <TriangleIcon class="absolute right-3.5 inset-y-0 my-auto w-3 h-3 text-[var(--pad-mark)] transition-colors duration-100 group-data-pressed:text-[var(--pad-mark-active)]" />
        </div>
      ))}
      <span class="absolute inset-0 m-auto w-6 h-6 rounded-full bg-[var(--pad-dimple)] shadow-[inset_0_1px_2px_rgb(0_0_0/0.25)]" />
    </div>
  );

  return (
    <div
      role="group"
      aria-label="Directional pad"
      class="relative shrink-0 touch-none select-none transition-transform duration-75 ease-out"
      style={{
        width: `${DPAD_SIZE}px`,
        height: `${DPAD_SIZE}px`,
        transform: `perspective(360px) ${
          pressed.map((direction) => TILTS[direction]).join(" ")
        }`,
      }}
      onPointerDown={(event) => {
        event.preventDefault();
        (event.currentTarget as HTMLElement).setPointerCapture(event.pointerId);
        press(directionsAt(event));
      }}
      onPointerMove={(event) => {
        const target = event.currentTarget as HTMLElement;
        if (target.hasPointerCapture(event.pointerId)) {
          press(directionsAt(event));
        }
      }}
      onPointerUp={release}
      onPointerCancel={release}
      onLostPointerCapture={release}
      onContextMenu={(event) => event.preventDefault()}
    >
      <PlasticCross color={preset.dpad} />
      {face}
    </div>
  );
}

function Well({ preset, class: classes = "", children }: WellProps) {
  if (preset.model !== "gbc") return <>{children}</>;

  return (
    <div class={`control-well grid place-items-center ${classes}`}>
      {children}
    </div>
  );
}

function Caption({ preset, children }: CaptionProps) {
  return (
    <span
      class={`select-none ${
        preset.model === "dmg"
          ? "text-[11px] font-extrabold italic tracking-[0.12em]"
          : "text-[9px] font-bold tracking-[0.14em] opacity-70"
      }`}
      style={{ color: preset.ink }}
    >
      {children}
    </span>
  );
}

function PadButton(
  {
    button,
    label,
    width,
    height,
    color,
    engraved = false,
    onButton,
  }: PadButtonProps,
) {
  const [pressed, setPressed] = useState(false);
  const current = useRef(false);

  const press = (down: boolean) => (event: PointerEvent) => {
    event.preventDefault();
    if (down === current.current) return;
    if (down) {
      (event.currentTarget as HTMLElement).setPointerCapture(event.pointerId);
      navigator.vibrate?.(HAPTIC_MS);
    }

    onButton(button, down);
    current.current = down;
    setPressed(down);
  };

  return (
    <button
      type="button"
      aria-label={label}
      data-pressed={pressed || undefined}
      class="shrink-0 rounded-full touch-none select-none"
      onPointerDown={press(true)}
      onPointerUp={press(false)}
      onPointerCancel={press(false)}
      onLostPointerCapture={press(false)}
      onContextMenu={(event) => event.preventDefault()}
    >
      <span
        class="plastic-key grid place-items-center rounded-full"
        style={{
          width: `${width}px`,
          height: `${height}px`,
          backgroundColor: color,
        }}
      >
        {engraved && (
          <span class="text-[19px] font-bold text-[var(--key-label)]">
            {label}
          </span>
        )}
      </span>
    </button>
  );
}

export function TouchControls({ onButton, preset }: TouchControlsProps) {
  const dmg = preset.model === "dmg";
  const face = {
    onButton,
    preset,
    color: preset.buttons,
    engraved: !dmg,
    width: 54,
    height: 54,
  };
  const rubber = {
    onButton,
    preset,
    color: preset.rubber,
    width: dmg ? 46 : 40,
    height: 13,
  };

  return (
    <div
      class={`pad-plastic hidden pointer-coarse:flex flex-col items-center gap-4 w-full max-w-sm px-2 ${
        dmg ? "pad-dmg" : ""
      }`}
    >
      <div class="flex w-full items-center justify-between">
        <Well preset={preset} class="w-[150px] h-[150px] -ml-2 rounded-full">
          <DirectionalPad onButton={onButton} preset={preset} />
        </Well>
        <div class={`flex items-start ${dmg ? "gap-5" : "gap-4"} pr-1`}>
          <div
            class={`flex flex-col items-center gap-1.5 ${
              dmg ? "mt-5" : "mt-9"
            }`}
          >
            <Well preset={preset} class="p-1.5 rounded-full">
              <PadButton button="b" label="B" {...face} />
            </Well>
            {dmg && <Caption preset={preset}>B</Caption>}
          </div>
          <div class="flex flex-col items-center gap-1.5">
            <Well preset={preset} class="p-1.5 rounded-full">
              <PadButton button="a" label="A" {...face} />
            </Well>
            {dmg && <Caption preset={preset}>A</Caption>}
          </div>
        </div>
      </div>
      {dmg
        ? (
          <div class="flex gap-2 mt-1 mr-6">
            <div class="flex flex-col items-center gap-1.5 -rotate-[25deg]">
              <PadButton button="select" label="Select" {...rubber} />
              <Caption preset={preset}>SELECT</Caption>
            </div>
            <div class="flex flex-col items-center gap-1.5 mt-4 -rotate-[25deg]">
              <PadButton button="start" label="Start" {...rubber} />
              <Caption preset={preset}>START</Caption>
            </div>
          </div>
        )
        : (
          <div class="flex flex-col items-center gap-1.5">
            <div class="flex gap-6">
              <PadButton button="select" label="Select" {...rubber} />
              <PadButton button="start" label="Start" {...rubber} />
            </div>
            <div class="flex gap-6">
              <Caption preset={preset}>SELECT</Caption>
              <Caption preset={preset}>START</Caption>
            </div>
          </div>
        )}
    </div>
  );
}

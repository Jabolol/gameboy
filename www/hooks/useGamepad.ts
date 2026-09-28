import { useEffect } from "preact/hooks";
import type { Button } from "../types/emulator.ts";
import type { GameboySession } from "../utils/session.ts";

const GAMEPAD_BUTTONS: [number, Button][] = [
  [0, "b"],
  [1, "a"],
  [8, "select"],
  [9, "start"],
  [12, "up"],
  [13, "down"],
  [14, "left"],
  [15, "right"],
];

const TURBO_BUTTON = 5;
const AXIS_THRESHOLD = 0.5;
const RUMBLE_MS = 100;

function readButtons(pad: Gamepad): Record<Button, boolean> {
  const state = Object.fromEntries(
    GAMEPAD_BUTTONS.map(([index, button]) => [
      button,
      pad.buttons[index]?.pressed ?? false,
    ]),
  ) as Record<Button, boolean>;
  const [x = 0, y = 0] = pad.axes;

  state.left ||= x < -AXIS_THRESHOLD;
  state.right ||= x > AXIS_THRESHOLD;
  state.up ||= y < -AXIS_THRESHOLD;
  state.down ||= y > AXIS_THRESHOLD;
  return state;
}

function vibrate(pad: Gamepad | undefined) {
  if (pad?.vibrationActuator) {
    pad.vibrationActuator.playEffect("dual-rumble", {
      duration: RUMBLE_MS,
      strongMagnitude: 1,
      weakMagnitude: 1,
    });
  } else {
    navigator.vibrate?.(RUMBLE_MS);
  }
}

export function useGamepad(
  session: GameboySession | null,
  onTurbo: (turbo: boolean) => void,
) {
  useEffect(() => {
    if (!session) return;

    const previous: Partial<Record<Button, boolean>> = {};
    let turbo = false;
    let rumbleUntil = 0;
    let frame = 0;

    const poll = (time: number) => {
      const pad = navigator.getGamepads().find((p) => p?.connected) ??
        undefined;

      if (pad) {
        const state = readButtons(pad);
        for (const [, button] of GAMEPAD_BUTTONS) {
          if (state[button] !== (previous[button] ?? false)) {
            session.button(button, state[button]);
            previous[button] = state[button];
          }
        }
        const turboPressed = pad.buttons[TURBO_BUTTON]?.pressed ?? false;
        if (turboPressed !== turbo) {
          turbo = turboPressed;
          onTurbo(turbo);
        }
      }
      if (session.rumble() && time >= rumbleUntil) {
        vibrate(pad);
        rumbleUntil = time + RUMBLE_MS;
      }
      frame = requestAnimationFrame(poll);
    };

    frame = requestAnimationFrame(poll);
    return () => cancelAnimationFrame(frame);
  }, [session, onTurbo]);
}

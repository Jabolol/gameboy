import type { Color } from "../utils/colorSampling.ts";

export type Model = "dmg" | "gbc";

export type CaseId =
  | "classic"
  | "berry"
  | "grape"
  | "kiwi"
  | "dandelion"
  | "teal"
  | "atomic"
  | "dynamic";

export interface CasePreset {
  id: CaseId;
  name: string;
  model: Model;
  body: Color;
  ink: string;
  buttons: string;
  dpad: string;
  rubber: string;
  translucent?: boolean;
}

export const SURROUND: Color = [17, 17, 19];

export const CASES: readonly CasePreset[] = [
  {
    id: "classic",
    name: "Classic",
    model: "dmg",
    body: [188, 184, 182],
    ink: "#303a93",
    buttons: "#862a58",
    dpad: "#27272a",
    rubber: "#787377",
  },
  {
    id: "berry",
    name: "Berry",
    model: "gbc",
    body: [214, 36, 95],
    ink: "#5e0c2b",
    buttons: "#313335",
    dpad: "#313335",
    rubber: "#3b3b40",
  },
  {
    id: "grape",
    name: "Grape",
    model: "gbc",
    body: [92, 46, 140],
    ink: "#1f0d3a",
    buttons: "#313335",
    dpad: "#313335",
    rubber: "#3b3b40",
  },
  {
    id: "kiwi",
    name: "Kiwi",
    model: "gbc",
    body: [170, 211, 63],
    ink: "#3e5510",
    buttons: "#313335",
    dpad: "#313335",
    rubber: "#3b3b40",
  },
  {
    id: "dandelion",
    name: "Dandelion",
    model: "gbc",
    body: [246, 207, 29],
    ink: "#6b5200",
    buttons: "#313335",
    dpad: "#313335",
    rubber: "#3b3b40",
  },
  {
    id: "teal",
    name: "Teal",
    model: "gbc",
    body: [30, 159, 196],
    ink: "#0a4252",
    buttons: "#313335",
    dpad: "#313335",
    rubber: "#3b3b40",
  },
  {
    id: "atomic",
    name: "Atomic Purple",
    model: "gbc",
    body: [111, 85, 168],
    ink: "#1f0d3a",
    buttons: "#3a2a5c",
    dpad: "#3a2a5c",
    rubber: "#6d6480",
    translucent: true,
  },
];

export const DYNAMIC_CONTROLS: CasePreset = {
  id: "dynamic",
  name: "Dynamic",
  model: "gbc",
  body: [0, 0, 0],
  ink: "var(--accent)",
  buttons: "var(--accent)",
  dpad: "var(--accent)",
  rubber: "var(--accent)",
};

export const CASE_IDS: readonly CaseId[] = [
  ...CASES.map(({ id }) => id),
  "dynamic",
];

export function findCase(id: CaseId): CasePreset | null {
  return CASES.find((preset) => preset.id === id) ?? null;
}

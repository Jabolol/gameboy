export const THEMES = ["light", "dark", "auto"] as const;
export type Theme = typeof THEMES[number];

export type DetectedTheme = "light" | "dark" | string;

export type ThemeState =
  | { mode: "light" }
  | { mode: "dark" }
  | { mode: "auto"; detected: DetectedTheme };

export const THEME_CYCLE = {
  light: "dark",
  dark: "auto",
  auto: "light",
} as const satisfies Record<Theme, Theme>;

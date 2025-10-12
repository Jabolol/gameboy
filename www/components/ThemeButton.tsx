import { JSX, SVGAttributes } from "preact";
import type { Theme } from "../types/theme.ts";
import { ControlButton } from "./ControlButton.tsx";
import { AutoIcon } from "./icons/AutoIcon.tsx";
import { MoonIcon } from "./icons/MoonIcon.tsx";
import { SunIcon } from "./icons/SunIcon.tsx";

interface ThemeButtonProps {
  theme: Theme;
  onCycle: () => void;
}

const THEME_ICONS: Record<
  Theme,
  (props: SVGAttributes<SVGSVGElement>) => JSX.Element
> = {
  light: SunIcon,
  dark: MoonIcon,
  auto: AutoIcon,
};

export function ThemeButton({ theme, onCycle }: ThemeButtonProps) {
  const Icon = THEME_ICONS[theme];

  return (
    <ControlButton onClick={onCycle} label={`Theme: ${theme}`}>
      <Icon class="w-5 h-5" />
    </ControlButton>
  );
}

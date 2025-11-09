import type { ComponentChildren } from "preact";

interface DockProps {
  children: ComponentChildren;
}

const DOCK_CLASSES =
  "flex gap-1 sm:gap-2 items-center px-2 sm:px-4 py-1.5 sm:py-2 bg-white/80 dark:bg-black/80 backdrop-blur-xl border border-white/20 dark:border-gray-700/50 rounded-xl shadow-2xl shadow-black/10 max-w-full overflow-x-auto";

const DIVIDER_CLASSES = "w-px h-3 sm:h-4 bg-gray-300 dark:bg-gray-700 flex-shrink-0";

export function Dock({ children }: DockProps) {
  return <div class={DOCK_CLASSES}>{children}</div>;
}

export function DockDivider() {
  return <div class={DIVIDER_CLASSES} />;
}

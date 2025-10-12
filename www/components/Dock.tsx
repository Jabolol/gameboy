import type { ComponentChildren } from "preact";

interface DockProps {
  children: ComponentChildren;
}

const DOCK_CLASSES =
  "flex gap-2 items-center px-4 py-2 bg-white/80 dark:bg-black/80 backdrop-blur-xl border border-white/20 dark:border-gray-700/50 rounded-xl shadow-2xl shadow-black/10";

const DIVIDER_CLASSES = "w-px h-4 bg-gray-300 dark:bg-gray-700";

export function Dock({ children }: DockProps) {
  return <div class={DOCK_CLASSES}>{children}</div>;
}

export function DockDivider() {
  return <div class={DIVIDER_CLASSES} />;
}

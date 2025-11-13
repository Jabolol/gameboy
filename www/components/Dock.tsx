import type { ComponentChildren } from "preact";
import type { CSSProperties } from "preact";

interface DockProps {
  children: ComponentChildren;
  isAutoMode?: boolean;
}

const DOCK_BASE_CLASSES =
  "flex gap-1 sm:gap-2 items-center px-2 sm:px-4 py-1.5 sm:py-2 rounded-xl max-w-full overflow-x-auto";

const DOCK_NORMAL_CLASSES =
  "bg-white/80 dark:bg-black/80 backdrop-blur-xl border border-white/20 dark:border-gray-700/50 shadow-2xl shadow-black/10";

const DIVIDER_CLASSES =
  "w-px h-3 sm:h-4 bg-gray-300 dark:bg-gray-700 flex-shrink-0";

const LIQUID_GLASS_STYLE: CSSProperties = {
  position: "relative",
  background: "rgba(255, 255, 255, 0.15)",
  backdropFilter: "blur(2px) saturate(180%)",
  WebkitBackdropFilter: "blur(2px) saturate(180%)",
  border: "1px solid rgba(255, 255, 255, 0.8)",
  boxShadow:
    "0 8px 32px rgba(31, 38, 135, 0.2), inset 0 4px 20px rgba(255, 255, 255, 0.3)",
};

const LIQUID_SHINE_STYLE: CSSProperties = {
  position: "absolute",
  top: 0,
  left: 0,
  width: "100%",
  height: "100%",
  background: "rgba(255, 255, 255, 0.1)",
  borderRadius: "inherit",
  backdropFilter: "blur(1px)",
  WebkitBackdropFilter: "blur(1px)",
  boxShadow:
    "inset -10px -8px 0px -11px rgba(255, 255, 255, 1), inset 0px -9px 0px -8px rgba(255, 255, 255, 1)",
  opacity: 0.6,
  zIndex: -1,
  filter:
    "blur(1px) drop-shadow(10px 4px 6px rgba(0, 0, 0, 0.5)) brightness(115%)",
  pointerEvents: "none",
};

const CONTENT_WRAPPER_STYLE: CSSProperties = {
  position: "relative",
  zIndex: 1,
  display: "flex",
  gap: "inherit",
  alignItems: "center",
  width: "100%",
};

export function Dock({ children, isAutoMode = false }: DockProps) {
  if (isAutoMode) {
    return (
      <div class={DOCK_BASE_CLASSES} style={LIQUID_GLASS_STYLE}>
        <div style={LIQUID_SHINE_STYLE} />
        <div style={CONTENT_WRAPPER_STYLE}>{children}</div>
      </div>
    );
  }

  return (
    <div class={`${DOCK_BASE_CLASSES} ${DOCK_NORMAL_CLASSES}`}>{children}</div>
  );
}

export function DockDivider() {
  return <div class={DIVIDER_CLASSES} />;
}

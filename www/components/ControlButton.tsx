import type { ComponentChildren } from "preact";

interface ControlButtonProps {
  onClick: () => void;
  label: string;
  children: ComponentChildren;
  variant?: "icon" | "text";
}

const BUTTON_CLASSES =
  "p-1.5 text-gray-900 dark:text-gray-100 hover:text-gray-600 dark:hover:text-gray-400 transition-colors outline-none focus:outline-none";

const TEXT_VARIANT_CLASSES =
  "px-3 py-1.5 text-sm font-medium text-gray-900 dark:text-gray-100 hover:text-gray-600 dark:hover:text-gray-400 transition-colors outline-none focus:outline-none";

export function ControlButton({
  onClick,
  label,
  children,
  variant = "icon",
}: ControlButtonProps) {
  return (
    <button
      type="button"
      onClick={onClick}
      class={variant === "text" ? TEXT_VARIANT_CLASSES : BUTTON_CLASSES}
      aria-label={label}
    >
      {children}
    </button>
  );
}

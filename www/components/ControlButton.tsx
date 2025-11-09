import type { ComponentChildren } from "preact";

interface ControlButtonProps {
  onClick: () => void;
  label: string;
  children: ComponentChildren;
  variant?: "icon" | "text";
}

const BUTTON_CLASSES =
  "p-1 sm:p-1.5 text-gray-900 dark:text-gray-100 hover:text-gray-600 dark:hover:text-gray-400 transition-colors outline-none focus:outline-none flex-shrink-0";

const TEXT_VARIANT_CLASSES =
  "px-2 sm:px-3 py-1 sm:py-1.5 text-xs sm:text-sm font-medium text-gray-900 dark:text-gray-100 hover:text-gray-600 dark:hover:text-gray-400 transition-colors outline-none focus:outline-none flex-shrink-0";

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

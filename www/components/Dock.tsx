import type { ComponentChildren } from "preact";
import { Glass } from "./Glass.tsx";

interface DockProps {
  children: ComponentChildren;
  notice: string;
  showNotice: boolean;
}

interface ToastProps {
  notice: string;
  visible: boolean;
  side?: "above" | "below";
}

interface DockButtonProps {
  label: string;
  onClick: () => void;
  children: ComponentChildren;
}

const DOCK_RADIUS = 20;

const stopKey = (event: KeyboardEvent) => {
  if ((event.target as Element).matches(":focus-visible")) {
    event.stopPropagation();
  }
};

export function Dock({ children, notice, showNotice }: DockProps) {
  return (
    <div
      class="relative shrink-0 max-w-full"
      onKeyDown={stopKey}
      onKeyUp={stopKey}
      onKeyPress={stopKey}
    >
      <Toast notice={notice} visible={showNotice} />
      <Glass radius={DOCK_RADIUS} bezel={18}>
        <div class="flex items-center gap-0.5 p-1.5">{children}</div>
      </Glass>
    </div>
  );
}

export function Toast({ notice, visible, side = "above" }: ToastProps) {
  return (
    <div class={`toast toast-${side}`} data-open={visible || undefined}>
      <Glass radius={18} bezel={12} blur={14}>
        <p role="status" class="px-4 py-2 text-[14px] font-medium">
          {notice}
        </p>
      </Glass>
    </div>
  );
}

export function DockButton(
  { label, onClick, children }: DockButtonProps,
) {
  return (
    <button
      type="button"
      class="grid place-items-center w-10 h-10 shrink-0 rounded-full hover:bg-[var(--fill)]"
      aria-label={label}
      title={label}
      onClick={onClick}
    >
      {children}
    </button>
  );
}

export function DockDivider() {
  return <div class="w-px h-5 mx-1 shrink-0 bg-[var(--separator)]" />;
}

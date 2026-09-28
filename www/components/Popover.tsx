import type { ComponentChildren, RefObject } from "preact";
import { Glass } from "./Glass.tsx";

interface PopoverProps {
  open: boolean;
  align: "start" | "end";
  side?: "above" | "below";
  panelRef: RefObject<HTMLDivElement>;
  class?: string;
  children: ComponentChildren;
}

export const POPOVER_RADIUS = 22;

export function Popover(
  { open, align, side = "above", panelRef, class: classes, children }:
    PopoverProps,
) {
  const vertical = side === "above" ? "bottom" : "top";

  return (
    <div
      ref={panelRef}
      class={`popover popover-${side} ${
        align === "start"
          ? `left-0 origin-${vertical}-left`
          : `right-0 origin-${vertical}-right`
      }`}
      data-open={open || undefined}
      inert={!open}
    >
      <Glass radius={POPOVER_RADIUS} bezel={20} blur={14} class={classes}>
        {children}
      </Glass>
    </div>
  );
}

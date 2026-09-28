import type { JSX, SVGAttributes } from "preact";

export function ResizeIcon(
  props: SVGAttributes<SVGSVGElement>,
): JSX.Element {
  return (
    <svg
      fill="none"
      stroke="currentColor"
      strokeWidth="2"
      viewBox="0 0 24 24"
      {...props}
    >
      <path
        strokeLinecap="round"
        strokeLinejoin="round"
        d="M4 9V4h5m11 5V4h-5M4 15v5h5m11-5v5h-5"
      />
    </svg>
  );
}

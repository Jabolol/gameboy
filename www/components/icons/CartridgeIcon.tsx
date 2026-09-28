import type { JSX, SVGAttributes } from "preact";

export function CartridgeIcon(
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
        d="M6 3h9l3 3v14a1 1 0 01-1 1H7a1 1 0 01-1-1V3zm3 4h6v5H9V7zm0 10h2m4 0h.01"
      />
    </svg>
  );
}

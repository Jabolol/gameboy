import type { JSX, SVGAttributes } from "preact";

export function LcdIcon(
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
        d="M5 4h14a2 2 0 012 2v9a2 2 0 01-2 2H5a2 2 0 01-2-2V6a2 2 0 012-2zm3 17h8m-4-4v4m-5-8l3-3 2 2 4-4"
      />
    </svg>
  );
}

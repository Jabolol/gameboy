import type { JSX, SVGAttributes } from "preact";

export function CheckIcon(
  props: SVGAttributes<SVGSVGElement>,
): JSX.Element {
  return (
    <svg
      fill="none"
      stroke="currentColor"
      strokeWidth="2.5"
      viewBox="0 0 24 24"
      {...props}
    >
      <path
        strokeLinecap="round"
        strokeLinejoin="round"
        d="M5 12.5l4.5 4.5L19 7.5"
      />
    </svg>
  );
}

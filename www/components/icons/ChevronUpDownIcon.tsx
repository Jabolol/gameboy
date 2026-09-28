import type { JSX, SVGAttributes } from "preact";

export function ChevronUpDownIcon(
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
        d="M8 9l4-4 4 4m-8 6l4 4 4-4"
      />
    </svg>
  );
}

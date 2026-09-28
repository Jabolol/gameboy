import type { JSX, SVGAttributes } from "preact";

export function ArrowUpRightIcon(
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
        d="M7 17L17 7M8 7h9v9"
      />
    </svg>
  );
}

import type { JSX, SVGAttributes } from "preact";

export function HandheldIcon(
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
        d="M7 2h10a2 2 0 012 2v16a2 2 0 01-2 2H7a2 2 0 01-2-2V4a2 2 0 012-2zm1 3h8v6H8V5zm1 10h2m-1-1v2m5 0h.01M17 14h.01"
      />
    </svg>
  );
}

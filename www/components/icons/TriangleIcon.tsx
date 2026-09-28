import type { JSX, SVGAttributes } from "preact";

export function TriangleIcon(
  props: SVGAttributes<SVGSVGElement>,
): JSX.Element {
  return (
    <svg
      fill="currentColor"
      stroke="currentColor"
      strokeWidth="3"
      viewBox="0 0 24 24"
      {...props}
    >
      <path strokeLinejoin="round" d="M8 5l10 7-10 7z" />
    </svg>
  );
}

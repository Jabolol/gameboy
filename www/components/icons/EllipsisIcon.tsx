import type { JSX, SVGAttributes } from "preact";

export function EllipsisIcon(
  props: SVGAttributes<SVGSVGElement>,
): JSX.Element {
  return (
    <svg
      fill="currentColor"
      viewBox="0 0 24 24"
      {...props}
    >
      <path d="M6 10.25a1.75 1.75 0 110 3.5 1.75 1.75 0 010-3.5zm6 0a1.75 1.75 0 110 3.5 1.75 1.75 0 010-3.5zm6 0a1.75 1.75 0 110 3.5 1.75 1.75 0 010-3.5z" />
    </svg>
  );
}

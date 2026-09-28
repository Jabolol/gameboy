import type { JSX, SVGAttributes } from "preact";

export function DynamicIcon(
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
        d="M15 11.25l1.5 1.5.75-.75V8.76l2.28-.61a3 3 0 10-3.68-3.68l-.61 2.28H12l-.75.75 1.5 1.5M15 11.25l-8.47 8.47c-.34.34-.8.53-1.28.53s-.94.19-1.28.53l-.97.97-.75-.75.97-.97c.34-.34.53-.8.53-1.28s.19-.94.53-1.28L12.75 9M15 11.25L12.75 9"
      />
    </svg>
  );
}

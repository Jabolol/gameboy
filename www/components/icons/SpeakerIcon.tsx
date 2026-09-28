import type { JSX, SVGAttributes } from "preact";

interface SpeakerIconProps extends SVGAttributes<SVGSVGElement> {
  level: 0 | 1 | 2;
}

const WAVES = {
  0: "M16 9.5l5 5m0-5l-5 5",
  1: "M15.5 9.5a3.5 3.5 0 010 5",
  2: "M15.5 9.5a3.5 3.5 0 010 5m3-8a7.5 7.5 0 010 11",
};

export function SpeakerIcon(
  { level, ...props }: SpeakerIconProps,
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
        d={`M11 5L6 9H3v6h3l5 4V5z${WAVES[level]}`}
      />
    </svg>
  );
}

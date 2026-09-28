import type { ComponentChildren } from "preact";
import { useEffect, useId, useRef, useState } from "preact/hooks";
import { useElementSize } from "../hooks/useElementSize.ts";
import {
  createGlassMaps,
  crossPath,
  supportsRefraction,
} from "../utils/glass.ts";

interface GlassProps {
  radius: number;
  bezel?: number;
  blur?: number;
  arm?: number;
  class?: string;
  children: ComponentChildren;
}

export function Glass(
  { radius, bezel = 16, blur = 1.5, arm, class: classes = "", children }:
    GlassProps,
) {
  const id = `glass-${useId().replace(/[^\w-]/g, "")}`;
  const ref = useRef<HTMLDivElement>(null);
  const size = useElementSize(ref);
  const [refraction, setRefraction] = useState(false);

  useEffect(() => setRefraction(supportsRefraction()), []);

  const maps = size?.width && size.height
    ? createGlassMaps({ ...size, radius, bezel, arm })
    : null;
  const refracting = refraction && size && maps;
  const outline = arm && size
    ? crossPath(size.width, size.height, arm, radius)
    : null;

  return (
    <div
      ref={ref}
      class={`glass ${outline ? "glass-outlined" : ""} ${classes}`}
      style={{
        borderRadius: `${radius}px`,
        "--glass-clip": outline ? `path("${outline}")` : undefined,
      }}
    >
      {outline && (
        <svg class="glass-silhouette">
          <path d={outline} />
        </svg>
      )}
      <div
        class={`glass-layer ${refracting ? "" : "glass-frost"}`}
        style={refracting ? { backdropFilter: `url(#${id})` } : undefined}
      />
      <div class="glass-layer glass-tint" />
      <div
        class="glass-layer glass-specular"
        style={maps ? { backgroundImage: `url(${maps.specular})` } : undefined}
      />
      <div class="glass-content">{children}</div>
      {refracting && (
        <svg class="absolute w-0 h-0" color-interpolation-filters="sRGB">
          <filter id={id}>
            <feGaussianBlur
              in="SourceGraphic"
              stdDeviation={blur}
              edgeMode="duplicate"
              result="blurred"
            />
            <feImage
              href={maps.displacement}
              x="0"
              y="0"
              width={size.width}
              height={size.height}
              preserveAspectRatio="none"
              result="map"
            />
            <feDisplacementMap
              in="blurred"
              in2="map"
              scale={maps.scale}
              xChannelSelector="R"
              yChannelSelector="G"
              result="refracted"
            />
            <feColorMatrix in="refracted" type="saturate" values="1.6" />
          </filter>
        </svg>
      )}
    </div>
  );
}

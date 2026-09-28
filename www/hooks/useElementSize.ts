import type { RefObject } from "preact";
import { useEffect, useState } from "preact/hooks";

export interface Size {
  width: number;
  height: number;
}

export function useElementSize(ref: RefObject<HTMLElement>): Size | null {
  const [size, setSize] = useState<Size | null>(null);

  useEffect(() => {
    const element = ref.current;
    if (!element) return;

    const observer = new ResizeObserver(([entry]) => {
      const width = Math.round(entry.contentRect.width);
      const height = Math.round(entry.contentRect.height);
      setSize((current) =>
        current?.width === width && current.height === height
          ? current
          : { width, height }
      );
    });
    observer.observe(element);
    return () => observer.disconnect();
  }, [ref]);

  return size;
}

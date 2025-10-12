import { useCallback } from "preact/hooks";

export function useCycler<T>(
  values: readonly T[],
  current: T,
  setCurrent: (value: T) => void,
): () => void {
  return useCallback(() => {
    const currentIndex = values.indexOf(current);
    const nextIndex = (currentIndex + 1) % values.length;
    setCurrent(values[nextIndex]);
  }, [values, current, setCurrent]);
}

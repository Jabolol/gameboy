import { type StateUpdater, useEffect, useState } from "preact/hooks";
import type { StorageAdapter, StorageKey } from "../types/storage.ts";

export function usePersistedState<T>(
  key: StorageKey,
  storage: StorageAdapter<T>,
  defaultValue: T,
): [T, StateUpdater<T>] {
  const [state, setState] = useState<T>(() => storage.get(key, defaultValue));

  useEffect(() => {
    storage.set(key, state);
  }, [key, state, storage]);

  return [state, setState];
}

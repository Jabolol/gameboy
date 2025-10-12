import type {
  Parser,
  Serializer,
  StorageAdapter,
  StorageKey,
  Validator,
} from "../types/storage.ts";

export function createTypedStorage<T>(
  parse: Parser<T>,
  serialize: Serializer<T> = String,
  validate?: Validator<T>,
): StorageAdapter<T> {
  return {
    get(key: StorageKey, defaultValue: T): T {
      if (typeof localStorage === "undefined") return defaultValue;

      const raw = localStorage.getItem(key);
      if (!raw) return defaultValue;

      try {
        const parsed = parse(raw);
        return validate?.(parsed) ?? true ? parsed : defaultValue;
      } catch {
        return defaultValue;
      }
    },

    set(key: StorageKey, value: T): void {
      if (typeof localStorage === "undefined") return;
      localStorage.setItem(key, serialize(value));
    },
  };
}

export const stringStorage = createTypedStorage<string>((v) => v);
export const numberStorage = createTypedStorage<number>(parseFloat);
export const booleanStorage = createTypedStorage<boolean>((v) => v === "true");

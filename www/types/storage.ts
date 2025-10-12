export type StorageKey = `gb-${string}`;

export interface StorageAdapter<T> {
  get(key: StorageKey, defaultValue: T): T;
  set(key: StorageKey, value: T): void;
}

export type Parser<T> = (raw: string) => T;
export type Serializer<T> = (value: T) => string;
export type Validator<T> = (value: T) => boolean;

export function calculateConsensus<T extends string>(
  history: readonly T[],
  threshold: number,
  current: T,
): T {
  if (history.length === 0) return current;

  const counts = history.reduce((acc, item) => {
    acc.set(item, (acc.get(item) ?? 0) + 1);
    return acc;
  }, new Map<T, number>());

  for (const [value, count] of counts.entries()) {
    const ratio = count / history.length;
    if (ratio >= threshold) return value;
    if (ratio <= (1 - threshold)) continue;
  }

  return current;
}

export class ConsensusTracker<T extends string> {
  private history: T[] = [];

  constructor(
    private readonly maxSize: number,
    private readonly threshold: number,
  ) {}

  add(value: T): void {
    this.history.push(value);
    if (this.history.length > this.maxSize) {
      this.history.shift();
    }
  }

  getConsensus(current: T): T {
    return calculateConsensus(this.history, this.threshold, current);
  }

  isReady(): boolean {
    return this.history.length >= this.maxSize;
  }

  reset(): void {
    this.history = [];
  }
}

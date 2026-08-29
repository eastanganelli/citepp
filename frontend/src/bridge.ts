// Typed wrapper around the C++ functions exposed via webview bind().
// Each is injected on window as an async function returning JSON.
// This file is the single source of truth for the C++ <-> JS contract.

declare global {
  interface Window {
    countPrimes: (limit: number) => Promise<{ limit: number; primes: number; ms: number }>;
    systemInfo: () => Promise<{ platform: string; cores: number }>;
  }
}

export const backend = {
  countPrimes: (limit: number) => window.countPrimes(limit),
  systemInfo: () => window.systemInfo(),
};

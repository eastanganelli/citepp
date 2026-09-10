import { useEffect, useState } from "react";
import { backend } from "./bridge";

export default function App() {
  const [sys, setSys] = useState<{ platform: string; cores: number } | null>(null);
  const [limit, setLimit] = useState(200000);
  const [result, setResult] = useState("");
  const [busy, setBusy] = useState(false);

  // Example 2: ask C++ for data on mount.
  useEffect(() => {
    backend.systemInfo().then(setSys).catch(console.error);
  }, []);

  // Example 1: send an argument to C++, get a computed result back.
  async function runPrimes() {
    setBusy(true);
    setResult("computing in C++...");
    const r = await backend.countPrimes(limit);
    setResult(`primes <= ${r.limit}: ${r.primes}  (${r.ms} ms in C++)`);
    setBusy(false);
  }

  return (
    <div style={S.page}>
      <h1 style={S.h1}>citepp — React UI + C++ backend</h1>

      <div style={S.card}>
        {sys ? `Platform: ${sys.platform} · CPU cores: ${sys.cores}`
             : "loading system info from C++..."}
      </div>

      <div style={S.card}>
        <label style={S.label}>Count primes up to</label>
        <input style={S.input} type="number" value={limit}
               onChange={(e) => setLimit(+e.target.value)} />
        <button style={S.btn} disabled={busy} onClick={runPrimes}>Run in C++</button>
        <pre style={S.out}>{result || "result here"}</pre>
      </div>
    </div>
  );
}

const S: Record<string, React.CSSProperties> = {
  // height (not minHeight) + overflowY keeps the page pinned to the window and
  // scrolls the content inside it instead of growing the document.
  page: { fontFamily: "system-ui, sans-serif", background: "#0f1117",
          color: "#e6e6e6", height: "100%", overflowY: "auto", padding: 24,
          display: "flex", flexDirection: "column", gap: 16, alignItems: "center" },
  h1: { fontSize: 20, margin: 0 },
  card: { background: "#171a23", border: "1px solid #262b38", borderRadius: 12,
          padding: 20, width: "100%", maxWidth: 480 },
  label: { fontSize: 13, color: "#8b93a7", display: "block", marginBottom: 6 },
  input: { width: "100%", boxSizing: "border-box", padding: 10, marginBottom: 12,
           background: "#0f1117", color: "#e6e6e6", border: "1px solid #2d3342",
           borderRadius: 8 },
  btn: { width: "100%", padding: 10, background: "#3b82f6", color: "#fff",
         border: "none", borderRadius: 8, fontWeight: 600, cursor: "pointer" },
  out: { background: "#0b0d13", border: "1px solid #262b38", borderRadius: 8,
         padding: 12, marginTop: 12, color: "#7ee787", whiteSpace: "pre-wrap" },
};

import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import type { Plugin } from "vite";

// Vite's HTML plugin always tags the entry as <script type="module" crossorigin>,
// whatever the rollup output format is. The bundle we emit is an IIFE, so rewrite
// the tag into a classic script; see the note on the build config below. "defer"
// has to be added explicitly: a module script is deferred by default, a classic
// one is not, and the entry sits in <head> where #root does not exist yet.
function classicEntryScript(): Plugin {
  return {
    name: "citepp:classic-entry-script",
    enforce: "post",
    apply: "build",
    transformIndexHtml(html) {
      return html
        .replace(/<script\s+type="module"\s+crossorigin\s+/g, "<script defer ")
        .replace(/<script\s+type="module"\s+/g, "<script defer ");
    },
  };
}

// base: "./"  -> relative asset paths, required for file:// loading in webview.
// build.outDir "dist" -> CMake copies it to <bin>/ui.
//
// The bundle is emitted as a classic IIFE script, not as an ES module: the page
// is loaded over file://, where a document's origin is "null" and the CORS
// check on <script type="module"> fetches always fails, leaving a blank window.
// Classic scripts are exempt from that check, so the output must not be split
// into modules and must not carry a crossorigin attribute.
export default defineConfig({
  plugins: [react(), classicEntryScript()],
  base: "./",
  build: {
    outDir: "dist",
    emptyOutDir: true,
    // Emitting <link rel="modulepreload"> would reintroduce module loading.
    modulePreload: false,
    cssCodeSplit: false,
    // Inline every asset as a data: URI so nothing is fetched at runtime.
    assetsInlineLimit: Number.MAX_SAFE_INTEGER,
    rollupOptions: {
      output: {
        format: "iife",
        // An IIFE bundle cannot be split, so dynamic imports must be inlined.
        inlineDynamicImports: true,
        entryFileNames: "assets/[name]-[hash].js",
        assetFileNames: "assets/[name]-[hash].[ext]",
      },
    },
  },
});

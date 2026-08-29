import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// base: "./"  -> relative asset paths, required for file:// loading in webview.
// build.outDir "dist" -> CMake copies it to <bin>/ui.
export default defineConfig({
  plugins: [react()],
  base: "./",
  build: { outDir: "dist", emptyOutDir: true },
});

import { defineConfig, Plugin } from "vite";
import { fresh } from "@fresh/plugin-vite";
import tailwindcss from "@tailwindcss/vite";

function gameboyWorkerPlugin(): Plugin {
  return {
    name: "gameboy-worker-fix",
    enforce: "pre",
    transform(code, id) {
      if (!id.includes("static/gameboy.js")) {
        return null;
      }

      const transformed = code.replace(
        /new\s+Worker\s*\(\s*new\s+URL\s*\(\s*["']gameboy\.js["']\s*,\s*import\.meta\.url\s*\)/g,
        "new Worker(import.meta.url",
      );

      return transformed !== code ? { code: transformed } : null;
    },
  };
}

export default defineConfig({
  plugins: [
    gameboyWorkerPlugin(),
    fresh(),
    tailwindcss(),
  ],
  server: {
    headers: {
      "Cross-Origin-Opener-Policy": "same-origin",
      "Cross-Origin-Embedder-Policy": "require-corp",
    },
  },
});

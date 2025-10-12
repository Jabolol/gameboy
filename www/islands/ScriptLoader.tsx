import { useGameboyInitializer } from "../hooks/useGameboyInitializer.ts";

export default function ScriptLoader() {
  useGameboyInitializer();
  return null;
}

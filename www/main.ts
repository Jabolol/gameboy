import "@std/dotenv/load";
import { App, staticFiles } from "fresh";
import policy from "./middleware/policy.ts";
import ga4 from "./middleware/ga4.ts";

export const app = new App()
  .use(policy)
  .use(ga4)
  .use(staticFiles())
  .fsRoutes();

if (import.meta.main) {
  await app.listen();
}

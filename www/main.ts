import "$std/dotenv/load.ts";
import { App, Context, staticFiles } from "fresh";

const middleware = async (ctx: Context<unknown>) => {
  const resp = await ctx.next();
  resp.headers.set("Cross-Origin-Embedder-Policy", "require-corp");
  resp.headers.set("Cross-Origin-Opener-Policy", "same-origin");
  resp.headers.set("Cross-Origin-Resource-Policy", "same-origin");
  return resp;
};

export const app = new App()
  .use(middleware)
  .use(staticFiles())
  .fsRoutes();

if (import.meta.main) {
  await app.listen();
}

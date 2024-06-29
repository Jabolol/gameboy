import { FreshContext } from "$fresh/server.ts";

export async function handler(_req: Request, ctx: FreshContext) {
  const resp = await ctx.next();
  resp.headers.set("Cross-Origin-Embedder-Policy", "require-corp");
  resp.headers.set("Cross-Origin-Opener-Policy", "same-origin");

  return resp;
}

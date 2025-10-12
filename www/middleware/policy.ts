import { Context } from "fresh";

const policy = async (ctx: Context<unknown>) => {
  const resp = await ctx.next();
  resp.headers.set("Cross-Origin-Embedder-Policy", "require-corp");
  resp.headers.set("Cross-Origin-Opener-Policy", "same-origin");
  resp.headers.set("Cross-Origin-Resource-Policy", "same-origin");
  return resp;
};

export { policy as default };

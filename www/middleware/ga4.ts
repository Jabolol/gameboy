import { Context } from "fresh";
import { GA4Report } from "../utils/ga4.ts";

const track = async (ctx: Context<unknown>) => {
  const res = await ctx.next();

  if (!res.headers.get("content-type")?.includes("text/html")) {
    return res;
  }

  const report = new GA4Report({
    request: ctx.req,
    response: res,
    conn: {
      remoteAddr: ctx.info.remoteAddr as Deno.NetAddr,
    },
  });

  await report.send();

  return res;
};

export { track as default };

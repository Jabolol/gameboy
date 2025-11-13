import { type PageProps } from "fresh";
import ogImage from "../static/web-ui.png?url";

export default function App({ Component, url }: PageProps) {
  const ogImageUrl = new URL(ogImage, url).href;

  return (
    <html lang="en">
      <head>
        <meta charset="utf-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
        <title>Gameboy</title>
        <meta
          name="description"
          content="An accurate gameboy emulator written in C from scratch with a Deno web interface"
        />
        <meta property="og:type" content="website" />
        <meta property="og:url" content="https://gameboy.deno.dev/" />
        <meta property="og:title" content="Gameboy Emulator" />
        <meta
          property="og:description"
          content="An accurate gameboy emulator written in C from scratch with a Deno web interface"
        />
        <meta
          property="og:image"
          content={ogImageUrl}
        />
        <meta property="og:image:type" content="image/png" />
        <meta property="og:image:width" content="1200" />
        <meta property="og:image:height" content="721" />
        <meta name="twitter:card" content="summary_large_image" />
        <meta name="twitter:url" content="https://gameboy.deno.dev/" />
        <meta name="twitter:title" content="Gameboy Emulator" />
        <meta
          name="twitter:description"
          content="An accurate gameboy emulator written in C from scratch with a Deno web interface"
        />
        <meta
          name="twitter:image"
          content={ogImageUrl}
        />
        <meta name="theme-color" content="#ffffff" />
      </head>
      <body>
        <Component />
      </body>
    </html>
  );
}

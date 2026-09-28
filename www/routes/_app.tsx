import { type PageProps } from "fresh";
import ogImage from "../static/web-ui.png?url";

export default function App({ Component, url }: PageProps) {
  const ogImageUrl = new URL(ogImage, url).href;

  return (
    <html lang="en">
      <head>
        <meta charset="utf-8" />
        <meta
          name="viewport"
          content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover"
        />
        <title>Gameboy</title>
        <meta
          name="description"
          content="An accurate gameboy emulator written in C from scratch with a Deno web interface"
        />
        <meta property="og:type" content="website" />
        <meta property="og:url" content="https://gameboy.monad.deno.net/" />
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
        <meta name="twitter:url" content="https://gameboy.monad.deno.net/" />
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
        <meta name="mobile-web-app-capable" content="yes" />
        <meta name="apple-mobile-web-app-capable" content="yes" />
        <meta
          name="apple-mobile-web-app-status-bar-style"
          content="black-translucent"
        />
        <link rel="manifest" href="/manifest.webmanifest" />
        <link rel="icon" href="/favicon.ico" sizes="48x48" />
        <link rel="icon" href="/icon.svg" type="image/svg+xml" />
        <link rel="apple-touch-icon" href="/apple-touch-icon.png" />
      </head>
      <body>
        <Component />
      </body>
    </html>
  );
}

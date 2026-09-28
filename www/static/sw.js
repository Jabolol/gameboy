const CACHE = "gameboy-v1";

self.addEventListener("install", () => self.skipWaiting());

self.addEventListener("activate", (event) => {
  event.waitUntil(self.clients.claim());
});

self.addEventListener("fetch", (event) => {
  const { request } = event;
  const url = new URL(request.url);

  if (request.method !== "GET" || url.origin !== self.location.origin) return;

  event.respondWith(
    url.pathname.startsWith("/roms/")
      ? cacheFirst(request)
      : networkFirst(request),
  );
});

async function store(request, response) {
  if (response.ok) {
    const cache = await caches.open(CACHE);
    await cache.put(request, response.clone());
  }
  return response;
}

async function cacheFirst(request) {
  return await caches.match(request) ?? store(request, await fetch(request));
}

async function networkFirst(request) {
  try {
    return await store(request, await fetch(request));
  } catch (error) {
    const cached = await caches.match(request, {
      ignoreSearch: request.mode === "navigate",
    });
    if (cached) return cached;
    throw error;
  }
}

import "./static/styles.css";
import "./static/web-ui.png";

if (import.meta.env.PROD && "serviceWorker" in navigator) {
  navigator.serviceWorker.register("/sw.js");
}

for (const type of ["gesturestart", "gesturechange", "dblclick"]) {
  document.addEventListener(type, (event) => event.preventDefault(), {
    passive: false,
  });
}

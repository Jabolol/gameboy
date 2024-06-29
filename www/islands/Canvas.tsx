export default function Canvas() {
  return <canvas id="canvas" onContextMenu={(evt) => evt.preventDefault()} />;
}

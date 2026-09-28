import Canvas from "../islands/Canvas.tsx";

export default function Home() {
  return (
    <div
      className="app-screen w-full overflow-hidden bg-white dark:bg-black"
      style={{
        backgroundColor: "rgb(var(--bg-color, 255 255 255))",
      }}
    >
      <Canvas />
    </div>
  );
}

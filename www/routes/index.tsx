import Canvas from "../islands/Canvas.tsx";

export default function Home() {
  return (
    <div
      className="flex items-center justify-center w-full min-h-screen bg-white dark:bg-black"
      style={{
        backgroundColor: "rgb(var(--bg-color, 255 255 255))",
      }}
    >
      <Canvas />
    </div>
  );
}

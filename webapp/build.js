import fs from "fs"
import esbuild from "esbuild"

/** @type {import("esbuild").BuildOptions} */
const config = {
  entryPoints: ["main.ts"],
  outdir: "dist",
  bundle: true,
  minify: false,
  logLevel: "info",
  write: false,
}

const jsResult = await esbuild.build(config)
const js = jsResult.outputFiles[0].text

const html = `
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" />
</head>
<body>
  <canvas id="realtimeChart" width="600" height="300"></canvas>
  <div id="lastUpdate"></div>
  <select id="topicSelect">
    <option value="temp" selected>温湿度計</option>
    <option value="acc">加速度センサ</option>
    <option value="water">冷却液 温度センサ</option>
  </select>
  <label>
    <input type="number" id="numberLimit" step="1" value="100">
    上限
  </label>
  <label>
    <input type="checkbox" id="checkStop">
    ストップ
  </label>
  <label>
    <input type="checkbox" id="checkAlign">
    Y軸を揃える
  </label>
  <script>${js}</script>
</body>
</html>
`

if (!fs.existsSync("dist")) {
  fs.mkdirSync("dist")
}
fs.writeFileSync("dist/index.html", html)

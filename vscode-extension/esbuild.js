// esbuild.js - build script cho neon-mdf-tools extension. `serialport` la
// native binding (.node binary) - KHONG bundle duoc, phai de nguyen trong
// node_modules va ship kem trong .vsix (xem package.json "vsix" script +
// kiem tra bang `vsce ls` truoc khi coi la xong).
const esbuild = require("esbuild");

const production = process.argv.includes("--production");
const watch = process.argv.includes("--watch");

async function main() {
  const ctx = await esbuild.context({
    entryPoints: ["src/extension.ts"],
    bundle: true,
    outfile: "dist/extension.js",
    external: ["vscode", "serialport", "@serialport/bindings-cpp"],
    format: "cjs",
    platform: "node",
    sourcemap: !production,
    minify: production,
    logLevel: "info",
  });

  if (watch) {
    await ctx.watch();
  } else {
    await ctx.rebuild();
    await ctx.dispose();
  }
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});

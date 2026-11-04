// esbuild.js - build script for neon-mdf-tools extension. `serialport` is native binding (.node
// binary) - cannot bundle, must be left in node_modules and ship included in .vsix (see
// package.json "vsix" script + check with `vsce ls` before deemed complete).
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

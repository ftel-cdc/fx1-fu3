# Core/Arch/CortexM/CMSIS

- `CMSIS_5/` is vendored from ARM's official repository
  (`https://github.com/ARM-software/CMSIS_5`). It contains only the sparse
  `CMSIS/Core/Include` tree, excluding DSP, NN, RTOS2 and device packs that this
  SDK does not need. Do not edit these vendor files manually.

  **Current status:** this is a **plain vendored copy**, not an active Git
  submodule; the original repository had not been initialized with `git init`.
  To convert it to a submodule after initializing this repository, allowing
  future ARM fixes through `git submodule update --remote`:

  ```sh
  rm -rf Core/Arch/CortexM/CMSIS/CMSIS_5   # Remove the current vendored copy
  git submodule add https://github.com/ARM-software/CMSIS_5.git Core/Arch/CortexM/CMSIS/CMSIS_5
  cd Core/Arch/CortexM/CMSIS/CMSIS_5
  git sparse-checkout init --cone
  git sparse-checkout set CMSIS/Core/Include
  ```

- `Device/<Family>/` contains headers, linker scripts and startup code generated
  from `mcu-description/`; see `mcu-description/tools/generate.py` and
  `Device/FU2/`. Paths use a single family level rather than
  `<Vendor>/<Family>/`. Do not edit generated files manually. Only
  `system_<DEVICE>.c/.h` are handwritten.

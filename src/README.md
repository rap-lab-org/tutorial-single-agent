1. `make gen`: this will generate `compile_commands.json` in `build`, your editor may need this for auto completiton;
2. Compile:
  - `make dev`: compile programs in `Debug` mode, the executables will running slower but more friendly to external debugger (e.g., `gdb`, `lldb`);
  - `make fast`: compile programs in `Release` mode, the executables will running faster;
3. `make clean`: remove all existing building files, useful when you want to switch between `make dev` and `make fast`

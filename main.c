#define REBUILD_CMD "gcc -Wall -Wextra -pedantic -std=c23 main.c -o vmake -lmcu-debug"
#define VMAKE_IMPL
#include "vmake.h"

#include <mcu/handlers.h>

Vmake vmake;

i32 main(i32 argc, cstr argv[]) {
   vmake = Vmake_new();
   Vmake_go_rebuild_yourself(argc, argv);

   ModuleId module_1 = Module_new("module 1", MT_Executable);

   execute_command(false, "echo \"zhyivannye miratte\"");
   return 0;
}


#define VMAKE_IMPL
#include "vmake.h"

#include <mcu/handlers.h>

Vmake vmake;

i32 main(i32 argc, cstr argv[]) {
   vmake = Vmake_new();

   execute_command(false, "echo \"zhyivannye miratte\"");
   return 0;
}


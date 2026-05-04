#define VMAKE_IMPL
#include "vmake.h"

#include <mcu/handlers.h>

Vmake vmake;

i32 main(i32 argc, cstr argv[]) {
   vmake = Vmake_new();
   Vmake_go_rebuild_yourself(argc, argv);

   BuildOptions build_options = {
      .compiler = GCC,
      .optimization = Og,
      .standard = C23,
      .warnings = {
         .wall = true,
         .wextra = true,
         .pedantic = true
      }
   };

   ModuleId module_1 = Module_new("module-1", "example_module_1", MT_Executable);
   if (Vmake_build(module_1, build_options)) return 1;
   
   return 0;
}


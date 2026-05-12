#define VMAKE_IMPL
#include <vmake.h>

#include <mcu/handlers.h>

Vmake vmake;

bool build() {
   BuildOptions build_options = {
      .compiler     = GCC,
      .optimization = Og,
      .standard     = C23,
      .warnings = {
         .wall     = true,
         .wextra   = true,
         .pedantic = true
      },
      .link_mcu = true,
      .debug_mode = true
   };

   ModuleId mod_1 = Module_new("module-1", "./module_1", MT_Executable);
   return Vmake_build(mod_1, build_options);
}

i32 main(i32 argc, cstr argv[]) {
   vmake = Vmake_go_rebuild_yourself(argc, argv);
   
   i32 result = 0;
   for (i32 i = 1; i < argc - 1; ++i) {
      if (result != 0) return result;
   
      cstr_match(argv[i]) {
         ncstreq("build") {
            result = build();
         }
         cstreq("run") {
            result = execute_command("build/bin/module-1");
         }
         else {
            eprintln("Unknown command \"%s\"", argv[i]);
            return 0;
         }
      }
   }

   return result;
}


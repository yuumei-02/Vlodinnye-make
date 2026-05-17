#ifndef VMAKE_H
#define VMAKE_H

#define _POSIX_C_SOURCE 200809L
#include <mcu/core.h>
#include <mcu/io.h>

typedef enum : u8 {
   GCC,
   Clang,
} Compiler;

typedef enum : u8 {
   Og,
   Os, Oz,
   O0, O1,
   O2, O3,
} Optimization;

typedef enum : u8 {
   C23, C11, C99
} LanguageStandard;

typedef struct {
   Compiler compiler;
   Optimization optimization;
   LanguageStandard standard;
   struct {
      bool wall;
      bool wextra;
      bool pedantic;
      bool no_override_init;
   } warnings;
   bool link_mcu;
   bool debug_mode;
} BuildOptions;

/// Module ID
typedef usize ModuleId;

typedef enum {
   MT_Executable,
   MT_StaticLibrary,
   MT_Test
} ModuleType;

typedef struct {
   ModuleType type;
   String output_name;
   String path;
   Vector ModuleId_dep;
   Vector String_external_dep;
} Module;

typedef struct {
   Vector Modules;
} Vmake;

extern Vmake vmake;

const cstr Compiler_to_cstr(Compiler self);
const cstr Optimization_to_cstr(Optimization self);
const cstr LanguageStandard_to_cstr(LanguageStandard self);

BuildOptions BuildOptions_default_debug();
BuildOptions BuildOptions_default_release();

ModuleId Module_new(const cstr output_name, const cstr path, ModuleType type);
void Module_add_dependency(ModuleId self, ModuleId dependency);
void Module_add_external_dependency(ModuleId self, const cstr dependency_name);

Vmake Vmake_go_rebuild_yourself(i32 argc, cstr argv[]);
bool Vmake_build(ModuleId module, BuildOptions build_options);

/// returns [-1] on [failure].
/// returns [command]'s exit code on success.
i32 execute_command_impl(bool echo, bool suppress, const cstr format, ...);

#define execute_command(format, ...) \
   execute_command_impl(true, false, format __VA_OPT__(,) __VA_ARGS__)

#endif

#ifdef VMAKE_IMPL
#undef VMAKE_IMPL

#include <mcu/handlers.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

const cstr Compiler_to_cstr(Compiler self) {
   switch (self) {
      case GCC:   return "gcc";
      case Clang: return "clang";
   }

   panic("Invalid Compiler enum variant");
}

const cstr Optimization_to_cstr(Optimization self) {
   switch (self) {
      case Og: return "Og";
      case Os: return "Os";
      case Oz: return "Oz";
      case O0: return "O0";
      case O1: return "O1";
      case O2: return "O2";
      case O3: return "O3";
   }

   panic("Invalid Optimization enum variant");
}

const cstr LanguageStandard_to_cstr(LanguageStandard self) {
   switch (self) {
      case C23: return "c23";
      case C11: return "c11";
      case C99: return "c99";
   }

   panic("Invalid LanguageStandard enum variant");
}

BuildOptions BuildOptions_default_debug() {
   return (BuildOptions) {
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
}

BuildOptions BuildOptions_default_release() {
   return (BuildOptions) {
      .compiler     = GCC,
      .optimization = O2,
      .standard     = C23,
      .warnings = {
         .wall     = false,
         .wextra   = false,
         .pedantic = false
      },
      .link_mcu = true,
      .debug_mode = false
   };
}

ModuleId Module_new(const cstr output_name, const cstr path, ModuleType type) {
   mcu_assert(output_name != nullptr, "output_name can't be null");
   mcu_assert(path != nullptr, "path can't be null");
   
   Module self = {
      .type = type,
      .output_name = String_from((cstr) output_name),
      .path = String_from((cstr) path),
      .ModuleId_dep = Vector_new(sizeof(ModuleId)),
      .String_external_dep = Vector_new(sizeof(String))
   };

   Vector_push(&vmake.Modules, &self);
   return vmake.Modules.length - 1;
}

void Module_add_dependency(ModuleId self, ModuleId dependency) {
   Module* l_self = Vector_get(&vmake.Modules, self);
   Module* l_dep = Vector_get(&vmake.Modules, dependency);
   
   mcu_assert(l_self != nullptr, "Nonexistant module (self)");
   mcu_assert(l_dep != nullptr, "Nonexistant module (dependency)");

   Vector_push(&l_self->ModuleId_dep, &dependency);
}

void Module_add_external_dependency(ModuleId self, const cstr dependency_name) {
   Module* mod = Vector_get(&vmake.Modules, self);
   
   mcu_assert(mod != nullptr, "Nonexistant module");
   mcu_assert(dependency_name != nullptr, "dependency_name can't be null");

   String dep = String_from(dependency_name);
   Vector_push(&mod->String_external_dep, &dep);
}

#ifndef REBUILD_CMD
#define REBUILD_CMD "gcc -Wall -Wextra -pedantic -std=c23 vmake.c -o vmake -lmcu-debug"
#endif

Vmake Vmake_go_rebuild_yourself(i32 argc, cstr argv[]) {
   Vmake self = {
      .Modules = Vector_new(sizeof(Module))
   };
   
   for (i32 i = 1; i < argc; ++i) {
      if (strcmp(argv[i], "--no-rebuild") == 0) return self;
   }

   if (execute_command_impl(true, true, "mkdir -p build/bin")) goto failure;
   if (execute_command_impl(true, true, "mkdir -p build/lib")) goto failure;
   if (execute_command_impl(true, true, "mkdir -p build/obj")) goto failure;

   i32 result;
   // @todo: replace vmake with argv[0]
   result = execute_command_impl(false, true, "mv ./vmake ./vmake-old");
   if (result != 0) goto failure;
   
   result = execute_command_impl(false, false, REBUILD_CMD);
   if (result != 0) {
      execute_command_impl(false, true, "mv ./vmake-old ./vmake");
      goto failure;
   }

   execute_command_impl(false, true, "rm ./vmake-old");

   // @todo: pass current arguments
   String new_cmd = String_from("./vmake ");
   for (i32 i = 1; i < argc; ++i) {
      String_append_cstr(&new_cmd, argv[i]);
      String_append(&new_cmd, ' ');
   }
   String_append_cstr(&new_cmd, "--no-rebuild");
   
   i32 exit_code = execute_command_impl(true, false, new_cmd.chars);
   String_free(&new_cmd);
   if (exit_code != 0) exit(1);
   exit(0);

failure:
   eprintln("[!] Rebuild failed");
   exit(1);
}

bool Vmake_build(ModuleId module, BuildOptions build_options) {
   Module* mod = Vector_get(&vmake.Modules, module);
   if (mod == nullptr) {
      eprintln("[!] Module not found");
      return true;
   }

   foreach (mod->ModuleId_dep, i) {
      ModuleId* mod_id = Vector_get(&mod->ModuleId_dep, i);
      if (Vmake_build(*mod_id, build_options)) goto failure;
   }

   println("[i] Building module \"%s\"", mod->path.chars);

   String build_cmd = String_from((cstr) Compiler_to_cstr(build_options.compiler));
   if (build_options.warnings.wall)     String_append_cstr(&build_cmd, " -Wall");
   if (build_options.warnings.wextra)   String_append_cstr(&build_cmd, " -Wextra");
   if (build_options.warnings.pedantic) String_append_cstr(&build_cmd, " -pedantic");
   if (build_options.warnings.no_override_init) String_append_cstr(&build_cmd, " -Wno-override-init");
   if (!build_options.debug_mode)       String_append_cstr(&build_cmd, " -DNDEBUG");
   String_appendf(&build_cmd, " -std=%s -%s",
      LanguageStandard_to_cstr(build_options.standard),
      Optimization_to_cstr(build_options.optimization));
   String_append_cstr(&build_cmd, " -I./build/lib -L./build/lib");

   switch (mod->type) {
      case MT_Test: {
         String_appendf(&build_cmd, " -x c %s/*.ct", mod->path.chars);
      } [[fallthrough]];
      case MT_Executable: {
         String_appendf(&build_cmd, " %s/*.c -o build/bin/%s", mod->path.chars, mod->output_name.chars);

         foreach (mod->ModuleId_dep, i) {
            Module* dep = Vector_get(&vmake.Modules, *(ModuleId*) Vector_get(&mod->ModuleId_dep, i));
            String_appendf(&build_cmd, " -l%s", dep->output_name.chars);
         }

         foreach (mod->String_external_dep, i) {
            String* extrn_dep = Vector_get(&mod->String_external_dep, i);
            String_appendf(&build_cmd, " -l%s", extrn_dep->chars);
         }

         if (build_options.link_mcu) {
            String_append_cstr(&build_cmd, build_options.debug_mode
               ? " -lmcu-debug"
               : " -lmcu-release");
         }

         i32 result = execute_command_impl(true, false, build_cmd.chars);
         String_free(&build_cmd);
         if (result != 0) goto failure;
      } break;

      case MT_StaticLibrary: {
         String_appendf(&build_cmd, " -c %s/*.c", mod->path.chars);

         i32 result = execute_command_impl(true, false, build_cmd.chars);
         if (result != 0) {
            String_free(&build_cmd);
            goto failure;
         }

         String_clear(&build_cmd);
         String_appendf(&build_cmd, "ar rcs ./build/lib/lib%s.a *.o", mod->output_name.chars);

         result = execute_command_impl(true, false, build_cmd.chars);
         if (result != 0) {
            String_free(&build_cmd);
            goto failure;
         }

         String_clear(&build_cmd);
         String_append_cstr(&build_cmd, "rm *.o");

         result = execute_command_impl(true, false, build_cmd.chars);
         String_free(&build_cmd);
         if (result != 0) goto failure;
      } break;

      default: {
         panic("unreachable");
      }
   }
   
   println("[i] Build successfull");
   return false;

failure:
   eprintln("[!] Build failed");
   return true;
}

// @todo: make variadic
i32 execute_command_impl(bool echo, bool suppress, const cstr format, ...) {
   va_list args;
   va_start(args, format);
   String command = String_new();
   String_appendfv(&command, format, args);
   va_end(args);
   
   char output_buffer[1024*4];
   String_append_cstr(&command, " 2>&1");

   if (echo) println("[i] %s", command.chars);

   FILE* childp = popen(command.chars, "r");
   String_free(&command);
   if (childp == nullptr) {
      eprintln("[!] Failed to execute command, reason: \"%s\"", strerror(errno));
      return -1;
   }

   while (fgets(output_buffer, sizeof(output_buffer), childp) != null) {
      if (!suppress) printf("%s", output_buffer);
   }

   if (ferror(childp)) {
      eprintln("[!] Failed to read from pipe, reason: \"%s\"", strerror(errno));
      pclose(childp);
      return -1;
   }

   i32 result = pclose(childp);
   if (result == -1) {
      eprintln("[!] Failed to close pipe, reason: \"%s\"", strerror(errno));
      eprintln("[!] Proceeding...");
   }

   return result;
}

#endif


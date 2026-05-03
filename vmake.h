#ifndef VMAKE_H
#define VMAKE_H

#define _POSIX_C_SOURCE 200809L
#include <mcu/core.h>
#include <mcu/io.h>

/// Module ID
typedef usize ModuleId;

typedef enum {
   MT_Executable
} ModuleType;

typedef struct {
   ModuleType type;
   String name;
   Vector ModuleId_dep;
} Module;

typedef struct {
   Vector Modules;
} Vmake;

extern Vmake vmake;

ModuleId Module_new(const cstr name, ModuleType type);
void Module_add_dependency(ModuleId self, ModuleId dependency);

Vmake Vmake_new();
void Vmake_go_rebuild_yourself(i32 argc, cstr argv[]);

/// returns [-1] on [failure].
/// returns [command]'s exit code on success.
i32 execute_command(bool suppress, nullable const cstr command);

#endif

#ifdef VMAKE_IMPL
#undef VMAKE_IMPL

#include <stdlib.h>
#include <string.h>
#include <errno.h>

ModuleId Module_new(const cstr name, ModuleType type) {
   mcu_assert(name != nullptr, "name can't be null");
   
   Module self = {
      .type = type,
      .name = String_from((cstr) name),
      .ModuleId_dep = Vector_new(sizeof(ModuleId))
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

Vmake Vmake_new() {
   Vmake self = {
      .Modules = Vector_new(sizeof(Module))
   };
   
   return self;
}

#ifndef REBUILD_CMD
#define REBUILD_CMD "gcc -Wall -Wextra -pedantic -std=c23 vmake.c -o vmake -lmcu-debug"
#endif

void Vmake_go_rebuild_yourself(i32 argc, cstr argv[]) {
   for (i32 i = 1; i < argc; ++i) {
      if (strcmp(argv[i], "--no-rebuild") == 0) return;
   }

   i32 result;
   // @todo: replace vmake with argv[0]
   result = execute_command(true, "mv ./vmake ./vmake-old");
   if (result != 0) goto failure;
   
   result = execute_command(true, REBUILD_CMD);
   if (result != 0) {
      execute_command(true, "mv ./vmake-old ./vmake");
      goto failure;
   }

   // @todo: pass current arguments
   exit(execute_command(false, "./vmake --no-rebuild"));
   return;

failure:
   eprintln("[!] Rebuild failed");
   exit(1);
}

// @todo: make variadic
i32 execute_command(bool suppress, nullable const cstr command) {
   if (command == nullptr) return 0;

   char output_buffer[1024*4];

   FILE* childp = popen(command, "r");
   if (childp == nullptr) {
      eprintln("Failed to execute command, reason: \"%s\"", strerror(errno));
      return -1;
   }

   while (fgets(output_buffer, sizeof(output_buffer), childp) != null) {
      if (!suppress) printf("%s", output_buffer);
   }

   if (ferror(childp)) {
      eprintln("Failed to read from pipe, reason: \"%s\"", strerror(errno));
      pclose(childp);
      return -1;
   }

   i32 result = pclose(childp);
   if (result == -1) {
      eprintln("Failed to close pipe, reason: \"%s\"", strerror(errno));
   }
   
   return result;
}

#endif


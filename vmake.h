#ifndef VMAKE_H
#define VMAKE_H

#define _POSIX_C_SOURCE 200809L
#include <mcu/core.h>
#include <mcu/io.h>

/// Module ID
typedef usize MID;

typedef enum {
   MT_Executable
} ModuleType;

typedef struct {
   MID id;
   ModuleType type;
   String name;
   Vector MID_dep;
} Module;

typedef struct {
   Vector Modules;
} Vmake;

extern Vmake vmake;

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

Vmake Vmake_new() {
   Vmake self = {
      .Modules = Vector_new(sizeof(Module))
   };
   
   return self;
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

void Vmake_go_rebuild_yourself(i32 argc, cstr argv[]) {}

#endif


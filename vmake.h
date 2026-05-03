#ifndef VMAKE_H
#define VMAKE_H

#include <mcu/core.h>

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

#endif

#ifdefine VMAKE_IMPL
#undef VMAKE_IMPL

Vmake Vmake_new() {
   Vmake self = {
      .Modules = Vector_new(sizeof(Module))
   };
   
   return self;
}

void Vmake_go_rebuild_yourself(i32 argc, cstr argv[]);

#endif


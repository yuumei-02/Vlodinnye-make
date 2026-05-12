#include <mcu/core.h>
#include <mcu/handlers.h>
#include <mcu/io.h>

#include <mcu/memory.h>

i32 main() {
   const cstr msg = "Hello from module-1";
   cstr lib_usage = mcu_malloc(strlen(msg));
   memcpy(lib_usage, msg, strlen(msg));

   println("%s", lib_usage);
   return 0;
}


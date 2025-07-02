#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

struct StartArgs {
  bool host;
  const char *config;
  const void *ipc_window;
  const uint16_t *instance_name;
};

struct EverythingExeOffsets {
  uint32_t regcomp_p3;
  uintptr_t regcomp_p3_search;
  uintptr_t regcomp_p3_filter;
  uint32_t regcomp_p;
  uint32_t regcomp_p2;
  uintptr_t regcomp_p2_termtext_0;
  uintptr_t regcomp_p2_termtext_1;
  uintptr_t regcomp_p2_modifiers;
  uint32_t regcomp;
  uint32_t regexec;
};

extern "C" {

extern bool start(const StartArgs *args);

extern void stop();

void plugin_start();

void plugin_stop();

EverythingExeOffsets get_everything_exe_offsets();

}  // extern "C"

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

using regoff_t = int32_t;

/// The structure in which a captured offset is returned.
struct regmatch_t {
  regoff_t rm_so;
  regoff_t rm_eo;
};

extern "C" {

extern bool start(const StartArgs *args);

extern void stop();

void plugin_start();

void plugin_stop();

EverythingExeOffsets get_everything_exe_offsets();

const void *search_compile(const char *pattern, uint32_t cflags, uint32_t modifiers);

int32_t search_exec(const void *matcher,
                    const char *haystack,
                    uint32_t length,
                    uintptr_t nmatch,
                    regmatch_t *pmatch,
                    uint32_t eflags);

void search_free(void *matcher);

}  // extern "C"

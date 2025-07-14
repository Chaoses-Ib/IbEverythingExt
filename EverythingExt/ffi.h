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

/// See https://www.pcre.org/original/doc/html/pcre_compile.html
///
/// Usually 0x441
using PcreFlags = uint32_t;

using Modifiers = uint32_t;

using regoff_t = int32_t;

/// The structure in which a captured offset is returned.
struct regmatch_t {
  regoff_t rm_so;
  regoff_t rm_eo;
};

extern "C" {

void plugin_start();

void plugin_stop();

extern bool start(const StartArgs *args);

extern void stop();

void on_ipc_window_created();

EverythingExeOffsets get_everything_exe_offsets();

const void *search_compile(const char *pattern, PcreFlags cflags, Modifiers modifiers);

int32_t search_exec(const void *matcher,
                    const char *haystack,
                    uint32_t length,
                    uintptr_t nmatch,
                    regmatch_t *pmatch,
                    PcreFlags eflags);

void search_free(void *matcher);

}  // extern "C"

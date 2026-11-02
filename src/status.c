#include "status.h"

const char *status_str(status_t st) {
    // No default case, so clang warns if a code is added to status_t and not handled here.
    switch (st) {
    case ST_OK:         return "ok";
    case ST_NOT_FOUND:  return "not found";
    case ST_CORRUPTION: return "corruption";
    case ST_IO_ERROR:   return "io error";
    case ST_NO_MEMORY:  return "out of memory";
    }
    return "unknown";
}

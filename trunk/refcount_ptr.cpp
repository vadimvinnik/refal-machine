#include <map>
#include "refcount_ptr_internals.h"

using std::map;

typedef unsigned long int count_t;

static map<void const*, count_t> refcount_map;

void add_ref(void const* ptr) {
  if (nullptr != ptr) {
    ++refcount_map[ptr];
  }
}

bool delete_ref_is_last(void const* ptr) {
  if (nullptr != ptr) {
    return (0 == --refcount_map.at(ptr));
  }

  return false;
}


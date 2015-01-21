#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unwind.h>

#include <exception>

int num_caughts = 0;
int num_cleanups = 0;

class SuccessExit {
public:
  ~SuccessExit() {
    assert(num_caughts == 1 && "catch (...) should catch force unwind");
    assert(num_cleanups == 1 && "cleanup should be executed once");
    exit(0);
  }
};

class Cleanup {
public:
  ~Cleanup() {
    ++num_cleanups;
  }
};

_Unwind_Exception *test_allocate_exception() {
  _Unwind_Exception *ex =
      (_Unwind_Exception *)malloc(sizeof(_Unwind_Exception));
  memset(ex, '\0', sizeof(_Unwind_Exception));
  memcpy(&ex->exception_class, "CLNGTST", 8);
  ex->exception_cleanup = 0;
  return ex;
}

_Unwind_Reason_Code forced_unwind_callback(int version,
#if defined(__clang__)
                                           _Unwind_Action actions,
#else
                                           int actions,
#endif
                                           _Unwind_Exception_Class klass,
                                           _Unwind_Exception* ex,
                                           struct _Unwind_Context* ctx,
                                           void* userdata) {
  if (actions & _UA_END_OF_STACK) {
    assert(0 && "should reach success exit destructor before end-of-stack");
    abort();
  }
  return _URC_NO_REASON;
}

void run_forced_unwind() {
  _Unwind_ForcedUnwind(test_allocate_exception(), forced_unwind_callback, 0);
  abort();
}

void test3() {
  try {
    run_forced_unwind();
  } catch (void *p) {
    assert(0 && "force unwind should not be caught by void *");
  }
  abort();
}

void test2() {
  try {
    test3();
  } catch (...) {
    ++num_caughts;
    throw;
  }
  abort();
}

void test1() {
  Cleanup c;
  test2();
  abort();
}

int main() {
  // Setup the success exit.
  SuccessExit s;

  // Run the test.
  test1();
  abort();
}

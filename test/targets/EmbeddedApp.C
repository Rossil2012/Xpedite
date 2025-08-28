///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite target app to test embedded profiling from process context
//
// This app intializes framework and profiling session standalone to collect profile data.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Framework.H>  // Include xpedite framework for initialization and profiling
#include <xpedite/framework/Probes.H>     // Include xpedite probe macros for instrumentation
#include "../util/Args.H"                 // Include argument parsing utilities
#include <stdexcept>                      // Include standard exception classes
#include <cstdlib>                        // Include C standard library functions
#include <sys/mman.h>                     // Include memory mapping functions
#include <unistd.h>                       // Include POSIX API functions (usleep)

// Simple function foo with probe scope for profiling
void foo() {
  XPEDITE_PROBE_SCOPE(Foo);                      // Automatic probe at function entry and exit
}

// Simple function bar with probe scope for profiling
void bar() {
  XPEDITE_PROBE_SCOPE(Bar);                      // Automatic probe at function entry and exit
}

// Simple function baz with probe scope for profiling
void baz() {
  XPEDITE_PROBE_SCOPE(Baz);                      // Automatic probe at function entry and exit
}

// Main function - demonstrates embedded profiling with custom configuration
// Parameters: argc_ - command line argument count, argv_ - command line arguments
// Returns: 0 on successful completion
int main(int argc_, char** argv_) {
  auto args = parseArgs(argc_, argv_);            // Parse command line arguments

  using namespace xpedite::framework;            // Import xpedite framework namespace
  // Initialize xpedite framework with remote profiling disabled (embedded mode)
  if(!xpedite::framework::initialize("xpedite-appinfo.txt", {DISABLE_REMOTE_PROFILING})) { 
    throw std::runtime_error {"failed to init xpedite"};  // Throw error if initialization fails
  }

  // Configure profiling session with specific probes and PMU settings
  xpedite::framework::ProfileInfo profileInfo {
    {"TxnBegin", "TxnEnd", "FooBegin", "FooEnd"},  // List of probes to monitor
    PMUCtlRequest {                               // Performance monitoring unit configuration
      ._cpu = 0, ._fixedEvtCount = 1, ._gpEvtCount = 0, ._offcoreEvtCount = 0,  // Event counts
      ._fixedEvents = {                           // Fixed performance events
        PMUFixedEvent {._ctrIndex = 0, ._user = 1, ._kernel = 1}  // Monitor user and kernel events
      },
      ._gpEvents = {},                            // No general purpose events
      ._offcoreEvents = {}                        // No off-core events
    },
    400000                                        // Sample collection limit
  };

  auto guard = xpedite::framework::profile(profileInfo);  // Start profiling session with RAII guard

  std::cout << "Begin profile" << std::endl;     // Log start of profiling
  for(int i=0; i<args.txnCount; ++i) {            // Loop for specified number of transactions
    XPEDITE_TXN_SCOPE(Txn);                      // Begin transaction scope
    foo();                                        // Call profiled function foo
    bar();                                        // Call profiled function bar
    baz();                                        // Call profiled function baz
    if(i % 100 == 0) {                            // Every 100th iteration
      usleep(100000);                             // Sleep for 100ms to simulate work
    }
  }
  std::cout << "End profile" << std::endl;       // Log end of profiling
  return 0;                                       // Return success
}

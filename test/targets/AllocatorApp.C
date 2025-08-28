///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite target app to test memory allocation intercept functionality
//
// This app allocates memory using a variety of methods in each transaction
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Framework.H>  // Include xpedite framework for initialization
#include <xpedite/framework/Probes.H>     // Include xpedite probe macros for instrumentation
#include "../util/Args.H"                 // Include argument parsing utilities
#include <stdexcept>                      // Include standard exception classes
#include <cstdlib>                        // Include C standard library for malloc/free functions
#include <sys/mman.h>                     // Include memory mapping functions (mmap/munmap)
#include <unistd.h>                       // Include POSIX API functions (getpagesize)

// Main function - tests various memory allocation methods within xpedite transactions
// Parameters: argc_ - command line argument count, argv_ - command line arguments
// Returns: 0 on successful completion
int main(int argc_, char** argv_) {

  using namespace xpedite::framework;            // Import xpedite framework namespace
  // Initialize xpedite framework and wait for profiler to begin profiling
  if(!xpedite::framework::initialize("xpedite-appinfo.txt", {AWAIT_PROFILE_BEGIN})) { 
    throw std::runtime_error {"failed to init xpedite"};  // Throw error if initialization fails
  }

  auto args = parseArgs(argc_, argv_);            // Parse command line arguments for test configuration

  using Type = int;                               // Type alias for the data type being allocated
  using Pointer = int*;                           // Type alias for pointer to the data type
  constexpr int ALIGNMENT = 2048;                 // Alignment requirement for aligned allocation

  Pointer ptr;                                    // Pointer variable for allocation operations
  for(int i=0; i<args.txnCount; ++i) {            // Loop for specified number of transactions
    XPEDITE_TXN_SCOPE(Allocation);                // Begin transaction scope to measure allocation overhead

    ptr = new Type {};                            // Test C++ new operator allocation
    delete ptr;                                   // Free memory allocated with new

    ptr = new Type[4] {};                         // Test C++ array new operator allocation
    delete[] ptr;                                 // Free array memory with delete[]

    if((ptr = static_cast<Pointer>(malloc(sizeof(Type))))) {  // Test C malloc allocation
      free(ptr);                                  // Free malloc allocated memory
    }

    if((ptr = static_cast<Pointer>(calloc(1, sizeof(Type))))) {  // Test C calloc allocation
      if((ptr = static_cast<Pointer>(realloc(ptr, 2*sizeof(Type))))) {  // Test realloc resize
        free(ptr);                                // Free reallocated memory
      }
    }

    // Test POSIX aligned memory allocation
    if(!posix_memalign(reinterpret_cast<void**>(&ptr), ALIGNMENT, sizeof(Type))) {
      free(ptr);                                  // Free aligned allocated memory
    }

    auto size = getpagesize();                    // Get system page size for mmap allocation
    // Test memory mapping allocation with read/write permissions and anonymous mapping
    if((ptr = static_cast<Pointer>(mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0))) != MAP_FAILED) {
      munmap(ptr, size);                          // Unmap the memory region
    }
  }                                               // End of transaction scope
  return 0;                                       // Return success
}

///////////////////////////////////////////////////////////////////////////////
//
// A collection of data and utility methods to
//
//   1. Convert hex ascii byte pairs to 8 bit numbers
//   2. Pin threads to a given cpu core
//   3. List regular files at a given file system path
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include<xpedite/util/Util.H>      // Include main utility header with function declarations
#include <xpedite/util/Errno.H>   // Include error handling utilities for system error reporting
#include<thread>                  // Include C++ thread support for native handle types
#include <sched.h>                // Include POSIX scheduler for CPU set operations
#include <dirent.h>               // Include directory operations for file listing functionality

namespace xpedite { namespace util {  // Begin xpedite::util namespace for utility functions

  // Lookup table for fast ASCII hex character to integer conversion
  // Array size is 256 (2^8) to cover all possible byte values (0-255)
  // Valid hex digits: '0'-'9' map to 0-9, 'A'-'F' and 'a'-'f' map to 10-15
  // Invalid characters map to 16 (out of range) to indicate conversion failure
  uint8_t atoiTable[1 << 8] {
    // Characters 0-31 (control chars): all invalid, set to 16
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    // Characters 32-47 (space, punctuation): all invalid, set to 16
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    // Characters 48-57 ('0'-'9'): valid digits 0-9, chars 58-63 (':' to '?'): invalid
    16, 16, 16, 16, 16, 16, 16, 16, +0, +1, +2, +3, +4, +5, +6, +7, +8, +9, 16, 16,
    // Characters 64 ('@'), then 65-70 ('A'-'F'): valid hex digits 10-15, rest invalid
    16, 16, 16, 16, 16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    // Characters 80-95: all invalid, then 96 ('`'), 97-102 ('a'-'f'): valid hex 10-15
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 10, 11, 12,
    // Characters 97-102 ('a'-'f'): valid hex digits 10-15, rest invalid
    13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    // Characters 128-147: all invalid (extended ASCII)
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    // Characters 148-167: all invalid
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    // Characters 168-187: all invalid
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    // Characters 188-207: all invalid
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    // Characters 208-227: all invalid
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    // Characters 228-247: all invalid
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    // Characters 248-255: all invalid
    16, 16, 16, 16, 16, 16, 16, 16
  };

  // Function to pin a thread to a specific CPU core for performance optimization
  // Parameters: handle_ - native thread handle, core_ - target CPU core number
  void pinThread(std::thread::native_handle_type handle_, unsigned core_) {
    cpu_set_t cpuset;                              // CPU set structure to specify allowed cores
    CPU_ZERO(&cpuset);                             // Clear all CPUs from the set (initialize to empty)
    CPU_SET(core_, &cpuset);                       // Add the target core to the CPU set
    int rc = pthread_setaffinity_np(handle_, sizeof(cpu_set_t), &cpuset);  // Set thread affinity
    if(rc != 0) {                                  // Check if pthread_setaffinity_np failed
      std::string errMsg;                          // String to hold human-readable error message
      switch (rc) {                                // Translate error codes to descriptive messages
        case EFAULT:                               // Invalid memory address error
          errMsg = "A supplied memory address was invalid";
          break;
        case EINVAL:                               // Invalid argument error (bad core number)
          errMsg = "supplied core was invalid";
          break;
        case ESRCH:                                // No such process error (thread not found)
          errMsg = "thread not alive";
          break;
        default:                                   // Any other unexpected error code
          errMsg = "unknown error";
          break;
      }
      std::ostringstream stream;                   // String stream for building error message
      stream << "xpedite - failed to pin thread [pthread_setaffinity_np error - " << rc << " | " << errMsg << "]";
      XpediteLogInfo << stream.str()<< XpediteLogEnd;  // Log the error using xpedite logging system
      throw std::runtime_error {stream.str()};    // Throw runtime exception with detailed error
    }
  }

  // Function to list all regular files in a directory
  // Parameter: path_ - null-terminated string path to directory to scan
  // Returns: vector of strings containing filenames (not full paths)
  std::vector<std::string> listFiles(const char* path_) {
    std::vector<std::string> files;               // Vector to store discovered filenames
    if(DIR* dir = opendir(path_)) {               // Open directory for reading, check if successful
      while(auto* entry = readdir(dir)) {         // Read each directory entry until end of stream
        if(entry->d_type == DT_REG) {             // Check if entry is a regular file (not dir/link/etc)
          files.emplace_back(entry->d_name);      // Add filename to results vector (construct in-place)
        }
      }
      closedir(dir);                              // Close directory handle to free resources
    } else {                                      // Directory open failed
      util::Errno e;                              // Capture current errno for error reporting
      std::ostringstream stream;                  // String stream for building error message
      stream << "Failed to list dir \"" << path_ << "\" - " << e.asString();  // Format error message
      XpediteLogInfo << stream.str()<< XpediteLogEnd;  // Log error using xpedite logging system
      throw std::runtime_error {stream.str()};   // Throw exception with descriptive error message
    }
    return files;                                 // Return vector containing all found filenames
  }

}}  // End namespace xpedite::util

///////////////////////////////////////////////////////////////////////////////
//
// A utility class to control storage for xpedite samples data
//
// The storage manager keeps track of current memory/file system consumption.
// It also provides methods to build file system paths for different data files
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include "StorageMgr.H"                 // Include storage manager class declaration
#include <xpedite/util/Util.H>          // Include utility functions for process name and file operations
#include <xpedite/util/Errno.H>         // Include error handling utilities
#include <sstream>                      // Include string stream for path building
#include <cstring>                      // Include C string functions (strlen)
#include <cstdio>                       // Include C standard I/O functions (remove)

namespace xpedite { namespace framework {  // Begin xpedite::framework namespace

  // Directory path for storing samples files (shared memory for performance)
  const char* SAMPLES_DIR_PATH {"/dev/shm/"};

  // File extension for xpedite samples data files
  const char* SAMPLES_FILE_SUFFIX {".data"};

  // Precomputed length of samples file suffix for efficient string operations
  size_t SAMPLES_FILE_SUFFIX_LEN {strlen(SAMPLES_FILE_SUFFIX)};

  // Build unique prefix for samples files based on process name
  // Returns: string containing "xpedite-<processname>" prefix
  std::string StorageMgr::buildSamplesFilePrefix() {
    std::ostringstream stream;                    // String stream for building prefix
    stream << "xpedite-" << util::getProcessName();  // Combine xpedite prefix with process name
    return stream.str();                          // Return constructed prefix string
  }

  // Build template path for samples files including timestamp and wildcard
  // Returns: string template for samples file paths (used for temporary file creation)
  std::string StorageMgr::buildSamplesFileTemplate() {
    std::ostringstream stream;                    // String stream for building template
    stream << SAMPLES_DIR_PATH << buildSamplesFilePrefix()  // Start with directory and prefix
      << "-"  << time(nullptr) << "-*" << SAMPLES_FILE_SUFFIX;  // Add timestamp, wildcard, suffix
    return stream.str();                          // Return constructed template string
  }

  // Clean up old samples files from the samples directory
  // Removes all files matching the current process's prefix and suffix pattern
  void StorageMgr::reset() {
    auto filePrefix = buildSamplesFilePrefix();   // Get prefix for current process
    auto files = util::listFiles(SAMPLES_DIR_PATH);  // List all files in samples directory
    int fileCount {};                             // Counter for files removed
    std::ostringstream stream;                    // Stream for building log message
    stream << "Xpedite purging old sample files ";  // Start log message
    for(auto& file : files) {                     // Iterate through all files in directory
      if(file.find(filePrefix) == 0 &&           // Check if filename starts with our prefix
        file.rfind(SAMPLES_FILE_SUFFIX) == file.size() - SAMPLES_FILE_SUFFIX_LEN) {  // And ends with suffix
        auto path = SAMPLES_DIR_PATH + file;      // Build full file path
        stream << "\n\t->\t " << path;            // Add file to log message
        if(remove(path.c_str())) {                // Attempt to remove the file
          util::Errno e;                          // Capture errno if removal failed
          stream << " - [" << e.asString() << "]";  // Log error details
        } else {
          stream << " - [DELETED]";               // Log successful deletion
        }
        ++fileCount;                              // Increment counter of processed files
      }
    }
   XpediteLogInfo << stream.str() << "\nremoved " << fileCount  // Log summary of cleanup operation
     << " out of " << files.size() << " file(s)" << XpediteLogEnd;
  }

}}  // End namespace xpedite::framework

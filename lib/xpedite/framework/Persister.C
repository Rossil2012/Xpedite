///////////////////////////////////////////////////////////////////////////////
//
// Methods to persist timing and pmc data to filesystem
// The records can be encoded in one these three formats (text, csv, binary) 
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Persister.H>  // Include persister class declaration
#include <xpedite/probes/Config.H>        // Include probe configuration settings
#include <xpedite/probes/ProbeList.H>     // Include global probe list management
#include <xpedite/probes/Sample.H>        // Include sample data structures
#include <xpedite/util/Util.H>            // Include utility functions for logging
#include <xpedite/util/Tsc.H>             // Include timestamp counter utilities
#include <xpedite/pmu/PMUCtl.H>           // Include performance monitoring unit control
#include <sys/time.h>                     // Include time functions (gettimeofday)
#include <cstdint>                        // Include fixed-width integer types
#include <cstdio>                         // Include C standard I/O (write)
#include <cstring>                        // Include C string functions (strlen)
#include <memory>                         // Include smart pointer utilities

namespace xpedite { namespace framework {  // Begin xpedite::framework namespace

  // Static counter for tracking number of persisted data segments
  static unsigned batchCount;

  // Resize internal buffer to accommodate new object of specified size
  // Parameter: objSize_ - size of object that needs to be added to buffer
  void Persister::resizeBuffer(size_t objSize_) {
    auto size = _buffer.size();                   // Get current buffer size
    while(size < _hdr->size() + objSize_) {       // Check if current size is insufficient
      size *= 2;                                  // Double the buffer size
    }
    _buffer.resize(size);                         // Resize the buffer to new size
    _hdr = reinterpret_cast<FileHeader*>(_buffer.data());  // Update header pointer after resize
  }

  // Constructor - initialize persister with file header and probe information
  Persister::Persister()
    : _hdr {}, _buffer {} {                       // Initialize member variables
    auto tscHz = util::estimateTscHz();           // Estimate timestamp counter frequency
    timeval  time;                                // Structure to hold current time
    gettimeofday(&time, nullptr);                // Get current time for header
    _buffer.resize(2 * 1024 * 1024);              // Initialize buffer to 2MB size
    _hdr = new (_buffer.data()) FileHeader {time, tscHz, pmu::pmuCtl().pmcCount()};  // Construct header in buffer
    for(auto& probe : probes::probeList()) {     // Iterate through all registered probes
      Name probeName    {probe.name(), static_cast<uint32_t>(strlen(probe.name()))+1};    // Create name object for probe
      Name fileName     {probe.file(), static_cast<uint32_t>(strlen(probe.file()))+1};    // Create name object for file
      Name functionName {probe.func(), static_cast<uint32_t>(strlen(probe.func()))+1};    // Create name object for function
      auto objSize =  sizeof(ProbeInfo) + probeName._size + fileName._size + functionName._size;  // Calculate total size needed
      if(freeSize() < objSize) {                  // Check if buffer has enough free space
        resizeBuffer(objSize);                    // Resize buffer if needed
      }
      _hdr->add(                                  // Add probe information to header
        probe.rawRecorderCallSite(), probe.attr(), probe.id(), probeName, fileName, functionName, probe.line()
      );
    }
  }

  // Write file header to a file descriptor
  // Parameter: fd_ - file descriptor to write header to
  void Persister::persistHeader(int fd_) {
    write(fd_, _buffer.data(), _hdr->size());     // Write header buffer to file
    XpediteLogInfo << "persisted file header with " << _hdr->probeCount() << " call sites  | capacity "
      << _hdr->size() << " bytes" << XpediteLogEnd;  // Log header persistence details
  }

  // Write sample data segment to a file descriptor
  // Parameters: fd_ - file descriptor, begin_ - start of sample data, end_ - end of sample data
  void Persister::persistData(int fd_, const probes::Sample* begin_, const probes::Sample* end_) {

    if(!begin_ || begin_ == end_) {               // Check if there's valid data to persist
      return;                                     // Return early if no data
    }
    uint64_t ccstart {RDTSC()};                   // Record start timestamp for performance measurement
    timeval  time;                                // Structure to hold current time
    gettimeofday(&time, nullptr);                // Get current time for segment header
    unsigned size = reinterpret_cast<const char*>(end_) - reinterpret_cast<const char*>(begin_);  // Calculate data size

    SegmentHeader segmentHeader{time, size, ++batchCount};  // Create segment header with metadata
    write(fd_, &segmentHeader, sizeof(segmentHeader));      // Write segment header to file
    write(fd_, begin_, size);                     // Write actual sample data to file
    if(probes::config().verbose()) {              // Check if verbose logging is enabled
      XpediteLogInfo << "persisted segment " << size << " bytes in " << RDTSC() - ccstart << " cycles" << XpediteLogEnd;  // Log persistence performance
    }
  }

}}  // End namespace xpedite::framework

///////////////////////////////////////////////////////////////////////////////
//
// Global static definitions for Per thread probe sample buffers
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/platform/Builtins.H>   // Include platform-specific built-in definitions and macros
#include <xpedite/framework/SamplesBuffer.H>  // Include samples buffer class declaration
#include <xpedite/util/Util.H>           // Include utility functions (like gettid)

// Thread-local storage pointer to the current thread's samples buffer
// Each thread gets its own independent buffer for storing profiling samples
static __thread xpedite::framework::SamplesBuffer* _tlSamplesBuffer;

// Thread-local pointer to current write position in samples buffer
// This is used by probes to quickly write sample data without function calls
__thread xpedite::probes::Sample* samplesBufferPtr;

// Thread-local pointer to end of current writable range in samples buffer
// Used to detect when buffer needs expansion to accommodate more samples
__thread xpedite::probes::Sample* samplesBufferEnd;

namespace xpedite { namespace framework {  // Begin xpedite::framework namespace

  // Static member: atomic pointer to head of linked list of all samples buffers
  // Aligned to cache line boundary to prevent false sharing between CPU cores
  alignas(common::ALIGNMENT) std::atomic<SamplesBuffer*> SamplesBuffer::_head {};

  // Check if the current thread has initialized its samples buffer
  // Returns: true if thread-local buffer is allocated and ready for use
  bool SamplesBuffer::isInitialized() {
    return _tlSamplesBuffer != nullptr;           // Simply check if thread-local pointer is set
  }

  // Get or create the samples buffer for the current thread
  // Returns: pointer to thread's samples buffer (guaranteed non-null)
  SamplesBuffer* SamplesBuffer::samplesBuffer() {
    if(XPEDITE_UNLIKELY(!_tlSamplesBuffer)) {     // Check if buffer not yet allocated (unlikely path)
      _tlSamplesBuffer = SamplesBuffer::allocate();  // Allocate new buffer for this thread
    }
    return _tlSamplesBuffer;                      // Return thread's buffer pointer
  }

  // Expand the writable range of the current thread's samples buffer
  // Called when current buffer segment is full and more space is needed
  void SamplesBuffer::expand() {
    if(probes::config().verbose()) {              // Check if verbose logging is enabled
      XpediteLogInfo << "Xpedite SamplesBuffer expand: tid - " << util::gettid() << " | begin - " << samplesBufferPtr
        << " | end - " << samplesBufferEnd << XpediteLogEnd;  // Log buffer expansion details
    }
    // Get next writable range from buffer and update thread-local pointers
    std::tie(samplesBufferPtr, samplesBufferEnd) = samplesBuffer()->nextWritableRange();
  }

}}  // End namespace xpedite::framework


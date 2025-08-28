///////////////////////////////////////////////////////////////////////////////
//
// Global static definitions for logger
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <ios>                            // Include I/O stream base classes
#include <iomanip>                        // Include I/O manipulators for formatting output
#include <unistd.h>                       // Include POSIX operating system API
#include <xpedite/log/Log.H>              // Include logging system declarations
#include <xpedite/probes/Probe.H>         // Include probe class definition
#include <xpedite/probes/ProbeList.H>     // Include probe list container definition

namespace xpedite { namespace log {       // Begin xpedite::log namespace

  // Global counter for tracking number of log messages generated
  uint64_t logCounter = 0;

  // Log detailed information about a probe to a specific output stream
  // Parameters: logfile_ - output stream, probe_ - probe to log, action_ - optional action description
  void logProbe(std::ostream& logfile_, const probes::Probe& probe_, const char* action_) {
    if(action_) {                                 // Check if action description is provided
      logfile_ << "Action=" << action_ << " | ";  // Write action prefix to log
    }

    logfile_ << "Id="                             // Write probe ID with zero-padded formatting
      << std::setfill('0') << std::setw(4) << probe_.id() << std::setfill(' ') 
      << " | Probe=" << &probe_                   // Write probe object address
      << " | CallSite=" << std::hex << reinterpret_cast<const void*>(probe_.rawCallSite()) << std::dec  // Write call site address in hex
      << " | RecorderReturnSite=" << std::hex << reinterpret_cast<const void*>(probe_.recorderReturnSite()) << std::dec  // Write return address in hex
      << " | Status=" << (probe_.isActive() ? "enabled" : "disabled")  // Write probe status
      << " | Name=" << probe_.name()              // Write probe name
      << " | File=" << probe_.file()              // Write source file name
      << " | Line=" << probe_.line()              // Write line number
      << " | Function=" << probe_.func()          // Write function name
      << " | Attributes=" << probe_.attr().toString() << std::endl;  // Write probe attributes and end line
  }

  // Log probe information to standard output (convenience wrapper)
  // Parameters: probe_ - probe to log, action_ - optional action description
  void logProbe(const probes::Probe& probe_, const char* action_) {
    logProbe(std::cout, probe_, action_);         // Delegate to stream version using std::cout
  }

  // Log information about all probes in a probe list to a specific output stream
  // Parameters: logfile_ - output stream, probeList_ - list of probes to log
  void logProbes(std::ostream& logfile_, const probes::ProbeList& probeList_) {
    for(auto& probe : probeList_) {               // Iterate through all probes in the list
      log::logProbe(logfile_, probe);             // Log each probe without action description
    }
  }

  // Log all probes in a probe list to standard output (convenience wrapper)
  // Parameters: probeList_ - list of probes to log
  void logProbes(const probes::ProbeList& probeList_) {
    logProbes(std::cout, probeList_);             // Delegate to stream version using std::cout
  }
}}  // End namespace xpedite::log

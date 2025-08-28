///////////////////////////////////////////////////////////////////////////////
//
// Probes - Probes with near zero overhead, that can be activated at runtime
//
// Provides a collection of methods to
//   1. Lazy initialize thread sample buffers
//   2. Logic to locate, enable and disable probes
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/probes/Config.H>       // Include probe configuration settings
#include <xpedite/probes/ProbeCtl.H>     // Include probe control function declarations
#include <xpedite/probes/ProbeList.H>    // Include probe list management functions
#include <xpedite/util/Util.H>           // Include utility functions for logging
#include <xpedite/util/AddressSpace.H>   // Include address space management for memory protection
#include <set>                           // Include STL set container for unique segment collection

namespace xpedite { namespace probes {  // Begin xpedite::probes namespace

  // Main probe control function - enables, disables, or reports on probes
  // Parameters: cmd_ - command to execute, file_ - source file filter, line_ - line filter, name_ - probe name filter
  void probeCtl(Command cmd_, const char* file_, int line_, const char *name_) {
    util::AddressSpace& asp (util::addressSpace());  // Get reference to address space manager
    std::set<util::AddressSpace::Segment*> segments; // Set to collect unique memory segments for modification

    switch (cmd_) {                               // Switch on the requested command
    case Command::ENABLE:                         // Enable matching probes
    case Command::DISABLE:                        // Disable matching probes

      // First pass: collect all memory segments containing matching probes
      for(auto& probe : probeList()) {            // Iterate through all registered probes
        if(probe.match(file_, line_, name_)) {    // Check if probe matches filter criteria
          segments.emplace(asp.find(probe.rawCallSite()));  // Add segment containing probe to set
        }
      }

      // Make memory segments writable before modifying probe instructions
      for(auto* segment : segments) {             // Iterate through collected segments
        if(segment)                               // Check if segment pointer is valid
          segment->makeWritable();                // Remove write protection from memory segment
      }

      // Second pass: actually enable or disable the matching probes
      for(auto& probe : probeList()) {            // Iterate through all probes again
        if(probe.match(file_, line_, name_)) {    // Check if probe matches filter criteria
          if(config().verbose())                  // Check if verbose logging is enabled
            log::logProbe(probe, (cmd_ == Command::ENABLE) ? "Probe Enable" : "Probe Disable");  // Log action
          if(cmd_ == Command::ENABLE)             // Check if enabling probe
            probe.activate();                     // Activate the probe (patch in jump instruction)
          else
            probe.deactivate();                   // Deactivate the probe (restore original instruction)
        }
      }

      // Restore original memory protections after modification
      for(auto segment : segments) {              // Iterate through modified segments
        if(segment)                               // Check if segment pointer is valid
          segment->restoreProtections();          // Restore original memory protection flags
      }
      break;
    case Command::REPORT:                         // Report status of matching probes
      for(auto& probe : probeList()) {            // Iterate through all registered probes
        if(probe.match(file_, line_, name_)) {    // Check if probe matches filter criteria
          log::logProbe(probe, "Probe ");         // Log probe information and status
        }
      }
      break;
    default:                                      // Handle unknown command
      XpediteLogError << "probeCtl unknown cmd \" " << static_cast<int>(cmd_) << "\"" << XpediteLogEnd;  // Log error
      break;
    }
  }

}}  // End namespace xpedite::probes


////////////////////////////////////////////////////////////////////////////////////
//
// SamplesLoader loads probe sample data from binary files
//
// Xpedite probes store timing and performance counter data using variable 
// length POD objects. A collection of sample objects is grouped and written
// as a batch. 
//
// The loader iterates through the POD collection,  to extract 
// records in string format for consumption by the profiler
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/SamplesLoader.H>  // Include samples loader class declaration
#include <fstream>                            // Include file stream operations for CSV output
#include <stdexcept>                          // Include standard exception classes
#include <sstream>                            // Include string stream for error message formatting
#include <iomanip>                            // Include I/O manipulators for formatting hex output
#include <ios>                                // Include I/O stream base classes and flags

namespace xpedite { namespace framework {  // Begin xpedite::framework namespace

  // Static method to save samples data from binary file to CSV file
  // Parameters: samplesPath_ - path to binary samples file, dest_ - destination CSV file path
  // Returns: number of samples processed and written to CSV
  int SamplesLoader::saveAsCsv(const char* samplesPath_, const char* dest_) {
    using namespace xpedite::probes;             // Import probes namespace for Sample types
    using namespace xpedite::framework;          // Import framework namespace 

    std::ofstream destStream;                    // Output file stream for writing CSV data
    try {
      destStream.open(dest_, std::ios_base::out); // Open destination file for writing
    }
    catch(std::ios_base::failure& e) {           // Catch file opening errors
      std::ostringstream stream;                 // String stream for error message formatting
      stream << "xpedite failed to open log " << dest_ << " for writing - " << e.what();  // Format error
      throw std::runtime_error {stream.str()};  // Throw runtime error with descriptive message
    }
    return streamAsCsv(samplesPath_, destStream); // Delegate to streaming method and return count
  }

  // Static method to stream samples data from binary file to any output stream as CSV
  // Parameters: samplesPath_ - path to binary samples file, destStream_ - output stream for CSV data
  // Returns: number of samples processed and written
  int SamplesLoader::streamAsCsv(const char* samplesPath_, std::ostream& destStream_) {
    using namespace xpedite::probes;             // Import probes namespace for Sample types
    using namespace xpedite::framework;          // Import framework namespace

    SamplesLoader loader {samplesPath_};         // Create loader instance for the samples file
    auto pmcCount = loader.pmcCount();           // Get number of performance counters in samples
    destStream_ << "Tsc,ReturnSite,Data";        // Write CSV header with basic columns
    for(unsigned i=0; i<pmcCount; ++i) {         // Add header columns for each performance counter
      destStream_ << ",Pmc-" << i+1;             // PMC columns numbered 1-based for readability
    }
    destStream_ << std::endl;                    // End header line

    int count {};                                // Counter for number of samples processed
    for(auto& sample : loader) {                 // Iterate through all samples in the file
      destStream_ << std::hex << sample.tsc() << std::dec << "," << sample.returnSite();  // Write TSC and return site
      if (sample.hasData()) {                    // Check if sample contains data payload
        // Write data as hex: upper 64 bits first, then lower 64 bits with padding
        destStream_ << std::hex << "," << std::get<1>(sample.data()) << std::setw(16) << std::setfill('0') 
          << std::right << std::get<0>(sample.data()) << std::dec;
      }
      else {
        destStream_ << ",";                      // Empty data column if no data present
      }

      if (sample.hasPmc()) {                     // Check if sample contains performance counter data
        const uint64_t* v; int c;                // Variables for PMC data pointer and count
        std::tie(v, c) = sample.pmc();           // Extract PMC data array and count
        for(int i=0; i<c; ++i) {                 // Write each performance counter value
          destStream_ << "," << v[i];            // Comma-separated PMC values
        }
      }
      destStream_ << std::endl;                  // End sample line
      ++count;                                   // Increment processed sample count
    }
    return count;                                // Return total number of samples processed
  }

}}  // End namespace xpedite::framework

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

#include <xpedite/framework/SamplesLoader.H>  // Include samples loader class for binary data parsing
#include <iostream>                           // Include standard I/O stream for output

// Main function - command line utility to convert binary samples to CSV format
// Parameters: argc_ - command line argument count, argv_ - command line arguments
// Returns: 0 on success, 1 on error
int main(int argc_, char** argv_) {
  if(argc_ <2) {                              // Check if samples file path was provided
    std::cerr << "[usage]: " << argv_[0] << " <samples-file>" << std::endl;  // Print usage message
    exit(1);                                  // Exit with error code
  }
  // Load binary samples file and stream contents to stdout as CSV
  xpedite::framework::SamplesLoader::streamAsCsv(argv_[1], std::cout);
  return 0;                                   // Return success
}

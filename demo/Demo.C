///////////////////////////////////////////////////////////////////////////////
//
// A stand alone program, demonstrating instrumentation and profiling with Xpedite
// The program can be used to profile latency of random memory acccess vs 
// accessing contiguous memory regions
//
// The program accepts the following command line arguments
//  -m Creates multiple threads for profiling
//  -r Run the random access test
//  -t Transaction count, number of memory accesses done by the program
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>         // Include for standard output stream operations
#include <stdexcept>        // Include for standard exception classes (invalid_argument)
#include <unistd.h>         // Include for getopt command line parsing function
#include<thread>            // Include for C++ threading support
#include "Demo.H"           // Include demo-specific headers and function declarations

// Global configuration variables for demo program
int  cpu {0};               // CPU core number to pin threads to (default: 0)
bool multiThreaded {};      // Flag indicating whether to run multi-threaded demo (default: false)
int txnCount {100};         // Number of transactions/memory accesses to perform (default: 100)
bool randomize {};          // Flag for random vs sequential memory access pattern (default: false)
bool pinMemory {};          // Flag to enable memory pinning for performance (default: false)

// Function to parse command line arguments and set global configuration
// Parameters: argc_ - number of command line arguments, argv_ - array of argument strings
void parseArgs(int argc_, char** argv_) {
  int arg;                                        // Variable to store current argument character
  while ((arg = getopt (argc_, argv_, "mrt:c:")) != -1) {  // Parse options until no more arguments
    switch (arg) {                                // Handle each command line option
    case 'm':                                     // Multi-threaded mode option
      multiThreaded = true;                       // Enable multi-threaded execution
      break;
    case 'r':                                     // Random access pattern option
      randomize = true;                           // Enable random memory access instead of sequential
      break;
    case 't':                                     // Transaction count option (requires value)
      txnCount = std::stoi(optarg);               // Convert string argument to integer transaction count
      break;
    case 'c':                                     // CPU core option (requires value)
      cpu = std::stoi(optarg);                    // Convert string argument to CPU core number
      break;
    case 'l':                                     // Memory pinning option
      pinMemory = true;                           // Enable memory pinning for better performance
      break;
    case '?':                                     // Unknown/invalid option
    default:                                      // Default case for any unhandled options
      std::cerr << argv_[0] << " [-c <cpu>] [-t <txn count>] [-r] [-m]" << std::endl;  // Print usage
      throw std::invalid_argument{"Invalid argument"};  // Throw exception for invalid arguments
    }
  }
}

// Main function - entry point for the xpedite demo application
// Parameters: argc_ - command line argument count, argv_ - array of command line argument strings
// Returns: integer exit code (0 for success)
int main(int argc_, char** argv_) {
  using namespace xpedite::demo;                  // Import xpedite demo namespace to access demo functions
  parseArgs(argc_, argv_);                        // Parse command line arguments and set global config
  std::cout                                       // Print program banner with current configuration
    << "\n========================================================================================\n"
    << " \txpedite " << (multiThreaded ? "Multi thread " : "") << "demo [txnCount - " << txnCount 
    << " | randomization - " << (randomize ? "enabled" : "disabled") 
    << " | cpu - " << cpu << "]" 
    << " | pinMemory - " << (pinMemory ? "enabled" : "disabled") << "]" 
    << "\n========================================================================================\n\n";
  initialize(pinMemory);                          // Initialize xpedite framework with memory pinning config
  if(multiThreaded) {                             // Execute multi-threaded version if requested
    int trc;                                      // Variable to store return code from second thread
    std::thread t {[&trc]() {trc = runDemo(txnCount, randomize, cpu);}};  // Create worker thread
    auto rc = runDemo(txnCount, randomize, cpu);  // Run demo on main thread with same parameters
    t.join();                                     // Wait for worker thread to complete
    return  rc + trc;                             // Return sum of both thread return codes
  }
  return runDemo(txnCount, randomize, cpu);       // Run single-threaded demo and return its exit code
}

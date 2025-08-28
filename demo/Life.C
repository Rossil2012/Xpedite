///////////////////////////////////////////////////////////////////////////////
//
// A stand alone program, demonstrating instrumentation and profiling with Xpedite
// The program provides a trivial example for usage of transaction and probe API.
//
// A binary (named life) can be built by invoking the following command
// after substitution of xpedite install directories
//    g++ -pthread -std=c++11 -I <path-to-xpedite-headers> Life.C -o life -L <path-to-xpedite-libs> -lxpedite -ldl -lrt
//
// To generate a profile info file, use geneate command with xpedite, after launching binary
//    xpedite generate -a /tmp/xpedite-appinfo.txt
// 
// To create a profile report, use record command with xpedite, while the binary is running
//    xpedite record -p profileInfo.py
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <stdexcept>                      // Include standard exception classes
#include <iostream>                       // Include standard I/O stream operations
#include <xpedite/framework/Framework.H>  // Include xpedite framework initialization functions
#include <xpedite/framework/Probes.H>     // Include xpedite probe macros and definitions
#include <xpedite/framework/Options.H>    // Include xpedite configuration options

// Simple function to simulate eating activity
void eat()   { std::cout << "eat..."   << std::endl; }

// Simple function to simulate sleeping activity  
void sleep() { std::cout << "sleep..." << std::endl; }

// Simple function to simulate coding activity
void code()  { std::cout << "code..."  << std::endl; }

// Main life simulation function demonstrating xpedite profiling capabilities
// Parameter: timeToLive_ - number of life cycles to execute
void life(int timeToLive_) {
  for(unsigned i=0; i<timeToLive_; ++i) {         // Loop for specified number of life cycles
    XPEDITE_TXN_SCOPE(Life);                      // Begin transaction scope for entire life cycle
    eat();                                        // Execute eating activity

    XPEDITE_PROBE(SleepBegin);                    // Insert probe to mark beginning of sleep activity
    sleep();                                      // Execute sleeping activity

    XPEDITE_PROBE(CodeBegin);                     // Insert probe to mark beginning of coding activity
    code();                                       // Execute coding activity
  }                                               // End of transaction scope (automatic probe inserted)
}

// Main function - entry point for the life simulation demo
// Returns: 0 on successful completion
int main() {
  // Configure xpedite options to wait for profiler connection before starting
  const xpedite::framework::Options options = {xpedite::framework::AWAIT_PROFILE_BEGIN};
  
  // Initialize xpedite framework with app info file path and configuration options
  if(!xpedite::framework::initialize("/tmp/xpedite-appinfo.txt", options)) { 
    throw std::runtime_error {"failed to init xpedite"};  // Throw error if initialization fails
  }
  
  life(100);                                      // Run life simulation for 100 cycles
}

/**
 *      CUDA HLT1
 *      
 *      author  -  GPU working group
 *      e-mail  -  lhcb-parallelization@cern.ch
 *
 *      Original development June, 2014
 *      Restarted development on February, 2018
 *      CERN
 */
#include <iostream>
#include <string>
#include <cstring>
#include <exception>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <unistd.h>
#include "tbb/tbb.h"
#include "cuda_runtime.h"
#include "CudaCommon.h"
#include "Logger.h"
#include "Tools.h"
#include "InputTools.h"
#include "InputReader.h"
#include "Timer.h"
#include "StreamWrapper.cuh"
#include "Constants.cuh"
#include "MuonDefinitions.cuh"
#include "MuonFeatures.h"

void printUsage(char* argv[]){
  std::cerr << "Usage: "
    << argv[0]
    << std::endl << " -f {folder containing bin files with raw bank information}"
    << std::endl << " -u {folder containing bin files with UT raw bank information}"
    << std::endl << " -s {folder containing bin files with SciFi hit information}"
    << std::endl << " -g {folder containing detector configuration}"
    << std::endl << " -d {folder containing .bin files with MC truth information}"
    << std::endl << " -a {folder containnig bin files with Muon raw bank informatiom}"
    << std::endl << " -n {number of events to process}=0 (all)"
    << std::endl << " -o {offset of events from which to start}=0 (beginning)"
    << std::endl << " -t {number of threads / streams}=1"
    << std::endl << " -r {number of repetitions per thread / stream}=1"
    << std::endl << " -c {run checkers}=0"
    << std::endl << " -k {simplified kalman filter}=0"
    << std::endl << " -m {reserve Megabytes}=1024"
    << std::endl << " -v {verbosity}=3 (info)"
    << std::endl << " -p (print memory usage)"
    << std::endl << " -x {run algorithms on x86 architecture as well (if possible)}=0"
    << std::endl;
}

int main(int argc, char *argv[])
{
  std::string folder_name_velopix_raw;
  std::string folder_name_UT_raw = "";
  std::string folder_name_scifi_hits = "";
  std::string folder_name_MC = "";
  std::string folder_name_detector_configuration = "";
  std::string folder_name_muon_hits = "";
  uint number_of_events_requested = 0;
  uint start_event_offset = 0;
  uint tbb_threads = 1;
  uint number_of_repetitions = 1;
  uint verbosity = 3;
  bool print_memory_usage = false;
  // By default, do_check will be true when mc_check is enabled 
  bool do_check = true;
  bool do_simplified_kalman_filter = false;
  bool run_on_x86 = false;
  size_t reserve_mb = 1024;
   
  signed char c;
  while ((c = getopt(argc, argv, "a:f:d:u:s:n:o:t:r:pha:b:d:v:c:k:m:g:x:")) != -1) {
    switch (c) {
    case 'a':
      folder_name_muon_hits = std::string(optarg);
      break;
    case 'f':
      folder_name_velopix_raw = std::string(optarg);
      break;
    case 'd':
      folder_name_MC = std::string(optarg);
      break;
    case 'u':
      folder_name_UT_raw = std::string(optarg);
      break;
    case 's':
      folder_name_scifi_hits = std::string(optarg);
      break;
    case 'g':
      folder_name_detector_configuration = std::string(optarg);
      break;
    case 'm':
      reserve_mb = atoi(optarg);
      break;
    case 'n':
      number_of_events_requested = atoi(optarg);
      break;
    case 'o':
      start_event_offset = atoi(optarg);
      break;
    case 't':
      tbb_threads = atoi(optarg);
      break;
    case 'r':
      number_of_repetitions = atoi(optarg);
      break;
    case 'c':
      do_check = atoi(optarg);
      break;
    case 'k':
      do_simplified_kalman_filter = atoi(optarg);
      break;
    case 'x':
      run_on_x86 = atoi(optarg);
      break;
    case 'v':
      verbosity = atoi(optarg);
      break;
    case 'p':
      print_memory_usage = true;
      break;
    case '?':
    case 'h':
    default:
      printUsage(argv);
      return -1;
    }
  }

  // Options sanity check
  if (folder_name_velopix_raw.empty() || folder_name_UT_raw.empty() ||
    folder_name_detector_configuration.empty() || (folder_name_MC.empty() && do_check)) {
    std::string missing_folder = "";

    if (folder_name_velopix_raw.empty()) missing_folder = "velopix raw events";
    else if (folder_name_UT_raw.empty()) missing_folder = "UT raw events";
    else if (folder_name_scifi_hits.empty()) missing_folder = "scifi hits events";
    else if (folder_name_detector_configuration.empty()) missing_folder = "detector geometry";
    else if (folder_name_MC.empty() && do_check) missing_folder = "Monte Carlo";
    else if (folder_name_muon_hits.empty()) missing_folder = "muon raw events";

    error_cout << "No folder for " << missing_folder << " specified" << std::endl;
    printUsage(argv);
    return -1;
  }

  // Set verbosity level
  std::cout << std::fixed << std::setprecision(2);
  logger::ll.verbosityLevel = verbosity;

  // Get device properties
  cudaDeviceProp device_properties;
  cudaCheck(cudaGetDeviceProperties(&device_properties, 0));
  
  // Show call options
  std::cout << "Requested options:" << std::endl
    << " folder with velopix raw bank input (-f): " << folder_name_velopix_raw << std::endl
    << " folder with UT raw bank input (-u): " << folder_name_UT_raw << std::endl
    << " folder with scifi hits input (-s): " << folder_name_scifi_hits << std::endl
    << " folder with detector configuration (-g): " << folder_name_detector_configuration << std::endl
    << " folder with MC truth input (-d): " << folder_name_MC << std::endl
    << " folder with muon raw events (-a): " << folder_name_muon_hits << std::endl
    << " run checkers (-c): " << do_check << std::endl
    << " number of files (-n): " << number_of_events_requested << std::endl
    << " start event offset (-o): " << start_event_offset << std::endl
    << " tbb threads (-t): " << tbb_threads << std::endl
    << " number of repetitions (-r): " << number_of_repetitions << std::endl
    << " simplified kalman filter (-k): " << do_simplified_kalman_filter << std::endl
    << " reserve MB (-m): " << reserve_mb << std::endl
    << " run algorithms on x86 architecture as well (-x): " << run_on_x86 << std::endl
    << " print memory usage (-p): " << print_memory_usage << std::endl
    << " verbosity (-v): " << verbosity << std::endl
    << " device: " << device_properties.name << std::endl
    << std::endl;

  // Read all inputs
  info_cout << "Reading input datatypes" << std::endl;

   number_of_events_requested = get_number_of_events_requested(
    number_of_events_requested, folder_name_velopix_raw);

  // Read SciFi hits
  std::vector<char> scifi_events;
  std::vector<unsigned int> scifi_event_offsets;
  verbose_cout << "Reading SciFi hits for " << number_of_events_requested << " events " << std::endl;
  read_folder( folder_name_scifi_hits, number_of_events_requested,
               scifi_events, scifi_event_offsets,
               start_event_offset );

  SciFi::HitsSoA *scifi_hits_events = new SciFi::HitsSoA[number_of_events_requested];
  uint32_t scifi_n_hits_layers_events[number_of_events_requested][SciFi::Constants::n_zones];
  read_scifi_events_into_arrays( scifi_hits_events, scifi_n_hits_layers_events,
                              scifi_events, scifi_event_offsets, number_of_events_requested );
  //check_scifi_events( scifi_hits_events, scifi_n_hits_layers_events, number_of_events );

  // Read muon hits
  std::vector<char> muon_events;
  std::vector<unsigned int> muon_event_offsets;
  verbose_cout << "Reading Muon hits for " << number_of_events_requested << " events " << std::endl;
  read_folder( folder_name_muon_hits, number_of_events_requested,
               muon_events, muon_event_offsets, start_event_offset);
  std::vector<Muon::HitsSoA> muon_hits_events(number_of_events_requested);
  read_muon_events_into_arrays(muon_hits_events.data(), muon_events, 
                               muon_event_offsets, number_of_events_requested);

  const int hits_to_out = 3;
  check_muon_events(muon_hits_events.data(), hits_to_out, number_of_events_requested);
  auto muTrack = new State(0,0,1,1,1);
  std::vector<double> features = calcBDT(*muTrack, muon_hits_events[0]);
  for(auto f : features) {
      std::cout << f << " ";
  }
  std::cout<<std::endl;
  delete muTrack;

  auto geometry_reader = GeometryReader(folder_name_detector_configuration);
  auto ut_magnet_tool_reader = UTMagnetToolReader(folder_name_detector_configuration);
  auto velo_reader = VeloReader(folder_name_velopix_raw);
  auto ut_reader = EventReader(folder_name_UT_raw);
  
  
  std::vector<char> velo_geometry = geometry_reader.read_geometry("velo_geometry.bin");
  std::vector<char> ut_boards = geometry_reader.read_geometry("ut_boards.bin");
  std::vector<char> ut_geometry = geometry_reader.read_geometry("ut_geometry.bin");
  std::vector<char> ut_magnet_tool = ut_magnet_tool_reader.read_UT_magnet_tool();
  velo_reader.read_events(number_of_events_requested, start_event_offset);
  ut_reader.read_events(number_of_events_requested, start_event_offset);
  

  info_cout << std::endl << "All input datatypes successfully read" << std::endl << std::endl;
  
  // Initialize detector constants on GPU
  Constants constants;
  constants.reserve_and_initialize();

  // Create streams
  StreamWrapper stream_wrapper;
  stream_wrapper.initialize_streams(
    tbb_threads,
    velo_geometry,
    ut_boards,
    ut_geometry,
    ut_magnet_tool,
    number_of_events_requested,
    do_check,
    do_simplified_kalman_filter,
    print_memory_usage,
    run_on_x86, 
    folder_name_MC,
    start_event_offset,
    reserve_mb,
    constants
  );
  
  // Attempt to execute all in one go
  Timer t;
  tbb::parallel_for(
    static_cast<uint>(0),
    static_cast<uint>(tbb_threads),
    [&] (uint i) {
      stream_wrapper.run_stream(
        i,
        velo_reader.host_events,
        velo_reader.host_event_offsets,
        velo_reader.host_events_size,
        velo_reader.host_event_offsets_size,
        ut_reader.host_events,
        ut_reader.host_event_offsets,
        ut_reader.host_events_size,
        ut_reader.host_event_offsets_size,
        scifi_hits_events,
        number_of_events_requested,
        number_of_repetitions
      );
    }
  );
  t.stop();

  delete [] scifi_hits_events; 
 
  std::cout << (number_of_events_requested * tbb_threads * number_of_repetitions / t.get()) << " events/s" << std::endl
    << "Ran test for " << t.get() << " seconds" << std::endl;

  std::ofstream outfile;
  outfile.open("../tests/test.txt", std::fstream::in | std::fstream::out | std::ios_base::app);
  outfile << start_event_offset << "\t" << (number_of_events_requested * tbb_threads * number_of_repetitions / t.get()) << std::endl;
  outfile.close();
  
  // Reset device
  cudaCheck(cudaDeviceReset());

  return 0;
}

#pragma once

#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <tuple>

#include "Common.h"
#include "CudaCommon.h"
#include "Logger.h"
#include "Timer.h"
#include "Tools.h"
#include "Catboost.h"
#include "TestCatboost.h"
#include "BaseDynamicScheduler.cuh"
#include "DynamicScheduler.cuh"
#include "SequenceSetup.cuh"
#include "Constants.cuh"
#include "VeloEventModel.cuh"
#include "UTDefinitions.cuh"
#include "RuntimeOptions.h"
#include "EstimateInputSize.cuh"
#include "HostBuffers.cuh"
#include "SequenceVisitor.cuh"

class Timer;

struct Stream {
  // Sequence and arguments
  sequence_t sequence_tuple;

  // Stream datatypes
  cudaStream_t cuda_stream;
  cudaEvent_t cuda_generic_event;
  uint stream_number;

  // Launch options
  bool do_check;
  bool do_simplified_kalman_filter;
  bool do_print_memory_manager;
  bool run_on_x86;

  // Pinned host datatypes
  uint* host_velo_tracks_atomics;
  uint* host_velo_track_hit_number;
  char* host_velo_track_hits;
  uint* host_total_number_of_velo_clusters;
  uint* host_number_of_reconstructed_velo_tracks;
  uint* host_accumulated_number_of_hits_in_velo_tracks;
  char* host_velo_states;
  uint* host_accumulated_number_of_ut_hits;
  uint* host_ut_hit_count;
  VeloUTTracking::TrackUT* host_veloUT_tracks;
  int* host_atomics_veloUT;

  /* UT DECODING */
  UTHits * host_ut_hits_decoded;

  // SciFi Decoding
  uint* host_accumulated_number_of_scifi_hits;

  //Catboost
  int tree_num;
  int model_float_feature_num;
  int model_bin_feature_num;
  int* host_tree_sizes;
  int* host_border_nums;
  int** host_tree_splits;
  float* host_catboost_output;
  float** host_borders;
  float** host_features;
  double** host_leaf_values;
  const int* treeSplitsPtr_flat;
  const double* leafValuesPtr_flat;
  const NCatBoostFbs::TObliviousTrees* ObliviousTrees;

  // Dynamic scheduler
  DynamicScheduler<sequence_t, argument_tuple_t> scheduler;

  // Host buffers
  HostBuffers host_buffers;

  // Monte Carlo folder name
  std::string folder_name_MC;
  uint start_event_offset;

  // GPU Memory base pointer
  char* dev_base_pointer;

  // Constants
  Constants constants;

  // Visitors for sequence algorithms
  SequenceVisitor sequence_visitor;

  cudaError_t initialize(
    const uint max_number_of_events,
    const bool param_do_check,
    const bool param_do_simplified_kalman_filter,
    const bool param_print_memory_usage,
    const bool param_run_on_x86,
    const std::string& param_folder_name_MC,
    const uint param_start_event_offset,
    const size_t param_reserve_mb,
    const uint param_stream_number,
    const Constants& param_constants
  );

  void run_monte_carlo_test(
    const uint number_of_events_requested
  );
  

  cudaError_t run_sequence(
    const RuntimeOptions& runtime_options
  );
};

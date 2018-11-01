#include "Stream.cuh"

/**
 * @brief Sets up the chain that will be executed later.
 */
cudaError_t Stream::initialize(
  const uint max_number_of_events,
  const bool param_do_check,
  const bool param_do_simplified_kalman_filter,
  const bool param_do_print_memory_manager,
  const bool param_run_on_x86,
  const std::string& param_folder_name_MC,
  const uint param_start_event_offset,
  const size_t reserve_mb,
  const uint param_stream_number,
  const Constants& param_constants
) {
  // Set stream and events
  cudaCheck(cudaStreamCreate(&cuda_stream));
  cudaCheck(cudaEventCreate(&cuda_generic_event));

  // Set stream options
  stream_number = param_stream_number;
  do_check = param_do_check;
  do_simplified_kalman_filter = param_do_simplified_kalman_filter;
  do_print_memory_manager = param_do_print_memory_manager;
  run_on_x86 = param_run_on_x86;
  folder_name_MC = param_folder_name_MC;
  start_event_offset = param_start_event_offset;
  constants = param_constants;

  // Special case
  // Populate velo geometry
  cudaCheck(cudaMalloc((void**)&dev_velo_geometry, velopix_geometry.size()));
  cudaCheck(cudaMemcpyAsync(dev_velo_geometry, velopix_geometry.data(), velopix_geometry.size(), cudaMemcpyHostToDevice, stream));

  // Populate UT boards and geometry
  cudaCheck(cudaMalloc((void**)&dev_ut_boards, ut_boards.size()));
  cudaCheck(cudaMemcpyAsync(dev_ut_boards, ut_boards.data(), ut_boards.size(), cudaMemcpyHostToDevice, stream));

  cudaCheck(cudaMalloc((void**)&dev_ut_geometry, ut_geometry.size()));
  cudaCheck(cudaMemcpyAsync(dev_ut_geometry, ut_geometry.data(), ut_geometry.size(), cudaMemcpyHostToDevice, stream));

  // Populate UT magnet tool values
  cudaCheck(cudaMalloc((void**)&dev_ut_magnet_tool, ut_magnet_tool.size()));
  cudaCheck(cudaMemcpyAsync(dev_ut_magnet_tool, ut_magnet_tool.data(), ut_magnet_tool.size(), cudaMemcpyHostToDevice, stream));

  // Populate FT geometry
  cudaCheck(cudaMalloc((void**)&dev_scifi_geometry, scifi_geometry.size()));
  cudaCheck(cudaMemcpyAsync(dev_scifi_geometry, scifi_geometry.data(), scifi_geometry.size(), cudaMemcpyHostToDevice, stream));

  // Memory allocations for host memory (copy back)
  cudaCheck(cudaMallocHost((void**)&host_velo_tracks_atomics, (2 * max_number_of_events + 1) * sizeof(int)));
  cudaCheck(cudaMallocHost((void**)&host_velo_track_hit_number, max_number_of_events * VeloTracking::max_tracks * sizeof(uint)));
  cudaCheck(cudaMallocHost((void**)&host_velo_track_hits, max_number_of_events * VeloTracking::max_tracks * VeloTracking::max_track_size * sizeof(Velo::Hit)));
  cudaCheck(cudaMallocHost((void**)&host_total_number_of_velo_clusters, sizeof(uint)));
  cudaCheck(cudaMallocHost((void**)&host_number_of_reconstructed_velo_tracks, sizeof(uint)));
  cudaCheck(cudaMallocHost((void**)&host_accumulated_number_of_hits_in_velo_tracks, sizeof(uint)));
  cudaCheck(cudaMallocHost((void**)&host_velo_states, max_number_of_events * VeloTracking::max_tracks * sizeof(Velo::State)));
  cudaCheck(cudaMallocHost((void**)&host_veloUT_tracks, max_number_of_events * VeloUTTracking::max_num_tracks * sizeof(VeloUTTracking::TrackUT)));
  cudaCheck(cudaMallocHost((void**)&host_atomics_veloUT, VeloUTTracking::num_atomics * max_number_of_events * sizeof(int)));
  cudaCheck(cudaMallocHost((void**)&host_accumulated_number_of_ut_hits, sizeof(uint)));
  cudaCheck(cudaMallocHost((void**)&host_accumulated_number_of_scifi_hits, sizeof(uint)));

  //Catboost initialization
  CatboostEvaluator evaluator("../../data/MuID-Run2-MC-570-v1.cb");
  model_float_feature_num = (int)evaluator.GetFloatFeatureCount();
  ObliviousTrees = evaluator.GetObliviousTrees();
  tree_num = ObliviousTrees->TreeSizes()->size();
  treeSplitsPtr_flat = ObliviousTrees->TreeSplits()->data();
  leafValuesPtr_flat = ObliviousTrees->LeafValues()->data();
  
  cudaCheck(cudaMallocHost((void***)&host_features, max_number_of_events * sizeof(float*)));
  cudaCheck(cudaMallocHost((void***)&host_borders, model_float_feature_num * sizeof(float*)));
  cudaCheck(cudaMallocHost((void**)&host_border_nums, model_float_feature_num * sizeof(int)));

  cudaCheck(cudaMallocHost((void***)&host_leaf_values, tree_num * sizeof(double*)));
  cudaCheck(cudaMallocHost((void***)&host_tree_splits, tree_num * sizeof(int*)));
  cudaCheck(cudaMallocHost((void**)&host_catboost_output, max_number_of_events * sizeof(float)));
  cudaCheck(cudaMallocHost((void**)&host_tree_sizes, tree_num * sizeof(int)));

  int index = 0;
  for (const auto& ff : *ObliviousTrees->FloatFeatures()) {
    int border_num = ff->Borders()->size();
    host_border_nums[index] = border_num;
    model_bin_feature_num += border_num;
    cudaCheck(cudaMalloc((void**)&host_borders[index], border_num*sizeof(float)));
    cudaCheck(cudaMemcpy(host_borders[index], ff->Borders()+1, border_num*sizeof(float),cudaMemcpyHostToDevice));
    index++;
  }

  for (int i = 0; i < tree_num; i++) {
    host_tree_sizes[i] = ObliviousTrees->TreeSizes()->Get(i);
  }

  for (int i = 0; i < tree_num; i++) {
    int depth = host_tree_sizes[i];

    cudaCheck(cudaMalloc((void**)&host_leaf_values[i], (1 << depth)*sizeof(double)));
    cudaCheck(cudaMemcpy(host_leaf_values[i], leafValuesPtr_flat, (1 << depth)*sizeof(double), cudaMemcpyHostToDevice));

    cudaCheck(cudaMalloc((void**)&host_tree_splits[i], depth*sizeof(int)));
    cudaCheck(cudaMemcpy(host_tree_splits[i], treeSplitsPtr_flat, depth*sizeof(int), cudaMemcpyHostToDevice));
    
    leafValuesPtr_flat += (1 << depth);
    treeSplitsPtr_flat += depth;
  }

  // Define sequence of algorithms to execute
  sequence.set(sequence_algorithms());

  // Get sequence and argument names
  sequence_names = get_sequence_names();
  argument_names = get_argument_names();

  // Set options for each algorithm
  // (number of blocks, number of threads, stream, dynamic shared memory space)
  // Setup sequence items opts that are static and will not change
  // regardless of events on flight
  sequence.item<seq::prefix_sum_single_block>().set_opts(                      dim3(1), dim3(1024), stream);
  sequence.item<seq::copy_and_prefix_sum_single_block>().set_opts(             dim3(1), dim3(1024), stream);
  sequence.item<seq::prefix_sum_single_block_velo_track_hit_number>().set_opts(dim3(1), dim3(1024), stream);
  sequence.item<seq::prefix_sum_single_block_ut_hits>().set_opts(              dim3(1), dim3(1024), stream);
  sequence.item<seq::prefix_sum_single_block_scifi_hits>().set_opts(              dim3(1), dim3(1024), stream);
  // Reserve host buffers
  host_buffers.reserve(max_number_of_events);

  // Get dependencies for each algorithm
  std::vector<std::vector<int>> sequence_dependencies = get_sequence_dependencies();

  // Get output arguments from the sequence
  std::vector<int> sequence_output_arguments = get_sequence_output_arguments();

  // Prepare dynamic scheduler
  scheduler = {
    // get_sequence_names(),
    get_argument_names(), sequence_dependencies, sequence_output_arguments,
    reserve_mb * 1024 * 1024, do_print_memory_manager};

  // Malloc a configurable reserved memory
  cudaCheck(cudaMalloc((void**)&dev_base_pointer, reserve_mb * 1024 * 1024));

  return cudaSuccess;
}

cudaError_t Stream::run_sequence(const RuntimeOptions& runtime_options) {
  for (uint repetition=0; repetition<runtime_options.number_of_repetitions; ++repetition) {
    // Generate object for populating arguments
    ArgumentManager<argument_tuple_t> arguments {dev_base_pointer};

    // Reset scheduler
    scheduler.reset();

    // Visit all algorithms in configured sequence
    run_sequence_tuple(
      sequence_visitor,
      sequence_tuple,
      runtime_options,
      constants,
      arguments,
      scheduler,
      host_buffers,
      cuda_stream,
      cuda_generic_event);

    cudaEventRecord(cuda_generic_event, cuda_stream);
    cudaEventSynchronize(cuda_generic_event);
  }

  return cudaSuccess;
}

void Stream::run_monte_carlo_test(const uint number_of_events_requested) {
  std::cout << "Checking Velo tracks reconstructed on GPU" << std::endl;

  const std::vector<trackChecker::Tracks> tracks_events = prepareTracks(
    host_buffers.host_velo_tracks_atomics,
    host_buffers.host_velo_track_hit_number,
    host_buffers.host_velo_track_hits,
    number_of_events_requested);

  call_pr_checker(
    tracks_events,
    folder_name_MC,
    start_event_offset,
    "Velo"
  );

  /* CHECKING VeloUT TRACKS */
  const std::vector<trackChecker::Tracks> veloUT_tracks = prepareVeloUTTracks(
    host_buffers.host_veloUT_tracks,
    host_buffers.host_atomics_veloUT,
    number_of_events_requested
  );

  std::cout << "Checking VeloUT tracks reconstructed on GPU" << std::endl;
  call_pr_checker(
    veloUT_tracks,
    folder_name_MC,
    start_event_offset,
    "VeloUT"
  );

  /* CHECKING Scifi TRACKS */
  const std::vector<trackChecker::Tracks> scifi_tracks = prepareForwardTracks(
    host_buffers.host_scifi_tracks,
    host_buffers.host_n_scifi_tracks,
    number_of_events_requested
  );
  
  std::cout << "Checking SciFi tracks reconstructed on GPU" << std::endl;
  call_pr_checker (
    scifi_tracks,
    folder_name_MC,
    start_event_offset,
    "Forward");
}

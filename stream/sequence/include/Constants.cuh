#pragma once

#include <array>
#include <cstdint>
#include <algorithm>
#include <numeric>

#include "CudaCommon.h"
#include "VeloDefinitions.cuh"
#include "ClusteringDefinitions.cuh"
#include "ClusteringCommon.h"
#include "VeloUTDefinitions.cuh"
#include "UTDefinitions.cuh"
#include "Logger.h"

/**
 * @brief Struct intended as a singleton with constants defined on GPU.
 * @details __constant__ memory on the GPU has very few use cases.
 *          Instead, global memory is preferred. Hence, this singleton
 *          should allocate the requested buffers on GPU and serve the
 *          pointers wherever needed.
 *          
 *          The pointers are hard-coded. Feel free to write more as needed.
 */
struct Constants {
  std::array<float, VeloUTTracking::n_layers> host_ut_dxDy;
  std::vector<uint> host_unique_x_sector_permutation;
  uint host_unique_x_sectors;

  float* dev_velo_module_zs;
  uint8_t* dev_velo_candidate_ks;
  uint8_t* dev_velo_sp_patterns;
  float* dev_velo_sp_fx;
  float* dev_velo_sp_fy;
  float* dev_ut_dxDy;
  // uint* dev_unique_x_sectors;
  uint* dev_unique_x_sectors_permutation;
  
  void reserve_and_initialize() {
    reserve_constants();
    initialize_constants();
  }

  /**
   * @brief Reserves the constants of the GPU.
   */
  void reserve_constants();

  /**
   * @brief Initializes constants on the GPU.
   */
  void initialize_constants();

  /**
   * @brief Initializes UT decoding constants.
   */
  void initialize_ut_decoding_constants(const std::vector<char>& ut_geometry);
};

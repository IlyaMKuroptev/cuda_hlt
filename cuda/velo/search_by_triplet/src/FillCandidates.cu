#include "FillCandidates.cuh"
#include "VeloEventModel.cuh"
#include <cassert>

__device__ void fill_candidates_impl(
  short* h0_candidates,
  short* h2_candidates,
  const uint* module_hitStarts,
  const uint* module_hitNums,
  const float* hit_Phis,
  const uint hit_offset
) {
  // Notation is m0, m1, m2 in reverse order for each module
  // A hit in those is h0, h1, h2 respectively

  // Assign a h1 to each threadIdx.x
  const auto module_index = blockIdx.y + 2; // 48 blocks y
  const auto m1_hitNums = module_hitNums[module_index];
  for (auto i=0; i<(m1_hitNums + blockDim.x - 1) / blockDim.x; ++i) {
    const auto h1_rel_index = i*blockDim.x + threadIdx.x;

    if (h1_rel_index < m1_hitNums) {
      // Find for module module_index, hit h1_rel_index the candidates
      const auto m0_hitStarts = module_hitStarts[module_index+2] - hit_offset;
      const auto m2_hitStarts = module_hitStarts[module_index-2] - hit_offset;
      const auto m0_hitNums = module_hitNums[module_index+2];
      const auto m2_hitNums = module_hitNums[module_index-2];
      const auto h1_index = module_hitStarts[module_index] + h1_rel_index - hit_offset;

      // Calculate phi limits
      const float h1_phi = hit_Phis[h1_index];

      // Find candidates
      bool first_h0_found = false, last_h0_found = false;
      bool first_h2_found = false, last_h2_found = false;
      
      // Add h0 candidates
      for (auto h0_rel_index=0; h0_rel_index < m0_hitNums; ++h0_rel_index) {
        const unsigned short h0_index = m0_hitStarts + h0_rel_index;
        const auto h0_phi = hit_Phis[h0_index];
        const bool tolerance_condition = fabs(h1_phi - h0_phi) < VeloTracking::phi_extrapolation;

        if (!first_h0_found && tolerance_condition) {
          h0_candidates[2*h1_index] = h0_index;
          first_h0_found = true;
        }
        else if (first_h0_found && !last_h0_found && !tolerance_condition) {
          h0_candidates[2*h1_index + 1] = h0_index;
          last_h0_found = true;
          break;
        }
      }
      if (first_h0_found && !last_h0_found) {
        h0_candidates[2*h1_index + 1] = m0_hitStarts + m0_hitNums;
      }
      // In case of repeated execution, we need to populate
      // the candidates with -1 if not found
      else if (!first_h0_found) {
        h0_candidates[2*h1_index] = -1;
        h0_candidates[2*h1_index + 1] = -1;
      }

      // Add h2 candidates
      for (int h2_rel_index=0; h2_rel_index < m2_hitNums; ++h2_rel_index) {
        const unsigned short h2_index = m2_hitStarts + h2_rel_index;
        const auto h2_phi = hit_Phis[h2_index];
        const bool tolerance_condition = fabs(h1_phi - h2_phi) < VeloTracking::phi_extrapolation;

        if (!first_h2_found && tolerance_condition) {
          h2_candidates[2*h1_index] = h2_index;
          first_h2_found = true;
        }
        else if (first_h2_found && !last_h2_found && !tolerance_condition) {
          h2_candidates[2*h1_index + 1] = h2_index;
          last_h2_found = true;
          break;
        }
      }
      if (first_h2_found && !last_h2_found) {
        h2_candidates[2*h1_index + 1] = m2_hitStarts + m2_hitNums;
      }
      else if (!first_h2_found) {
        h2_candidates[2*h1_index] = -1;
        h2_candidates[2*h1_index + 1] = -1;
      }
    }
  }
}

__global__ void fill_candidates(
  uint* dev_velo_cluster_container,
  uint* dev_module_cluster_start,
  uint* dev_module_cluster_num,
  short* dev_h0_candidates,
  short* dev_h2_candidates
) {
  /* Data initialization */
  // Each event is treated with two blocks, one for each side.
  const uint event_number = blockIdx.x;
  const uint number_of_events = gridDim.x;

  // Pointers to data within the event
  const uint number_of_hits = dev_module_cluster_start[VeloTracking::n_modules * number_of_events];
  const uint* module_hitStarts = dev_module_cluster_start + event_number * VeloTracking::n_modules;
  const uint* module_hitNums = dev_module_cluster_num + event_number * VeloTracking::n_modules;
  const uint hit_offset = module_hitStarts[0];
  assert((module_hitStarts[52] - module_hitStarts[0]) < VeloTracking::max_number_of_hits_per_event);
  
  // Order has changed since SortByPhi
  const float* hit_Phis = (float*) (dev_velo_cluster_container + 4 * number_of_hits + hit_offset);
  short* h0_candidates = dev_h0_candidates + 2*hit_offset;
  short* h2_candidates = dev_h2_candidates + 2*hit_offset;

  fill_candidates_impl(
    h0_candidates,
    h2_candidates,
    module_hitStarts,
    module_hitNums,
    hit_Phis,
    hit_offset
  );
}

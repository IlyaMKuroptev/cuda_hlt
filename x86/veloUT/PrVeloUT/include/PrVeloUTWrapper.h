#include "PrVeloUT.cuh"
#include "Logger.h"

void call_PrVeloUT(
    const uint* velo_track_hit_number,
    const VeloTracking::Hit<mc_check_enabled>* velo_track_hits,
    const int number_of_tracks_event,
    const int accumulated_tracks_event,
    const VeloState* velo_states_event,
    VeloUTTracking::HitsSoA *hits_layers_events,
    const PrUTMagnetTool *magnet_tool,
    VeloUTTracking::TrackUT VeloUT_tracks[VeloUTTracking::max_num_tracks],
    int &n_velo_tracks_in_UT,
    int &n_veloUT_tracks
);

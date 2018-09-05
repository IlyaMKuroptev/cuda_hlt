#pragma once

#include "Common.h"

#include "TrackChecker.h"
#include "PrForward.h"
#include "Tools.h"

std::vector< std::vector< VeloUTTracking::TrackVeloUT > > run_forward_on_CPU (
  std::vector< trackChecker::Tracks > * ft_tracks_events,
  ForwardTracking::HitsSoAFwd * hits_layers_events,
  std::vector< std::vector< VeloUTTracking::TrackVeloUT > > ut_tracks,
  const int &number_of_events
);
#pragma once

#include <dirent.h>
#include <math.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <numeric>
#include <algorithm>
#include <map>
#include <cmath>
#include <stdint.h>
#include "Logger.h"
#include "Common.h"
#include "../../cuda/velo/common/include/VeloDefinitions.cuh"
#include "../../checker/lib/include/Tracks.h"

/**
 * Generic StrException launcher
 */

void readFileIntoVector(
  const std::string& filename,
  std::vector<char>& events
);

void appendFileToVector(
  const std::string& filename,
  std::vector<char>& events,
  std::vector<unsigned int>& event_sizes
);

void readGeometry(
  const std::string& foldername,
  std::vector<char>& geometry
);

void readFolder(
  const std::string& foldername,
  unsigned int fileNumber,
  std::vector<char>& events,
  std::vector<unsigned int>& event_offsets
);

void statistics(
  const std::vector<char>& input,
  std::vector<unsigned int>& event_offsets
);

std::map<std::string, float> calcResults(
  std::vector<float>& times
);

void printOutSensorHits(
  const EventInfo& info,
  int sensorNumber,
  int* prevs,
  int* nexts
);

void printOutAllSensorHits(
  const EventInfo& info,
  int* prevs,
  int* nexts
);

void printInfo(
  const EventInfo& info,
  int numberOfSensors,
  int numberOfHits
);

template <bool mc_check>
void printTrack(
  Track <mc_check> * tracks,
  const int trackNumber,
  std::ofstream& outstream
);

template <bool mc_check>
void printTracks(
  Track <mc_check> * tracks,
  int* n_tracks,
  int n_events,
  std::ofstream& outstream
);

template <bool mc_check>
void writeBinaryTrack(
  const unsigned int* hit_IDs,
  const Track <mc_check> & track,
  std::ofstream& outstream
);

cudaError_t checkSorting(
  const std::vector<std::vector<uint8_t>>& input,
  unsigned int acc_hits,
  unsigned short* dev_hit_phi,
  const std::vector<unsigned int>& hit_offsets
);

void checkTracks(
		 std::vector< trackChecker::Tracks > all_tracks
);

#include "InputReader.h"

Reader::Reader(const std::string& folder_name) : folder_name(folder_name) {
  if (!exists_test(folder_name)) {
    throw StrException("Folder with name " + folder_name + "does not exist.");
  }
}

std::vector<char> GeometryReader::read_geometry(const std::string& filename) {
  std::vector<char> geometry;
  ::read_geometry(folder_name + "/" + filename, geometry);
  return geometry;
}

void EventReader::read_events(uint number_of_files, uint start_event_offset) {
  std::vector<char> events;
  std::vector<uint> event_offsets;

  read_folder(
    folder_name,
    number_of_files,
    events,
    event_offsets,
    start_event_offset
  );

  check_events(events, event_offsets, number_of_files);

  // TODO Remove: Temporal check to understand if number_of_files is the same as number_of_events
  const int number_of_events = event_offsets.size() - 1;
  if (number_of_files != number_of_events) {
    throw StrException("Number of files differs from number of events read.");
  }

  // Copy raw data to pinned host memory
  cudaCheck(cudaMallocHost((void**)&host_events, events.size()));
  cudaCheck(cudaMallocHost((void**)&host_event_offsets, event_offsets.size() * sizeof(uint)));
  std::copy_n(std::begin(events), events.size(), host_events);
  std::copy_n(std::begin(event_offsets), event_offsets.size(), host_event_offsets);

  host_events_size = events.size();
  host_event_offsets_size = event_offsets.size();
}


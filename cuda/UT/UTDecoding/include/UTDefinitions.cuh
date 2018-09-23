#pragma once

#include <stdint.h>
#include <vector>
#include <ostream>
#include "VeloUTDefinitions.cuh"

static constexpr uint32_t ut_number_of_sectors_per_board = 6;
static constexpr uint32_t ut_number_of_geometry_sectors = 1048;

/**
* @brief Offset and number of hits of each layer.
*/
struct UTHitCount {
  uint* layer_offsets;
  uint* n_hits_layers;

  __device__ __host__
  void typecast_before_prefix_sum(
    uint* base_pointer,
    const uint event_number
  );

  __device__ __host__
  void typecast_after_prefix_sum(
    uint* base_pointer,
    const uint event_number,
    const uint number_of_events
  );
};

struct UTBoards {
  uint32_t number_of_boards;
  uint32_t number_of_channels;
  uint32_t* stripsPerHybrids;
  uint32_t* stations;
  uint32_t* layers;
  uint32_t* detRegions;
  uint32_t* sectors;
  uint32_t* chanIDs;

  UTBoards(const std::vector<char>& ut_boards);
  
  __device__ __host__ UTBoards (
    const char* ut_boards
  );
};

struct UTGeometry {
  uint32_t number_of_sectors;
  uint32_t* firstStrip;
  float* pitch;
  float* dy;
  float* dp0diX;
  float* dp0diY;
  float* dp0diZ;
  float* p0X;
  float* p0Y;
  float* p0Z;
  float* cos;

  UTGeometry(const std::vector<char>& ut_geometry);
  
  __device__ __host__ UTGeometry (
    const char* ut_geometry
  );
};

struct  UTRawBank {
  uint32_t sourceID;
  uint32_t number_of_hits;
  uint16_t* data;

  __device__ __host__ UTRawBank (
    const char* ut_raw_bank
  );
};

struct  UTRawEvent {
  uint32_t number_of_raw_banks;
  uint32_t* raw_bank_offsets;
  char* data;

  __device__ __host__ UTRawEvent (
    const uint32_t* ut_raw_event
  );

  __device__ __host__ UTRawBank getUTRawBank(
    const uint32_t index
  ) const;
};

struct UTHit {
  float yBegin;
  float yEnd;
  float zAtYEq0;
  float xAtYEq0;
  float weight;
  uint32_t highThreshold;
  uint32_t LHCbID;
  uint32_t planeCode;

  UTHit() = default;

  UTHit(float yBegin,
        float yEnd,
        float zAtYEq0,
        float xAtYEq0,
        float weight,
        uint32_t highThreshold,
        uint32_t LHCbID,
        uint32_t planeCode
        );

  #define cmpf(a, b) (fabs((a) - (b)) > 0.000065f)

  bool operator!=(const UTHit & h) const {
    if (cmpf(yBegin,     h.yBegin))       return true;
    if (cmpf(yEnd,       h.yEnd))         return true;
    if (cmpf(zAtYEq0,    h.zAtYEq0))      return true;
    if (cmpf(xAtYEq0,    h.xAtYEq0))      return true;
    if (cmpf(weight,     h.weight))       return true;
    if (highThreshold != h.highThreshold) return true;
    if (LHCbID        != h.LHCbID)        return true;
    
    return false;
  }

  bool operator==(const UTHit& h) const {
    return !(*this != h);
  }

  friend std::ostream& operator<<(std::ostream& stream, const UTHit& ut_hit) {
    stream << "UT hit {"
      << ut_hit.LHCbID << ", "
      << ut_hit.yBegin << ", "
      << ut_hit.yEnd << ", "
      << ut_hit.zAtYEq0 << ", "
      << ut_hit.xAtYEq0 << ", "
      << ut_hit.weight << ", "
      << ut_hit.highThreshold << ", "
      << ut_hit.planeCode << "}";

    return stream;
  }
};

/* 
   SoA for hit variables
   The hits for every layer are written behind each other, the offsets 
   are stored for access;
   one Hits structure exists per event
*/
struct UTHits {
  float* yBegin;
  float* yEnd;
  float* zAtYEq0;
  float* xAtYEq0;
  float* weight;
  uint32_t* highThreshold;
  uint32_t* LHCbID;
  uint32_t* planeCode;
  uint32_t* temp;

  UTHits() = default;

  /**
   * @brief Populates the UTHits object pointers from an unsorted array of data
   *        pointed by base_pointer.
   */
  __host__ __device__ 
  void typecast_unsorted(uint32_t* base_pointer, uint32_t total_number_of_hits);

  /**
   * @brief Populates the UTHits object pointers from a sorted array of data
   *        pointed by base_pointer.
   */
  __host__ __device__ 
  void typecast_sorted(uint32_t* base_pointer, uint32_t total_number_of_hits);

  /**
   * @brief Gets a hit in the UTHit format from the global hit index.
   */
  UTHit getHit(uint32_t index) const;

  __host__ __device__ inline bool isYCompatible( const int i_hit, const float y, const float tol ) const { return yMin(i_hit) - tol <= y && y <= yMax(i_hit) + tol; }
  __host__ __device__ inline bool isNotYCompatible( const int i_hit, const float y, const float tol ) const { return yMin(i_hit) - tol > y || y > yMax(i_hit) + tol; }
  __host__ __device__ inline float cosT(const int i_hit, const float dxDy) const { return ( std::fabs( xAtYEq0[i_hit] ) < 1.0E-9 ) ? 1. / std::sqrt( 1 + dxDy * dxDy ) : std::cos(dxDy); }
  __host__ __device__ inline float sinT(const int i_hit, const float dxDy) const { return tanT(i_hit, dxDy) * cosT(i_hit, dxDy); }
  __host__ __device__ inline float tanT(const int i_hit, const float dxDy) const { return -1 * dxDy; }
  __host__ __device__ inline float xAt( const int i_hit, const float globalY, const float dxDy ) const { return xAtYEq0[i_hit] + globalY * dxDy; }
  __host__ __device__ inline float yMax(const int i_hit) const { return std::max( yBegin[i_hit], yEnd[i_hit] ); }
  __host__ __device__ inline float yMid(const int i_hit) const { return 0.5 * ( yBegin[i_hit] + yEnd[i_hit] ); }
  __host__ __device__ inline float yMin(const int i_hit) const { return std::min( yBegin[i_hit], yEnd[i_hit] ); }
};

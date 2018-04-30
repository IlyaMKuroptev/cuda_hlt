#pragma once

#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "../../../../main/include/Common.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <vector>
#include <stdint.h>
#include <stdio.h>

#define N_MODULES 52
namespace VeloTracking {
  // Detector constants
  // Note: constexpr for this variable (arrays) is still not supported, so we need
  //       to initialize it in runtime
  extern __constant__ float velo_module_zs [N_MODULES];
}

// Number of threads
#define NUMTHREADS_X 32

// How many concurrent h1s to process max
// It should be a divisor of NUMTHREADS_X
#define MAX_CONCURRENT_H1 16

// Number of concurrent h1s in the first iteration
// The first iteration has no flagged hits and more triplets per hit
#define MAX_CONCURRENT_H1_FIRST_ITERATION 8

// These parameters impact the found tracks
// Maximum / minimum acceptable phi
// This impacts enourmously the speed of track seeding
#define PHI_EXTRAPOLATION 0.062

// Tolerance angle for forming triplets
#define MAX_SLOPE 0.4f
#define TOLERANCE 0.6f

// Maximum scatter of each three hits
// This impacts velo tracks and a to a lesser extent
// long and long strange tracks
#define MAX_SCATTER_SEEDING 0.004f

// Making a bigger forwarding scatter window causes
// less clones and more ghosts
#define MAX_SCATTER_FORWARDING 0.004f

// Number of seeding iterations before storing tracklets
// This impacts the amount of shared memory to request per thread
// #define SEEDING_CONTINUOUS_ITERATIONS 2

// Maximum number of skipped modules allowed for a track
// before storing it
#define MAX_SKIPPED_MODULES 3

// Total number of atomics required
// This is just a constant (that I keep changing)
#define NUM_ATOMICS 4

// Constants for requested storage on device
#define MAX_TRACKS 1000
#define MAX_TRACK_SIZE 26
#define MAX_NUMHITS_IN_MODULE 300

// Maximum number of tracks to follow at a time
#define TTF_MODULO 2000

// Constants for filters
#define STATES_PER_TRACK 3
#define PARAM_W 3966.94f
#define PARAM_W_INVERTED 0.000252083f

// Asserts
#define ASSERTS_ENABLED false
#if ASSERTS_ENABLED == true
#include "assert.h"
#define ASSERT(EXPR) assert(EXPR);
#else
#define ASSERT(EXPR) 
#endif


  

struct Module {
    uint hitStart;
    uint hitNums;
    float z;

    __device__ Module(){}
    __device__ Module(
      const uint _hitStart,
      const uint _hitNums,
      const float _z
    ) : hitStart(_hitStart), hitNums(_hitNums), z(_z) {}
};

struct HitXY {
    float x;
    float y;

    __device__ HitXY(){}
    __device__ HitXY(
      const float _x,
      const float _y
    ) : x(_x), y(_y) {}
};



struct HitBase { // 4 * 4 = 16 B
    float x;
    float y;
    float z;
      
    __device__ HitBase(){}
    __device__ HitBase(
      const float _x,
      const float _y,
      const float _z
    ) : x(_x), y(_y), z(_z) {}
};

template <bool MCCheck>
struct Hit;

template <>
struct Hit <true> : public HitBase {
    uint32_t LHCbID;
    
    __device__ Hit(){}
    __device__ Hit(
      const float _x,
      const float _y,
      const float _z,
      const uint32_t _LHCbID
    ) : HitBase( _x, _y, _z ), LHCbID( _LHCbID ) {}
};

template <>
struct Hit <false> : public HitBase {
     __device__ Hit(){}
     __device__ Hit(
       const float _x,
       const float _y,
       const float _z
    ) : HitBase( _x, _y, _z) {}
};

/* Structure containing indices to hits within hit array */
struct TrackHits { // 4 + 26 * 4 = 116 B
  unsigned short hitsNum;
  unsigned short hits[MAX_TRACK_SIZE];

  __device__ TrackHits(){}
  __device__ TrackHits(
    const unsigned short _hitsNum,
    const unsigned short _h0,
    const unsigned short _h1,
    const unsigned short _h2
  ) : hitsNum(_hitsNum) {
    hits[0] = _h0;
    hits[1] = _h1;
    hits[2] = _h2;
  }
};

/* Structure to save final track
   Contains information needed later on in the HLT chain
   and / or for truth matching */
template <bool MCCheck>   
struct Track { // 4 + 26 * 16 = 420 B
  unsigned short hitsNum;
  Hit <MCCheck> hits[MAX_TRACK_SIZE];
  
  __device__ Track(){
  hitsNum = 0;
  }
 
  __device__ void addHit( Hit <MCCheck> _h ){
    hits[ hitsNum ] = _h;
    hitsNum++;
  }

}; 

/**
 * @brief A simplified state for the Velo
 *        
 *        {x, y, tx, ty, 0}
 *        
 *        associated with a simplified covariance
 *        since we do two fits (one in X, one in Y)
 *
 *        c00 0.f c20 0.f 0.f
 *            c11 0.f c31 0.f
 *                c22 0.f 0.f
 *                    c33 0.f
 *                        0.f
 */
struct VeloState { // 48 B
  float x, y, tx, ty;
  float c00, c20, c22, c11, c31, c33;
  float chi2;
  float z;
};

/**
 * @brief Means square fit parameters
 *        required for Kalman fit (Velo)
 */
struct TrackFitParameters {
  float tx, ty;
  bool backward;
};

#pragma once

/**
   Contains constants needed for the forward tracking
   - cut values
   - geometry descriptions
   - parameterizations
 */

namespace Forward{
  
  // dump a bunch of options here
  const float        deltaQuality = 0.1; // Difference in quality btw two tracks which share hits when clone killing
  const float        cloneFraction = 0.4; // The fraction of shared SciFi hits btw two tracks to trigger the clone killing
  
  const float        yTolUVSearch           =   11.* Gaudi::Units::mm  ;
  const float        tolY                   =    5.* Gaudi::Units::mm  ;
  const float        tolYSlope              =0.002 * Gaudi::Units::mm  ;
  const float        maxChi2LinearFit       =  100.                    ;   
  const float        maxChi2XProjection     =   15.                    ;   
  const float        maxChi2PerDoF          =    7.                    ;   

  const float        tolYMag                =   10.* Gaudi::Units::mm  ;
  const float        tolYMagSlope           =    0.015                 ;   
  const float        minYGap                =  0.4 * Gaudi::Units::mm  ;

  const unsigned int minTotalHits           =   10                     ;   
  const float        maxChi2StereoLinear    =   60.                    ;   
  const float        maxChi2Stereo          =    4.5                   ; 
 
  //first loop Hough Cluster search
  const unsigned int minXHits               =    5                     ;   
  const float        maxXWindow             =  1.2 * Gaudi::Units::mm  ;
  const float        maxXWindowSlope        =0.002 * Gaudi::Units::mm  ;
  const float        maxXGap                =  1.2 * Gaudi::Units::mm  ;
  const unsigned int minSingleHits          =    2                     ; 
 
  //second loop Hough Cluster search
  const bool         secondLoop             =  true                    ;   
  const unsigned int minXHits_2nd           =    4                     ;   
  const float        maxXWindow_2nd         =  1.5 * Gaudi::Units::mm  ;
  const float        maxXWindowSlope_2nd    =0.002 * Gaudi::Units::mm  ;
  const float        maxXGap_2nd            =  0.5 * Gaudi::Units::mm  ;

  //collectX search
  const float        minPt                  =  500 * Gaudi::Units::MeV ;
  //stereo hit matching
  const float        tolYCollectX           =    4.1* Gaudi::Units::mm ;
  const float        tolYSlopeCollectX      =0.0018 * Gaudi::Units::mm ;
  const float        tolYTriangleSearch     =    20.f                  ;   
  //veloUT momentum estimate
  const bool         useMomentumEstimate    = true                     ;   
  const bool         useWrongSignWindow     = true                     ;   
  const float        wrongSignPT            = 2000.* Gaudi::Units::MeV ; 
  //Track Quality NN
  const float        maxQuality             =   0.9                    ;   
  const float        deltaQuality_NN        =   0.1                    ; 

  // the Magnet Parametrization
  const float        zMagnetParams[4]       = {5212.38, 406.609, -1102.35, -498.039};

  // more Parametrizations
  const float        xParams[2]             = {18.6195, -5.55793};
  const float        byParams               = -0.667996;
  const float        cyParams               = -3.68424e-05;
 
  // momentum Parametrization
  const float        momentumParams[6]      = {1.21014, 0.637339, -0.200292, 0.632298, 3.23793, -27.0259};
  
  // covariance values
  const float        covarianceValues[5]    = {4.0, 400.0, 4.e-6, 1.e-4, 0.1};

  // z Reference plane
  const float        zReference             = 8520.; // in T2

  // definition of zones
  // access upper with offset of 6
  const int	     zoneoffsetpar	      = 6;
  const int          xZones[12]             = {0 , 6 , 8 , 14 , 16 , 22, 1 , 7 , 9 , 15 , 17 , 23 };
  const int          uvZones[12]            = {2 , 4 , 10, 12 , 18 , 20, 3 , 5 , 11, 13 , 19 , 21 };

  // ASSORTED GEOMETRY VALUES, eventually read this from some xml
  const float        xZone_zPos[6]          = {7826., 8036., 8508., 8718., 9193., 9403.};
  const float        uvZone_zPos[12]        = {7896., 7966., 8578., 8648., 9263., 9333., 7896., 7966., 8578., 8648., 9263., 9333.};
  const float        uvZone_dxdy[12]        = {0.0874892, -0.0874892, 0.0874892, -0.0874892, 0.0874892, -0.0874892, 0.0874892, -0.0874892, 0.0874892, -0.0874892, 0.0874892, -0.0874892};
  const float        Zone_dzdy[24]          = {0.0036010};
  // CHECK THESE VALUES USING FRAMEWORK
  const float        xLim_Max               = 3300.;
  const float        yLim_Max               = 2500.;
  const float        xLim_Min               = -3300.;
  const float        yLim_Min               = -25.;

  // other variables which get initialized
  const float              xHits[8]               = {0.,0.,0.,0.,0.,0.,0.,0.};

  // TO BE READ FROM XML EVENTUALLY
  const float              magscalefactor         = -1;
 
}
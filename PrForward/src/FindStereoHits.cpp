#include "FindStereoHits.h"


//=========================================================================
//  Collect all hits in the stereo planes compatible with the track
//=========================================================================
void collectStereoHits(
  SciFi::HitsSoA* hits_layers,
  SciFi::Tracking::Track& track,
  MiniState velo_state,
  SciFi::Tracking::HitSearchCuts& pars,
  int stereoHits[SciFi::Tracking::max_stereo_hits],
  int& n_stereoHits)
{
  
  for ( int zone = 0; zone < SciFi::Constants::n_layers; ++zone ) {
    const float parsX[4] = {track.trackParams[0],
                            track.trackParams[1],
                            track.trackParams[2],
                            track.trackParams[3]};
    const float parsY[4] = {track.trackParams[4],
                            track.trackParams[5],
                            track.trackParams[6],
                            0.};
    float zZone = SciFi::Tracking::uvZone_zPos[zone];
    const float yZone = straightLineExtend(parsY,zZone);
    zZone += SciFi::Tracking::Zone_dzdy[SciFi::Tracking::uvZones[zone]]*yZone;  // Correct for dzDy
    const float xPred  = straightLineExtend(parsX,zZone);

    const bool triangleSearch = std::fabs(yZone) < SciFi::Tracking::tolYTriangleSearch;
    // even zone number: if ( yZone > 0 ) continue;
    // odd zone number: if ( -yZone > 0 ) continue;
    // -> check for upper / lower half
    // -> only continue if yZone is in the correct half
    if(!triangleSearch && (2.f*float(((SciFi::Tracking::uvZones[zone])%2)==0)-1.f) * yZone > 0.f) continue;

    //float dxDySign = 1.f - 2.f *(float)(zone.dxDy()<0); // same as ? zone.dxDy()<0 : -1 : +1 , but faster??!!
    const float dxDySign = SciFi::Tracking::uvZone_dxdy[zone] < 0 ? -1.f : 1.f;
    const float seed_x_at_zZone = velo_state.x + (zZone - velo_state.z) * velo_state.tx;//Cached as we are upgrading one at a time, revisit
    const float dxTol = SciFi::Tracking::tolY + SciFi::Tracking::tolYSlope * (std::fabs(xPred - seed_x_at_zZone) + std::fabs(yZone));

    // -- Use a binary search to find the lower bound of the range of x values
    // -- This takes the y value into account
    const float lower_bound_at = -dxTol - yZone * SciFi::Tracking::uvZone_dxdy[zone] + xPred;
    int uv_zone_offset_begin = hits_layers->layer_offset[SciFi::Tracking::uvZones[zone]];
    int uv_zone_offset_end   = hits_layers->layer_offset[SciFi::Tracking::uvZones[zone]+1];
    int itH   = getLowerBound(hits_layers->m_x,lower_bound_at,uv_zone_offset_begin,uv_zone_offset_end);
    int itEnd = uv_zone_offset_end;

    if(triangleSearch){
      for ( ; itEnd != itH; ++itH ) {
        const float dx = hits_layers->m_x[itH] + yZone * hits_layers->m_dxdy[itH] - xPred ;
        if ( dx >  dxTol ) break;
        if( yZone > hits_layers->m_yMax[itH] + SciFi::Tracking::yTolUVSearch)continue;
        if( yZone < hits_layers->m_yMin[itH] - SciFi::Tracking::yTolUVSearch)continue;
        hits_layers->m_coord[itH] = dx*dxDySign;
        if ( n_stereoHits >= SciFi::Tracking::max_stereo_hits )
          break;
        stereoHits[n_stereoHits++] = itH;
      }
    }else{ //no triangle search, thus no min max check
      for ( ; itEnd != itH; ++itH ) {
        const float dx = hits_layers->m_x[itH] + yZone * hits_layers->m_dxdy[itH] - xPred ;
        if ( dx >  dxTol ) break;
        hits_layers->m_coord[itH] = dx*dxDySign;
        if ( n_stereoHits >= SciFi::Tracking::max_stereo_hits )
          break;
        stereoHits[n_stereoHits++] = itH;
      }
    }
    if ( n_stereoHits >= SciFi::Tracking::max_stereo_hits )
      break;
  }

}
 
//=========================================================================
//  Fit the stereo hits
//=========================================================================
bool selectStereoHits(
  SciFi::HitsSoA* hits_layers,
  SciFi::Tracking::Track& track, 
  int stereoHits[SciFi::Tracking::max_stereo_hits],
  int& n_stereoHits,
  MiniState velo_state, 
  SciFi::Tracking::HitSearchCuts& pars)
{
  std::vector<int> stereoHitsVec;
  stereoHitsVec.clear();
  for ( int i = 0; i < n_stereoHits; ++i ) {
    stereoHitsVec.push_back( stereoHits[i] );
  }
  
  //why do we rely on xRef? --> coord is NOT xRef for stereo HITS!
  int bestStereoHits[SciFi::Tracking::max_stereo_hits];
  int n_bestStereoHits = 0;
  float originalYParams[3] = {track.trackParams[4],
			      track.trackParams[5],
                              track.trackParams[6]};
  float bestYParams[3];
  float bestMeanDy       = 1e9f;

  int beginRange = -1; 
  
  if(pars.minStereoHits > n_stereoHits) return false; //otherwise crash if minHits is too large
  int endLoop = n_stereoHits - pars.minStereoHits;
  
  PlaneCounter planeCounter;
  while ( beginRange < endLoop ) {
    ++beginRange;
    planeCounter.clear();
    int endRange = beginRange;

    float sumCoord = 0.;
    // bad hack to reproduce itereator behavior from before
    int first_hit;
    if ( endRange == 0 )
      first_hit = 0;
    else
      first_hit = stereoHits[endRange-1];
    
    while( planeCounter.nbDifferent < pars.minStereoHits ||
           hits_layers->m_coord[ stereoHits[endRange] ] < hits_layers->m_coord[ first_hit] + SciFi::Tracking::minYGap ) {
      planeCounter.addHit( hits_layers->m_planeCode[ stereoHits[endRange] ] / 2 );
      sumCoord += hits_layers->m_coord[ stereoHits[endRange] ];
      ++endRange;
      if ( endRange == n_stereoHits ) break;
      first_hit = stereoHits[endRange-1];
    }

    //clean cluster
    while( true ) {
      const float averageCoord = sumCoord / float(endRange-beginRange);
      
      // remove first if not single and farthest from mean
      if ( planeCounter.nbInPlane( hits_layers->m_planeCode[ stereoHits[beginRange] ]/2 ) > 1 &&
           ((averageCoord - hits_layers->m_coord[ stereoHits[beginRange] ]) > 1.0f * 
            (hits_layers->m_coord[ stereoHits[endRange-1] ] - averageCoord)) ) {

        planeCounter.removeHit( hits_layers->m_planeCode[ stereoHits[beginRange] ]/2 );
        sumCoord -= hits_layers->m_coord[ stereoHits[beginRange] ];
        beginRange++;
        continue;
      }

      if(endRange == n_stereoHits) break; //already at end, cluster cannot be expanded anymore
      //add next, if it decreases the range size and is empty
      if ( (planeCounter.nbInPlane( hits_layers->m_planeCode[ stereoHits[beginRange] ]/2 ) == 0) &&
           (averageCoord - hits_layers->m_coord[ stereoHits[beginRange] ] > 
            hits_layers->m_coord[ stereoHits[endRange] ] - averageCoord )
         ) {
        planeCounter.addHit( hits_layers->m_planeCode[ stereoHits[endRange] ]/2 );
        sumCoord += hits_layers->m_coord[ stereoHits[endRange]];
        endRange++;
        continue;
      }

      break;
    }

    //Now we have a candidate, lets fit him
    // track = original; //only yparams are changed
    track.trackParams[4] = originalYParams[0];
    track.trackParams[5] = originalYParams[1];
    track.trackParams[6] = originalYParams[2];
   
    int trackStereoHits[SciFi::Tracking::max_stereo_hits];
    int n_trackStereoHits = 0;
    
    for ( int range = beginRange; range < endRange; ++range ) {
      trackStereoHits[n_trackStereoHits++] = stereoHits[range];
    }
    
    //fit Y Projection of track using stereo hits
    if(!fitYProjection(hits_layers, track, trackStereoHits, n_trackStereoHits, planeCounter, velo_state, pars))continue;
    // debug_cout << "Passed the Y fit" << std::endl;

    if(!addHitsOnEmptyStereoLayers(hits_layers, track, trackStereoHits, n_trackStereoHits, planeCounter, velo_state, pars))continue;
    //debug_cout << "Passed adding hits on empty stereo layers" << std::endl;
  
    if(n_trackStereoHits < n_bestStereoHits) continue; //number of hits most important selection criteria!
 
    //== Calculate  dy chi2 /ndf
    float meanDy = 0.;
    for ( int i_hit = 0; i_hit < n_trackStereoHits; ++i_hit ) {
      int hit = trackStereoHits[i_hit];
      const float d = trackToHitDistance(track.trackParams, hits_layers, hit) / hits_layers->m_dxdy[hit];
      meanDy += d*d;
    }
    meanDy /=  float(n_trackStereoHits-1);

    if ( n_trackStereoHits > n_bestStereoHits || meanDy < bestMeanDy  ){
      // if same number of hits take smaller chi2
      bestYParams[0] = track.trackParams[4];
      bestYParams[1] = track.trackParams[5];
      bestYParams[2] = track.trackParams[6];
      bestMeanDy     = meanDy;

      n_bestStereoHits = 0;
      for ( int i_hit = 0; i_hit < n_trackStereoHits; ++i_hit ) {
        assert( n_bestStereoHits < SciFi::Tracking::max_stereo_hits );
        bestStereoHits[n_bestStereoHits++] = trackStereoHits[i_hit];
      }
    }

  }
  if ( n_bestStereoHits > 0 ) {
    track.trackParams[4] = bestYParams[0];
    track.trackParams[5] = bestYParams[1];
    track.trackParams[6] = bestYParams[2];
    for ( int i_hit = 0; i_hit < n_bestStereoHits; ++i_hit ) {
      int hit = bestStereoHits[i_hit];
      track.addHit( hit );
    }
    return true;
  }
  return false;
}
 

//=========================================================================
//  Add hits on empty stereo layers, and refit if something was added
//=========================================================================
bool addHitsOnEmptyStereoLayers(
  SciFi::HitsSoA* hits_layers,
  SciFi::Tracking::Track& track,
  int stereoHits[SciFi::Tracking::max_stereo_hits],
  int& n_stereoHits,
  PlaneCounter& planeCounter,
  MiniState velo_state,
  SciFi::Tracking::HitSearchCuts& pars)
{
  //at this point pc is counting only stereo HITS!
  if(planeCounter.nbDifferent  > 5) return true;

  bool added = false;
  for ( unsigned int zone = 0; zone < 12; zone += 1 ) {
    if ( planeCounter.nbInPlane( SciFi::Tracking::uvZones[zone]/2 ) != 0 ) continue; //there is already one hit

    float zZone = SciFi::Tracking::uvZone_zPos[zone];

    const float parsX[4] = {track.trackParams[0],
                            track.trackParams[1],
                            track.trackParams[2],
                            track.trackParams[3]};
    const float parsY[4] = {track.trackParams[4],
                            track.trackParams[5],
                            track.trackParams[6],
                            0.};

    float yZone = straightLineExtend(parsY,zZone);
    zZone = SciFi::Tracking::Zone_dzdy[SciFi::Tracking::uvZones[zone]]*yZone;  // Correct for dzDy
    yZone = straightLineExtend(parsY,zZone);
    const float xPred  = straightLineExtend(parsX,zZone);

    const bool triangleSearch = std::fabs(yZone) < SciFi::Tracking::tolYTriangleSearch;
    // Not 100% sure about logic of next line, check it!
    if(!triangleSearch && (2.f*float((((SciFi::Tracking::uvZones[zone])%2)==0))-1.f) * yZone > 0.f) continue;

    //only version without triangle search!
    const float dxTol = SciFi::Tracking::tolY + SciFi::Tracking::tolYSlope * ( fabs( xPred - velo_state.x + (zZone - velo_state.z) * velo_state.tx) + fabs(yZone) );
    // -- Use a binary search to find the lower bound of the range of x values
    // -- This takes the y value into account
    const float lower_bound_at = -dxTol - yZone * SciFi::Tracking::uvZone_dxdy[zone] + xPred;
    int uv_zone_offset_begin = hits_layers->layer_offset[SciFi::Tracking::uvZones[zone]];
    int uv_zone_offset_end   = hits_layers->layer_offset[SciFi::Tracking::uvZones[zone]+1];
    int itH   = getLowerBound(hits_layers->m_x,lower_bound_at,uv_zone_offset_begin,uv_zone_offset_end);
    int itEnd = uv_zone_offset_end;
    
    int best = -1;
    float bestChi2 = SciFi::Tracking::maxChi2Stereo;
    if(triangleSearch){
      for ( ; itEnd != itH; ++itH ) {
        const float dx = hits_layers->m_x[itH] + yZone * hits_layers->m_dxdy[itH] - xPred ;
        if ( dx >  dxTol ) break;
        if( yZone > hits_layers->m_yMax[itH] + SciFi::Tracking::yTolUVSearch)continue;
        if( yZone < hits_layers->m_yMin[itH] - SciFi::Tracking::yTolUVSearch)continue;
        const float chi2 = dx*dx*hits_layers->m_w[itH];
        if ( chi2 < bestChi2 ) {
          bestChi2 = chi2;
          best = itH;
        }    
      }    
    }else{
      //no triangle search, thus no min max check
      for ( ; itEnd != itH; ++itH ) {
        const float dx = hits_layers->m_x[itH] + yZone * hits_layers->m_dxdy[itH] - xPred ;
        if ( dx >  dxTol ) break;
        const float chi2 = dx*dx*hits_layers->m_w[itH];
        if ( chi2 < bestChi2 ) {
          bestChi2 = chi2;
          best = itH;
        }
      }
    }

    if ( -1 != best ) {
      assert( n_stereoHits < SciFi::Tracking::max_stereo_hits );
      stereoHits[n_stereoHits++] = best;
      planeCounter.addHit( hits_layers->m_planeCode[best]/2 );
      added = true;
    }
  }
  if ( !added ) return true;
  return fitYProjection( hits_layers, track, stereoHits, n_stereoHits, planeCounter, velo_state, pars );
}
 

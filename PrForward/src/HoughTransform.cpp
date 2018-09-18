#include "HoughTransform.h"

//calculate xref for this plane
//in the c++ this is vectorized, undoing because no point before CUDA (but vectorization is obvious)
void xAtRef_SamePlaneHits(
  SciFi::HitsSoA* hits_layers,
  std::vector<int>& allXHits,
  const float xParams_seed[4],
  MiniState velo_state, 
  int itH, int itEnd)
{
  //this is quite computationally expensive mind, should take care when porting
  float zHit    = hits_layers->m_z[allXHits[itH]]; //all hits in same layer
  float xFromVelo_Hit = straightLineExtend(xParams_seed,zHit);
  float zMagSlope = SciFi::Tracking::zMagnetParams[2] * pow(velo_state.tx,2) +  SciFi::Tracking::zMagnetParams[3] * pow(velo_state.ty,2);
  float dSlopeDivPart = 1.f / ( zHit - SciFi::Tracking::zMagnetParams[0]);
  float dz      = 1.e-3f * ( zHit - SciFi::Tracking::zReference );
  
  while( itEnd>itH ){
    float xHit = hits_layers->m_x[allXHits[itH]];
    float dSlope  = ( xFromVelo_Hit - xHit ) * dSlopeDivPart;
    float zMag    = SciFi::Tracking::zMagnetParams[0] + SciFi::Tracking::zMagnetParams[1] *  dSlope * dSlope  + zMagSlope;
    float xMag    = xFromVelo_Hit + velo_state.tx * (zMag - zHit);
    float dxCoef  = dz * dz * ( SciFi::Tracking::xParams[0] + dz * SciFi::Tracking::xParams[1] ) * dSlope;
    float ratio   = (  SciFi::Tracking::zReference - zMag ) / ( zHit - zMag );
    hits_layers->m_coord[allXHits[itH]] = xMag + ratio * (xHit + dxCoef  - xMag);
    itH++;
  }
}
 

bool fitXProjection(
  SciFi::HitsSoA *hits_layers,
  std::vector<float> &trackParameters,
  std::vector<unsigned int> &coordToFit,
  PlaneCounter& planeCounter,
  SciFi::Tracking::HitSearchCuts& pars)
{

  if (planeCounter.nbDifferent < pars.minXHits) return false;
  bool doFit = true;
  while ( doFit ) {
    //== Fit a cubic
    float s0   = 0.f; 
    float sz   = 0.f; 
    float sz2  = 0.f; 
    float sz3  = 0.f; 
    float sz4  = 0.f; 
    float sd   = 0.f; 
    float sdz  = 0.f; 
    float sdz2 = 0.f; 

    for (auto hit : coordToFit ) {
      float d = trackToHitDistance(trackParameters, hits_layers, hit);
      float w = hits_layers->m_w[hit];
      float z = .001f * ( hits_layers->m_z[hit] - SciFi::Tracking::zReference );
      s0   += w;
      sz   += w * z; 
      sz2  += w * z * z; 
      sz3  += w * z * z * z; 
      sz4  += w * z * z * z * z; 
      sd   += w * d; 
      sdz  += w * d * z; 
      sdz2 += w * d * z * z; 
    }    
    const float b1 = sz  * sz  - s0  * sz2; 
    const float c1 = sz2 * sz  - s0  * sz3; 
    const float d1 = sd  * sz  - s0  * sdz; 
    const float b2 = sz2 * sz2 - sz * sz3; 
    const float c2 = sz3 * sz2 - sz * sz4; 
    const float d2 = sdz * sz2 - sz * sdz2;
    const float den = (b1 * c2 - b2 * c1 );
    if(!(std::fabs(den) > 1e-5)) return false;
    const float db  = (d1 * c2 - d2 * c1 ) / den; 
    const float dc  = (d2 * b1 - d1 * b2 ) / den; 
    const float da  = ( sd - db * sz - dc * sz2) / s0;
    trackParameters[0] += da;
    trackParameters[1] += db*1.e-3f;
    trackParameters[2] += dc*1.e-6f;    

    float maxChi2 = 0.f; 
    float totChi2 = 0.f;  
    //int   nDoF = -3; // fitted 3 parameters
    int  nDoF = -3;
    const bool notMultiple = planeCounter.nbDifferent == coordToFit.size();

    const auto itEnd = coordToFit.back();
    auto worst = itEnd;
    for ( auto itH : coordToFit ) {
      float d = trackToHitDistance(trackParameters, hits_layers, itH);
      float chi2 = d*d*hits_layers->m_w[itH];
      totChi2 += chi2;
      ++nDoF;
      if ( chi2 > maxChi2 && ( notMultiple || planeCounter.nbInPlane( hits_layers->m_planeCode[itH]/2 ) > 1 ) ) {
        maxChi2 = chi2;
        worst   = itH; 
      }    
    }    
    if ( nDoF < 1 )return false;
    trackParameters[7] = totChi2;
    trackParameters[8] = (float) nDoF;

    if ( worst == itEnd ) {
      return true;
    }    
    doFit = false;
    if ( totChi2/nDoF > SciFi::Tracking::maxChi2PerDoF  ||
         maxChi2 > SciFi::Tracking::maxChi2XProjection ) {
      // Should really make this bit of code into a function... 
      planeCounter.removeHit( hits_layers->m_planeCode[worst]/2 );
      std::vector<unsigned int> coordToFit_temp;
      coordToFit_temp.clear();
      for (auto hit : coordToFit) {
        if (hit != worst) coordToFit_temp.push_back(hit);
      }
      coordToFit = coordToFit_temp;
      if (planeCounter.nbDifferent < pars.minXHits + pars.minStereoHits) return false;
      doFit = true;
    }    
  }
  return true;
}
 

bool fitYProjection(
  SciFi::HitsSoA* hits_layers,
  SciFi::Track& track,
  std::vector<int>& stereoHits,
  PlaneCounter& planeCounter,
  MiniState velo_state,
  SciFi::Tracking::HitSearchCuts& pars)
{
  
  //debug_cout << "About to fit a Y projection with " << stereoHits.size() << " hits on " << planeCounter.nbDifferent << " different planes looking for " << pars.minStereoHits << std::endl;
  //if ( nbDifferent(planelist) < pars.minStereoHits ) return false;
  float maxChi2 = 1.e9f;
  bool parabola = false; //first linear than parabola
  //== Fit a line
  const float txs  = track.trackParams[0]; // simplify overgeneral c++ calculation
  const float tsxz = velo_state.x + (SciFi::Tracking::zReference - velo_state.z) * velo_state.tx; 
  const float tolYMag = SciFi::Tracking::tolYMag + SciFi::Tracking::tolYMagSlope * fabs(txs-tsxz);
  const float wMag   = 1./(tolYMag * tolYMag );

  bool doFit = true;
  while ( doFit ) {
    //Use position in magnet as constrain in fit
    //although because wMag is quite small only little influence...
    float zMag  = zMagnet(velo_state);
    const float tys = track.trackParams[4]+(zMag-SciFi::Tracking::zReference)*track.trackParams[5];
    const float tsyz = velo_state.y + (zMag-velo_state.z)*velo_state.ty;
    const float dyMag = tys-tsyz;
    zMag -= SciFi::Tracking::zReference;
    float s0   = wMag;
    float sz   = wMag * zMag;
    float sz2  = wMag * zMag * zMag;
    float sd   = wMag * dyMag;
    float sdz  = wMag * dyMag * zMag;

    std::vector<int>::const_iterator itEnd = std::end(stereoHits);

    if ( parabola ) {
      float sz2m = 0.;
      float sz3  = 0.;
      float sz4  = 0.;
      float sdz2 = 0.;

      for ( const auto hit : stereoHits ){
        const float d = - trackToHitDistance(track.trackParams, hits_layers, hit) / 
			  hits_layers->m_dxdy[hit];//TODO multiplication much faster than division!
        const float w = hits_layers->m_w[hit];
        const float z = hits_layers->m_z[hit] - SciFi::Tracking::zReference;
        s0   += w;
        sz   += w * z; 
        sz2m += w * z * z; 
        sz2  += w * z * z; 
        sz3  += w * z * z * z; 
        sz4  += w * z * z * z * z; 
        sd   += w * d; 
        sdz  += w * d * z; 
        sdz2 += w * d * z * z; 
      }    
      const float b1 = sz  * sz   - s0  * sz2; 
      const float c1 = sz2m* sz   - s0  * sz3; 
      const float d1 = sd  * sz   - s0  * sdz; 
      const float b2 = sz2 * sz2m - sz * sz3; 
      const float c2 = sz3 * sz2m - sz * sz4; 
      const float d2 = sdz * sz2m - sz * sdz2;
      const float den = (b1 * c2 - b2 * c1 );
      if(!(std::fabs(den) > 1e-5)) {
        // debug_cout << "Failing Y projection parabola fit" << std::endl;
	return false;
      }

      const float db  = (d1 * c2 - d2 * c1 ) / den; 
      const float dc  = (d2 * b1 - d1 * b2 ) / den; 
      const float da  = ( sd - db * sz - dc * sz2 ) / s0;
      track.trackParams[4] += da;
      track.trackParams[5] += db;
      track.trackParams[6] += dc;
    } else { // straight line fit

      for ( const auto hit : stereoHits ){
        const float d = - trackToHitDistance(track.trackParams, hits_layers, hit) / 
                          hits_layers->m_dxdy[hit];//TODO multiplication much faster than division!
        const float w = hits_layers->m_w[hit];
        const float z = hits_layers->m_z[hit] - SciFi::Tracking::zReference;
	s0   += w;
        sz   += w * z; 
        sz2  += w * z * z;
        sd   += w * d;
        sdz  += w * d * z;
      }
      const float den = (s0 * sz2 - sz * sz );
      if(!(std::fabs(den) > 1e-5)) { 
        // debug_cout << "Failing Y projection straight line fit" << std::endl;
        return false;
      }
      const float da  = (sd * sz2 - sdz * sz ) / den;
      const float db  = (sdz * s0 - sd  * sz ) / den;
      track.trackParams[4] += da;
      track.trackParams[5] += db;
    }//fit end, now doing outlier removal

    std::vector<int>::iterator worst = std::end(stereoHits);
    maxChi2 = 0.;
    for ( std::vector<int>::iterator itH = std::begin(stereoHits); itEnd != itH; ++itH) {
      float d = trackToHitDistance(track.trackParams, hits_layers, *itH);
      float chi2 = d*d*hits_layers->m_w[*itH];
      if ( chi2 > maxChi2 ) {
        maxChi2 = chi2;
        worst   = itH;
      }
    }

    if ( maxChi2 < SciFi::Tracking::maxChi2StereoLinear && !parabola ) {
      // debug_cout << "Maximum chi2 from linear fit was relatively small " << maxChi2 << " do parabolic fit" << std::endl;
      parabola = true;
      maxChi2 = 1.e9f;
      continue;
    }

    if ( maxChi2 > SciFi::Tracking::maxChi2Stereo ) {
      // debug_cout << "Removing hit " << *worst << " with chi2 " << maxChi2 << " allowable was " << SciFi::Tracking::maxChi2Stereo << std::endl;
      planeCounter.removeHit( hits_layers->m_planeCode[*worst]/2 );
      if ( planeCounter.nbDifferent < pars.minStereoHits ) {
	// debug_cout << "Failing because we have " << planeCounter.nbDifferent << " different planes and we need " << pars.minStereoHits << std::endl;
        return false;
      }
      stereoHits.erase( worst );
      continue;
    }
    break;
  }
  return true;
} 
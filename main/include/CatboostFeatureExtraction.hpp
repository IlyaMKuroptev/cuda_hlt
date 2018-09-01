#ifndef CatboostFeatureExtraction_hpp
#define CatboostFeatureExtraction_hpp

#include <vector>
#include "InputDataManager.hpp"
#include "MuonHit.hpp"

class CatboostFeatureExtraction
{
public:
    CatboostFeatureExtraction();
    
    void load_features();
    
    std::vector<float>& get_features();
    
private:
    InputDataManager * m_data;
    
    std::vector<MuonHit> matched_hits;
    
    std::vector<float> features;
    
    std::vector<float> dts;
    
    std::vector<float> times;
    
    std::vector<float> cross;
    
    std::vector<float> resX;
    
    std::vector<float> resY;
    
    void track_to_hit();
    
    void extract_dts();
    
    void extract_time();
    
    void extract_cross();
};


#endif

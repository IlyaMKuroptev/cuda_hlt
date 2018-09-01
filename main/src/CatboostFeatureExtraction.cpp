#include "CatboostFeatureExtraction.hpp"

void CatboostFeatureExtraction::load_features()
{}

std::vector<float>& CatboostFeatureExtraction::get_features()
{
    return features;
}


void CatboostFeatureExtraction::extract_dts()
{
    dts.resize(m_data->nStations());
    for (int st = 0; st < m_data->nStations(); ++st)
    {
        dts[st] = matched_hits[st].deltaTime;
    }
}

void CatboostFeatureExtraction::extract_time()
{
    dts.resize(m_data->nStations());
    for (int st = 0; st < m_data->nStations(); ++st)
    {
        times[st] = matched_hits[st].time;
    }
}

void CatboostFeatureExtraction::extract_cross()
{
    dts.resize(m_data->nStations());
    for (int st = 0; st < m_data->nStations(); ++st)
    {
        if (matched_hits[st].uncrossed == 0)
        {
            cross[st] = 2.0;
        }
        else
        {
            cross[st] = matched_hits[st].uncrossed;
        }
    }
}

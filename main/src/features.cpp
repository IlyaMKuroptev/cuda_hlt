#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>

#define SQRT3 1.7320508075688772
#define INVSQRT3 0.5773502691896258
#define MSFACTOR 5.552176750308537


using MuonTrackExtrapolation = std::vector<std::pair<float, float>>;
using LHCbID = int;

int nStations = 4;

struct State
{
    State()
    {
        x = ((float)rand());
        y = ((float)rand());
        tx = ((float)rand());
        ty = ((float)rand());
        z = ((float)rand());
    }
    float x;
    float y;
    float tx;
    float ty;
    float z;
};

struct Track
{
    Track()
    {
        p = float(rand());
    }
    State closestState(float f)
    {
        return *new State;
    };
    
    float p;
};


struct CommonMuonTool
{
    CommonMuonTool()
    {
        for (auto i = 0; i < nStations; ++i)
        {
            m_stationZ.push_back(float(rand()));
        }
    }
    using MuonTrackExtrapolation = std::vector<std::pair<float, float>>;
    std::vector<float> m_stationZ;
    size_t m_stationsCount = nStations;
    
    MuonTrackExtrapolation extrapolateTrack(Track& track)
    {
        MuonTrackExtrapolation extrapolation;
        State state = track.closestState(0);
        for (unsigned station = 0; station != m_stationsCount; ++station) {
            extrapolation.emplace_back(state.x + state.tx * (m_stationZ[station] - state.z),
                                       state.y + state.ty * (m_stationZ[station] - state.z));
        }
        return extrapolation;
    }
};


struct CommonConstMuonHit
{
    CommonConstMuonHit()
    {
        tile = rand()%10;
        x = float(rand());
        dx = float(rand());
        y = float(rand());
        dy = float(rand());
        time = rand();
        deltaTime = rand();
        uncrossed = rand()%2;
    }
    
    LHCbID tile;
    float x;
    float dx;
    float y;
    float dy;
    int time;
    int deltaTime;
    int uncrossed;
};
using CommonConstMuonHits = std::vector<CommonConstMuonHit>;


std::vector<double> calcBDT(Track &muTrack,
             CommonConstMuonHits &hits)
{
    // features
    std::vector<double> times, dts, cross, resX, resY, minDist, distSeedHit;
    
    // let's start
    CommonMuonTool muTool;
    const auto extrapolation = muTool.extrapolateTrack(muTrack);
    
    for( unsigned int st = 0; st != nStations; ++st ){
        times.push_back(-10000.);
        dts.push_back(-10000.);
        cross.push_back(0.);
        resX.push_back(-10000.);
        resY.push_back(-10000.);
        minDist.push_back(1e10);
        distSeedHit.push_back(1e6);
    }

    std::vector<LHCbID> closestHits(nStations);
    unsigned s = 0;
    CommonConstMuonHits::iterator ih;
    for (auto ih : hits) {
        const LHCbID id = ih.tile;
        s = (++s)%nStations;
        distSeedHit[s] = (ih.x - extrapolation[s].first)*(ih.x - extrapolation[s].first) + (ih.y - extrapolation[s].second)*(ih.y - extrapolation[s].second);
        if(distSeedHit[s] < minDist[s]) {
            minDist[s] = distSeedHit[s];
            closestHits[s] = id;
        }
    };
    
    float commonFactor = MSFACTOR/muTrack.p;
    for( unsigned int st = 0; st != nStations; ++st ){
        unsigned s = 0;
        LHCbID idFromTrack = closestHits[st];
        for (auto ih : hits){
            LHCbID idFromHit = ih.tile;
            if (idFromHit == idFromTrack) {
                s = (++s)%nStations;
                times[s] = ih.time;
                dts[s] = ih.deltaTime;
                (ih.uncrossed==0) ? cross[s] = 2. : cross[s] = ih.uncrossed;
                float travDist = sqrt((muTool.m_stationZ[s]-muTool.m_stationZ[0])*(muTool.m_stationZ[s]-muTool.m_stationZ[0])+
                                      (extrapolation[s].first-extrapolation[0].first)*(extrapolation[s].first-extrapolation[0].first)+
                                      (extrapolation[s].second-extrapolation[0].second)*(extrapolation[s].second-extrapolation[0].second));
                float errMS = commonFactor*travDist*sqrt(travDist)*0.23850119787527452;
                if(std::abs(extrapolation[s].first-ih.x)!=2000){
                    resX[s] = (extrapolation[s].first-ih.x)/sqrt((ih.dx*INVSQRT3)*(ih.dx*INVSQRT3)+errMS*errMS);
                }
                if(std::abs(extrapolation[s].second-ih.y)!=2000){
                    resY[s] = (extrapolation[s].second-ih.y)/sqrt((ih.dy*INVSQRT3)*(ih.dy*INVSQRT3)+errMS*errMS);
                }
            }
        };
    }
    
    
    std::vector<double> Input = {dts[0],dts[1],dts[2],dts[3],
        times[0],times[1],times[2],times[3],
        cross[0],cross[1],cross[2],cross[3],
        resX[0],resX[1],resX[2],resX[3],
        resY[0],resY[1],resY[2],resY[3]};

    return Input;
}

int main()
{
    auto muTrack = new Track;
    auto hits = new CommonConstMuonHits;
    for (auto i : calcBDT(*muTrack, *hits))
    {
        std::cout << i << " ";
    }
    std::cout << std::endl;
    return 0;
}

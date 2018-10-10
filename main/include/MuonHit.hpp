#ifndef MuonHits_h
#define MuonHits_h

class MuonHit
{
    MuonHit(unsigned int t, int dt, bool uncros)
    : time(t), deltaTime(dt), uncrossed(uncros)
    {}
    
public:
    unsigned int time;
    
    int deltaTime;
    
    bool uncrossed;
};

#endif

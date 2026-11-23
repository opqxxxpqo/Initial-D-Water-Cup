#pragma once
#include <cmath>
#include "water_core.h"

namespace cup {
struct Window {
    // Every fixed cup element and all possible projected surface heights fit here.
    int x0=56,y0=40,x1=184,y1=216; // Exclusive upper bounds.
    void include(int left,int top,int right,int bottom) {
        if(left<x0)x0=left;if(top<y0)y0=top;
        if(right>x1)x1=right;if(bottom>y1)y1=bottom;
    }
    void include(const Window &other) {include(other.x0,other.y0,other.x1,other.y1);}
    void clip() {
        if(x0<0)x0=0;if(y0<0)y0=0;
        if(x1>240)x1=240;if(y1>240)y1=240;
    }
    uint32_t pixels() const {return uint32_t(x1-x0)*uint32_t(y1-y0);}
};

inline Window visibleWindow(const spill::Water &water) {
    Window result;
    for(int i=0;i<water.count;++i){
        const spill::Drop &p=water.drops[i];
        const int x=int(std::round(120.0+p.x*(54.0/44.0)));
        const int y=int(std::round(188.0+p.y*(54.0/44.0)*.24-p.z*1.22));
        // Includes the up-to-3-pixel droplet trail and the drawn 1px drop.
        result.include(x-6,y-6,x+7,y+7);
    }
    result.clip();
    return result;
}
} // namespace cup

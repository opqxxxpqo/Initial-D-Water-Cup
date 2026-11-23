#pragma once
// Port of dont-spill/src/renderer.ts and colors.ts (default palette).
// The 24-node physics, 96-point periodic surface, projected tapered cup,
// layer order, pixel glints and alpha values follow the current Web renderer.
#include <cmath>
#include <cstdint>
#include "water_core.h"

namespace cup {
struct Point { float x, y; };

class Renderer {
public:
    explicit Renderer(uint16_t *rgb565ByteSwapped) : pixels_(rgb565ByteSwapped) {
        for (int i = 0; i < 24; ++i) {
            const float t = float(i) * (6.283185307179586f / 24.f);
            cos_[i] = std::cos(t);
            sin_[i] = std::sin(t);
        }
    }

    void draw(const spill::Water &water) {
        for (int i = 0; i < 240 * 240; ++i) pixels_[i] = 0; // Web default: #000000
        Point top[24], bottom[24], base[24], coarse[24], surface[96], poly[128], inner[24];
        ring(100.f, top); ring(0.f, bottom); ring(-8.f, base);
        float velocity = 0;
        for (int i = 0; i < 24; ++i) {
            const float v = std::abs(float(water.v[i]));
            if (v > velocity) velocity = v;
        }
        const double simDelta = water.time - lastSimTime_;
        if (simDelta < 0) { envelope_ = 0; phase_ = 0; }
        else if (simDelta > 0) {
            const float dt = float(simDelta > .1 ? .1 : simDelta);
            const float target = minimum(18.f, velocity * .23f);
            envelope_ += (target - envelope_) * (1.f - std::exp(-dt * (target > envelope_ ? 9.f : 2.2f)));
            if (envelope_ < .005f && velocity < .005f) envelope_ = 0;
            phase_ += dt * (.75f + minimum(1.f, envelope_ / 18.f) * .65f);
        }
        lastSimTime_ = water.time;
        for (int i = 0; i < 24; ++i) {
            const float t = float(i) * (6.283185307179586f / 24.f);
            const float noise = envelope_ * (
                .66f * std::sin(2.f*t + phase_ + .25f*std::sin(phase_*.43f)) +
                .25f * std::sin(3.f*t - phase_*.61f + .8f) +
                .09f * std::sin(5.f*t + phase_*.39f));
            const float h = clamp(float(water.level*100 + water.h[i]) + noise, 0.f, 100.f);
            const float radius = 36.f + h*.18f;
            coarse[i] = {120.f + cos_[i]*radius, 188.f - h*1.22f + sin_[i]*radius*.24f};
        }
        // Periodic cubic interpolation: 4 segments per water node, as in Web.
        for (int j = 0; j < 96; ++j) {
            const int i = j / 4; const float f = (j % 4) * .25f;
            const Point &a = coarse[(i+23)%24], &b = coarse[i];
            const Point &d = coarse[(i+1)%24], &e = coarse[(i+2)%24];
            surface[j] = {catmull(a.x,b.x,d.x,e.x,f), catmull(a.y,b.y,d.y,e.y,f)};
        }

        drawDrops(water, false);
        int n = 0;
        for (int i=12;i<24;++i) poly[n++]=top[i];
        poly[n++]=top[0]; poly[n++]=bottom[0];
        for (int i=23;i>=12;--i) poly[n++]=bottom[i];
        fillPoly(poly,n,GLASS,20); // rear glass, alpha .08
        if (water.level > 0) {
            n=0;
            for(int i=48;i<96;++i) poly[n++]=surface[i];
            poly[n++]=surface[0]; poly[n++]=bottom[0];
            for(int i=23;i>=12;--i) poly[n++]=bottom[i];
            fillPoly(poly,n,WATER,36); // rear water .14
            n=0;
            for(int i=0;i<=48;++i) poly[n++]=surface[i];
            for(int i=12;i>=0;--i) poly[n++]=bottom[i];
            fillPoly(poly,n,WATER,71); // front water .28
            // Preserve the front-water mask for deterministic subpixel glints.
            Point front[62]; const int frontN=n;
            for(int i=0;i<n;++i) front[i]=poly[i];
            fillPoly(surface,96,SURFACE,191); // plane .75
            chain(surface+48,48,HIGHLIGHT,184,true);
            thickLine(surface[95],surface[0],HIGHLIGHT,184);
            chain(surface,49,HIGHLIGHT,250,true);
            const uint8_t glintAlpha = uint8_t(41 + minimum(1.f,velocity/80.f)*41);
            for(int y=76;y<189;y+=4) for(int x=77;x<164;x+=5)
                if((x*13+y*19)%43<3 && inside(front,frontN,float(x)+.5f,float(y)+.5f))
                    pixel(x,y,HIGHLIGHT,glintAlpha);
            static constexpr int glints[5]={2,5,9,14,19};
            for(int k=0;k<5;++k){const Point &p=surface[glints[k]*4];
                rect(iround(p.x+2),iround(p.y-2),k==1?2:1,1,HIGHLIGHT,166);}
        }

        Point edge[3] = {top[12],bottom[12],base[12]}; chain(edge,3,OUTLINE,204);
        edge[0]=top[0];edge[1]=bottom[0];edge[2]=base[0];chain(edge,3,OUTLINE,204);
        edge[0]=top[13];edge[1]=bottom[13];chain(edge,2,REFLECTION,230);
        edge[0]=top[23];edge[1]=bottom[23];chain(edge,2,SECONDARY,204);
        n=0;for(int i=0;i<=12;++i)poly[n++]=bottom[i];
        for(int i=12;i>=0;--i)poly[n++]=base[i];
        fillPoly(poly,n,BASE_SHADOW,191);
        fillPoly(base,24,BASE,219); fillPoly(bottom,24,BASE,179);
        chain(base,13,REFLECTION,191); chain(bottom+12,12,SECONDARY,179);
        rect(96,190,13,1,SECONDARY);rect(109,198,17,1,REFLECTION);
        rect(132,192,10,1,SECONDARY);rect(100,205,25,1,BASE_SHADOW);
        const Point streak1[3]={{82,78},{85,105},{89,143}};chain(streak1,3,REFLECTION,140);
        const Point streak2[2]={{90,148},{93,170}};chain(streak2,2,REFLECTION,77);
        const Point streak3[2]={{113,82},{114,125}};chain(streak3,2,REFLECTION,102);
        const Point streak4[2]={{115,132},{115,176}};chain(streak4,2,REFLECTION,179);
        const Point streak5[3]={{154,83},{151,114},{148,139}};chain(streak5,3,SECONDARY,115);
        const Point streak6[2]={{147,147},{144,174}};chain(streak6,2,SECONDARY,77);
        Point rimBack[14];n=0;rimBack[n++]=top[12];
        for(int i=13;i<24;++i)rimBack[n++]=top[i];rimBack[n++]=top[0];
        chain(rimBack,n,OUTLINE,166);chain(top,13,RIM,242);
        for(int i=0;i<24;++i)inner[i]={120.f+(top[i].x-120.f)*.92f,62.f+(top[i].y-62.f)*.7f};
        chain(inner,24,OUTLINE,166); line(inner[23],inner[0],OUTLINE,166);
        rect(112,56,8,1,REFLECTION,242);rect(88,64,4,1,REFLECTION,242);
        rect(142,65,3,1,REFLECTION,242);rect(107,81,3,1,REFLECTION,242);
        drawDrops(water, true);
        drawRearOverflow(water);
    }

private:
    static constexpr uint16_t WATER=0x73AE, SURFACE=0x632C, HIGHLIGHT=0xFFFF;
    static constexpr uint16_t DROPLET=0xF79E, GLASS=0xA514, OUTLINE=0x73AE;
    static constexpr uint16_t RIM=0xE71C, REFLECTION=0xF79E, BASE=0x528A;
    static constexpr uint16_t BASE_SHADOW=0x3186, SECONDARY=0xAD55;
    uint16_t *pixels_;
    float cos_[24],sin_[24],envelope_=0,phase_=0;
    double lastSimTime_=0;
    static float minimum(float a,float b){return a<b?a:b;}
    static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
    static int iround(float x){return int(std::round(x));}
    static uint16_t swap(uint16_t c){return uint16_t((c<<8)|(c>>8));}
    static float catmull(float a,float b,float d,float e,float f){
        return .5f*(2*b+(-a+d)*f+(2*a-5*b+4*d-e)*f*f+(-a+3*b-3*d+e)*f*f*f);
    }
    void pixel(int x,int y,uint16_t color,uint8_t alpha=255){
        if(unsigned(x)>=240u||unsigned(y)>=240u||alpha==0)return;
        uint16_t &at=pixels_[y*240+x];
        if(alpha==255){at=swap(color);return;}
        const uint16_t old=swap(at);const unsigned inv=255-alpha;
        const unsigned r=(((old>>11)*inv+(color>>11)*alpha+127)/255);
        const unsigned g=((((old>>5)&63)*inv+((color>>5)&63)*alpha+127)/255);
        const unsigned b=(((old&31)*inv+(color&31)*alpha+127)/255);
        at=swap(uint16_t((r<<11)|(g<<5)|b));
    }
    void rect(int x,int y,int w,int h,uint16_t color,uint8_t alpha=255){
        for(int yy=y;yy<y+h;++yy)for(int xx=x;xx<x+w;++xx)pixel(xx,yy,color,alpha);
    }
    void line(Point a,Point b,uint16_t color,uint8_t alpha=255){
        int x=iround(a.x),y=iround(a.y),tx=iround(b.x),ty=iround(b.y);
        const int dx=std::abs(tx-x),dy=-std::abs(ty-y),sx=x<tx?1:-1,sy=y<ty?1:-1;
        int error=dx+dy;
        for(;;){pixel(x,y,color,alpha);if(x==tx&&y==ty)break;
            const int e=2*error;if(e>=dy){error+=dy;x+=sx;}if(e<=dx){error+=dx;y+=sy;}}
    }
    void thickLine(Point a,Point b,uint16_t color,uint8_t alpha){
        line(a,b,color,alpha);
        a.y+=1;b.y+=1;line(a,b,color,uint8_t(alpha/2));
    }
    void chain(const Point *p,int n,uint16_t color,uint8_t alpha=255,bool thick=false){
        for(int i=1;i<n;++i){if(thick)thickLine(p[i-1],p[i],color,alpha);
            else line(p[i-1],p[i],color,alpha);}
    }
    void fillPoly(const Point *p,int n,uint16_t color,uint8_t alpha){
        float minY=240,maxY=0;
        for(int i=0;i<n;++i){if(p[i].y<minY)minY=p[i].y;if(p[i].y>maxY)maxY=p[i].y;}
        int start=int(std::floor(minY)),stop=int(std::ceil(maxY));
        if(start<0)start=0;if(stop>240)stop=240;
        for(int y=start;y<stop;++y){
            const float scan=float(y)+.5f;float xs[128];int count=0;
            for(int i=0,j=n-1;i<n;j=i++)if((p[i].y>scan)!=(p[j].y>scan))
                xs[count++]=p[i].x+(scan-p[i].y)*(p[j].x-p[i].x)/(p[j].y-p[i].y);
            for(int i=1;i<count;++i){const float v=xs[i];int j=i-1;
                while(j>=0&&xs[j]>v){xs[j+1]=xs[j];--j;}xs[j+1]=v;}
            for(int i=0;i+1<count;i+=2){int x=int(std::ceil(xs[i]));int end=int(std::ceil(xs[i+1]));
                if(x<0)x=0;if(end>240)end=240;
                for(;x<end;++x)pixel(x,y,color,alpha);}
        }
    }
    static bool inside(const Point *p,int n,float x,float y){
        bool hit=false;for(int i=0,j=n-1;i<n;j=i++){
            if((p[i].y>y)!=(p[j].y>y) &&
               x<(p[j].x-p[i].x)*(y-p[i].y)/(p[j].y-p[i].y)+p[i].x)hit=!hit;
        }return hit;
    }
    void ring(float z,Point *out){
        const float radius=36.f+z*.18f;
        for(int i=0;i<24;++i)out[i]={120.f+cos_[i]*radius,188.f-z*1.22f+sin_[i]*radius*.24f};
    }
    void drawDrops(const spill::Water &water,bool front){
        for(int i=0;i<water.count;++i){const spill::Drop &p=water.drops[i];
            if((p.y>=0)!=front)continue;
            const float x=120.f+float(p.x)*(54.f/44.f);
            const float y=188.f+float(p.y)*(54.f/44.f)*.24f-float(p.z)*1.22f;
            const float vx=float(p.vx)*(54.f/44.f);
            const float vy=float(p.vy)*(54.f/44.f)*.24f-float(p.vz)*1.22f;
            const float speed=std::sqrt(vx*vx+vy*vy);
            const float length=minimum(3.f,speed*.014f);
            const uint8_t opacity=uint8_t(clamp(float(p.life/.18),0.f,1.f)*255.f);
            if(speed>30.f)line({x-vx/speed*length,y-vy/speed*length},{x,y},DROPLET,uint8_t(opacity*.35f));
            rect(iround(x),iround(y),1,1,DROPLET,opacity);
        }
    }
    void drawRearOverflow(const spill::Water &water){
        for(int i=0;i<water.count;++i){const spill::Drop &p=water.drops[i];
            if(p.y>=0)continue;
            const float x=120.f+float(p.x)*(54.f/44.f);
            const float y=188.f+float(p.y)*(54.f/44.f)*.24f-float(p.z)*1.22f;
            if(y>=62.f)continue; // Only droplets above the rear rim cross in front of its outline.
            const uint8_t opacity=uint8_t(clamp(float(p.life/.18),0.f,1.f)*255.f);
            rect(iround(x),iround(y),2,2,DROPLET,opacity);
        }
    }
};
} // namespace cup

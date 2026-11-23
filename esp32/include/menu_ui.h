#pragma once
#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace cup {
class MenuUi {
public:
    static constexpr int sliderX0=32,sliderX1=210,sliderY0=154,sliderY1=169;
    static constexpr float minGain=.6f,maxGain=3.8f;
    explicit MenuUi(uint16_t *byteSwappedRgb565):pixels_(byteSwappedRgb565){}

    void draw(float gain){
        for(int i=0;i<240*240;++i)pixels_[i]=0;
        drawResetIcon();drawSlider(gain);
    }
    // Reuse the existing menu glyph on an empty cup without clearing the cup.
    void drawResetHint(){drawResetIcon();}
    void drawSlider(float gain){
        for(int y=149;y<174;++y)for(int x=27;x<215;++x)pixel(x,y,0);
        const int split=sliderX0+int(std::round((limited(gain,minGain,maxGain)-minGain)/(maxGain-minGain)*(sliderX1-sliderX0)));
        for(int y=sliderY0;y<sliderY1;++y)for(int x=sliderX0;x<sliderX1;++x){
            const float px=float(x)+.5f,py=float(y)+.5f;
            float cx=px;
            if(cx<39.5f)cx=39.5f;if(cx>202.5f)cx=202.5f;
            const float dx=px-cx,dy=py-161.5f;
            if(dx*dx+dy*dy>7.5f*7.5f)continue;
            int shade=x<split?95:222;
            if(y<157)shade-=14-(y-sliderY0)*3; // Inset shadow near the top edge.
            else if(y>165)shade+=3;
            if(shade<0)shade=0;if(shade>255)shade=255;
            pixel(x,y,gray(uint8_t(shade)));
        }
    }
    static float gainAt(int x){
        const float t=limited(float(x-sliderX0)/float(sliderX1-sliderX0),0.f,1.f);
        return minGain+t*(maxGain-minGain);
    }
    static bool sliderHit(int x,int y){return x>=27&&x<=215&&y>=144&&y<=180;}
    static bool resetHit(int x,int y){return x>=94&&x<=146&&y>=60&&y<=116;}
private:
    struct Point{float x,y;};
    uint16_t *pixels_;
    static float limited(float v,float lo,float hi){return v<lo?lo:v>hi?hi:v;}
    static uint16_t gray(uint8_t value){return uint16_t(((value>>3)<<11)|((value>>2)<<5)|(value>>3));}
    static uint16_t swap(uint16_t v){return uint16_t((v<<8)|(v>>8));}
    void pixel(int x,int y,uint16_t color){
        if(unsigned(x)<240u&&unsigned(y)<240u)pixels_[y*240+x]=swap(color);
    }
    void lightPixel(int x,int y,uint16_t color){
        if(unsigned(x)>=240u||unsigned(y)>=240u)return;
        uint16_t &current=pixels_[y*240+x];
        if(swap(current)<color)current=swap(color);
    }
    void disk(float cx,float cy,float radius){
        const int x0=int(std::floor(cx-radius-1)),x1=int(std::ceil(cx+radius+1));
        const int y0=int(std::floor(cy-radius-1)),y1=int(std::ceil(cy+radius+1));
        for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){
            const float dx=float(x)+.5f-cx,dy=float(y)+.5f-cy;
            const float coverage=limited(radius+.5f-std::sqrt(dx*dx+dy*dy),0.f,1.f);
            if(coverage>0)lightPixel(x,y,gray(uint8_t(coverage*255.f)));
        }
    }
    void stroke(Point a,Point b,float width){
        const float dx=b.x-a.x,dy=b.y-a.y;
        const int steps=int(std::ceil(std::sqrt(dx*dx+dy*dy)*2.f));
        for(int i=0;i<=steps;++i){const float t=steps?float(i)/steps:0.f;
            disk(a.x+dx*t,a.y+dy*t,width*.5f);}
    }
    void quadratic(Point a,Point control,Point b){
        Point last=a;
        for(int i=1;i<=32;++i){const float t=float(i)/32.f,u=1.f-t;
            Point next={u*u*a.x+2*u*t*control.x+t*t*b.x,
                        u*u*a.y+2*u*t*control.y+t*t*b.y};
            stroke(last,next,3.6f);last=next;
        }
    }
    static float edge(Point a,Point b,float x,float y){return (x-a.x)*(b.y-a.y)-(y-a.y)*(b.x-a.x);}
    void triangle(Point a,Point b,Point c){
        int x0=int(std::floor(a.x)),x1=int(std::ceil(a.x));
        int y0=int(std::floor(a.y)),y1=int(std::ceil(a.y));
        for(Point p:{b,c}){if(p.x<x0)x0=int(std::floor(p.x));if(p.x>x1)x1=int(std::ceil(p.x));
            if(p.y<y0)y0=int(std::floor(p.y));if(p.y>y1)y1=int(std::ceil(p.y));}
        for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){
            const float px=float(x)+.5f,py=float(y)+.5f;
            const float e0=edge(a,b,px,py),e1=edge(b,c,px,py),e2=edge(c,a,px,py);
            if((e0>=0&&e1>=0&&e2>=0)||(e0<=0&&e1<=0&&e2<=0))pixel(x,y,0xffff);
        }
    }
    void drawResetIcon(){
        quadratic({105,84},{119,64},{136,82});
        triangle({102,87},{104,76},{114,83});
        quadratic({135,92},{120,111},{104,94});
        triangle({138,96},{127,91},{135,85});
    }
};
} // namespace cup

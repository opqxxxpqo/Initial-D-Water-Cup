#pragma once
#include <cmath>
#include <cstdint>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
namespace spill {
struct MotionInput { uint64_t timestampUs=0; double xG=0,yG=0,zG=0; bool valid=true; }; // g, calibrated dynamic vehicle frame; z reserved
struct Drop { double x,y,z,vx,vy,vz,life; };
struct Water {
 static constexpr int N=24,MaxDrops=72;
 double emitWait=0,nodeWait[N]={};uint32_t seed=12345;
 double random(){seed=seed*1664525u+1013904223u;return double(seed)/4294967296.0;}
 double h[N]={},v[N]={},level=.75,spilled=0,time=0;Drop drops[MaxDrops];int count=0;
 static double min(double a,double b){return a<b?a:b;} static double max(double a,double b){return a>b?a:b;}
 void reset(){emitWait=0;seed=12345;for(int i=0;i<N;i++)nodeWait[i]=0;for(int i=0;i<N;i++)h[i]=v[i]=0;level=.75;spilled=time=0;count=0;}
 void update(const MotionInput& m){bool ok=m.valid&&std::isfinite(m.xG)&&std::isfinite(m.yG);step(ok?m.xG:0,ok?m.yG:0);}
 void step(double x,double y,double dt=1.0/120){
 emitWait-=dt;for(int i=0;i<N;i++)nodeWait[i]-=dt;
 time+=dt;double old[N];for(int i=0;i<N;i++)old[i]=h[i];
 for(int i=0;i<N;i++){double t=double(i)/N*3.141592653589793*2;double target=-44*(x*std::cos(t)+y*std::sin(t));double force=30*(target-old[i])-6*v[i]+40*(old[(i+1)%N]+old[(i+N-1)%N]-2*old[i]);v[i]+=force*dt;h[i]+=v[i]*dt;}
 double mean=0,vm=0;for(int i=0;i<N;i++){mean+=h[i];vm+=v[i];}mean/=N;vm/=N;for(int i=0;i<N;i++){h[i]-=mean;v[i]-=vm;}
 for(int i=0;i<N;i++){double above=level*100+h[i]-100;if(above>0&&level>0){double loss=min(level,above*dt*.018/N);level-=loss;spilled+=loss;if(count<MaxDrops&&emitWait<=0&&nodeWait[i]<=0){double r=random(),q=random(),j=random();double t=double(i)/N*3.141592653589793*2+(r-.5)*.12;double speed=min(90,16+above*.8+max(0,v[i])*.08),tangent=(q-.5)*22;
 drops[count++]={44*std::cos(t),44*std::sin(t),100,std::cos(t)*speed-std::sin(t)*tangent,std::sin(t)*speed+std::cos(t)*tangent,min(85,12+max(0,v[i])*.13)*(.7+j*.6),.6+r*.3};emitWait=.018;nodeWait[i]=.085;}}}

 int live=0;for(int i=0;i<count;i++){Drop p=drops[i];p.x+=p.vx*dt;p.y+=p.vy*dt;p.vz-=180*dt;p.z+=p.vz*dt;p.life-=dt;if(p.life>0)drops[live++]=p;}count=live;
 double mh=0,mv=0;for(int i=0;i<N;i++){mh=max(mh,std::abs(h[i]));mv=max(mv,std::abs(v[i]));}if(x==0&&y==0&&mh<.002&&mv<.005)for(int i=0;i<N;i++)h[i]=v[i]=0;
 }
};
}

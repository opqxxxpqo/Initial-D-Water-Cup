import type {MotionInput} from './motion_input';
export const config={nodes:24,initial:.75,height:100,radius:44,spring:30,damping:6,spread:40};
export const splashConfig={globalGap:.018,nodeGap:.085,maxDrops:72,gravity:180};
export class Water{
 emitWait=0;nodeWait=Array(config.nodes).fill(0);seed=12345;
 private random(){this.seed=(Math.imul(this.seed,1664525)+1013904223)>>>0;return this.seed/4294967296;}
 h=Array(config.nodes).fill(0);v=Array(config.nodes).fill(0);level=config.initial;spilled=0;time=0;
 drops:{x:number,y:number,z:number,vx:number,vy:number,vz:number,life:number}[]=[];
 reset(){this.emitWait=0;this.nodeWait.fill(0);this.seed=12345;this.h.fill(0);this.v.fill(0);this.level=config.initial;this.spilled=0;this.time=0;this.drops=[];}
 update(input:MotionInput){const ok=input.valid&&Number.isFinite(input.xG)&&Number.isFinite(input.yG);this.step(ok?input.xG:0,ok?input.yG:0);}
 step(x:number,y:number,_bump=0,dt=1/120){
 this.emitWait-=dt;for(let i=0;i<config.nodes;i++)this.nodeWait[i]-=dt;
 this.time+=dt;const n=config.nodes,old=[...this.h];
 for(let i=0;i<n;i++){const t=i/n*Math.PI*2;const target=-44*(x*Math.cos(t)+y*Math.sin(t));const force=config.spring*(target-old[i])-config.damping*this.v[i]+config.spread*(old[(i+1)%n]+old[(i+n-1)%n]-2*old[i]);this.v[i]+=force*dt;this.h[i]+=this.v[i]*dt;}
 const mean=this.h.reduce((a,b)=>a+b,0)/n,vm=this.v.reduce((a,b)=>a+b,0)/n;this.h=this.h.map(a=>a-mean);this.v=this.v.map(a=>a-vm);
 // Overflow depends on simulated rim contact, never an acceleration threshold.
 for(let i=0;i<n;i++){const above=this.level*config.height+this.h[i]-config.height;if(above>0&&this.level>0){const loss=Math.min(this.level,above*dt*.018/n);this.level-=loss;this.spilled+=loss;if(this.drops.length<splashConfig.maxDrops&&this.emitWait<=0&&this.nodeWait[i]<=0){
 const r=this.random(),q=this.random(),j=this.random();
 const t=i/n*Math.PI*2+(r-.5)*.12;
 const speed=Math.min(90,16+above*.8+Math.max(0,this.v[i])*.08);
 const tangent=(q-.5)*22;
 this.drops.push({x:44*Math.cos(t),y:44*Math.sin(t),z:100,
 vx:Math.cos(t)*speed-Math.sin(t)*tangent,vy:Math.sin(t)*speed+Math.cos(t)*tangent,
 vz:Math.min(85,12+Math.max(0,this.v[i])*.13)*(.7+j*.6),life:.6+r*.3});
 this.emitWait=splashConfig.globalGap;this.nodeWait[i]=splashConfig.nodeGap;
 }}}

 this.drops=this.drops.filter(p=>{p.x+=p.vx*dt;p.y+=p.vy*dt;p.vz-=splashConfig.gravity*dt;p.z+=p.vz*dt;p.life-=dt;return p.life>0;});
 if(x===0&&y===0&&Math.max(...this.h.map(Math.abs))<.002&&Math.max(...this.v.map(Math.abs))<.005){this.h.fill(0);this.v.fill(0);}
 }
}

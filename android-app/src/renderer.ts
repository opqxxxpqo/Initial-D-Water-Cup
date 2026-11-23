import {Water,config as c} from './physics';
import {DEFAULT_COLORS,type ColorScheme} from './colors';

let palette:ColorScheme={...DEFAULT_COLORS};
export function setRenderColors(colors:ColorScheme){palette={...colors};}
const raster=document.createElement('canvas');raster.width=raster.height=240;
type Point=[number,number];
let noiseEnvelope=0;
let lastSimulationTime=0;
let flowPhase=0;

export function draw(target:CanvasRenderingContext2D,water:Water){
 const ctx=raster.getContext('2d')!;
 ctx.imageSmoothingEnabled=false;
 const px=(x:number,y:number,color:string,size=1,opacity=1)=>{ctx.globalAlpha=opacity;ctx.fillStyle=color;ctx.fillRect(Math.round(x),Math.round(y),size,size);ctx.globalAlpha=1};
 const poly=(points:Point[],color:string,opacity=1)=>{ctx.globalAlpha=opacity;ctx.fillStyle=color;ctx.beginPath();points.forEach(([x,y],i)=>i?ctx.lineTo(x,y):ctx.moveTo(x,y));ctx.closePath();ctx.fill();ctx.globalAlpha=1};
 const line=(a:Point,b:Point,color:string,opacity=1)=>{let x=Math.round(a[0]),y=Math.round(a[1]);const tx=Math.round(b[0]),ty=Math.round(b[1]);const dx=Math.abs(tx-x),dy=-Math.abs(ty-y),sx=x<tx?1:-1,sy=y<ty?1:-1;let error=dx+dy;for(;;){px(x,y,color,1,opacity);if(x===tx&&y===ty)break;const e=2*error;if(e>=dy){error+=dy;x+=sx}if(e<=dx){error+=dx;y+=sy}}};
 const chain=(points:Point[],color:string,opacity=1)=>{for(let i=1;i<points.length;i++)line(points[i-1],points[i],color,opacity)};
 const smoothChain=(points:Point[],color:string,opacity=1)=>{
  ctx.globalAlpha=opacity;ctx.strokeStyle=color;ctx.lineWidth=2.2;ctx.lineCap='round';ctx.lineJoin='round';
  ctx.beginPath();ctx.moveTo(...points[0]);
  for(let i=1;i<points.length;i++)ctx.lineTo(...points[i]);
  ctx.lineTo(...points[points.length-1]);ctx.stroke();ctx.globalAlpha=1;
 };
 const ring=(z:number):Point[]=>Array.from({length:c.nodes},(_,i)=>{const t=i/c.nodes*Math.PI*2;const radius=36+z*.18;return [120+Math.cos(t)*radius,188-z*1.22+Math.sin(t)*radius*.24]});
 const top=ring(100),bottom=ring(0),base=ring(-8);
 const velocity=Math.max(...water.v.map(Math.abs));
 const simDelta=water.time-lastSimulationTime;
 if(simDelta<0){noiseEnvelope=0;flowPhase=0;}
 else if(simDelta>0){
  const dt=Math.min(simDelta,.1),target=Math.min(18,velocity*.23);
  // Gradual attack and longer release prevent the outline from snapping
  // whenever the main slosh changes direction.
  noiseEnvelope+=(target-noiseEnvelope)*(1-Math.exp(-dt*(target>noiseEnvelope?5:6)));
  if(noiseEnvelope<.005&&velocity<.005)noiseEnvelope=0;
  flowPhase+=dt*(.75+Math.min(1,noiseEnvelope/18)*.65);
 }
 lastSimulationTime=water.time;
 const noiseStrength=noiseEnvelope;
 const phase=flowPhase;
 const coarse:Point[]=Array.from({length:c.nodes},(_,i)=>{
  const t=i/c.nodes*Math.PI*2;
  // Several low-frequency modes turn the simulated wave energy into an
  // irregular contour. At rest their amplitude reaches zero exactly.
  const noise=noiseStrength*(.66*Math.sin(2*t+phase+.25*Math.sin(phase*.43))+.25*Math.sin(3*t-phase*.61+.8)+.09*Math.sin(5*t+phase*.39));
  const height=Math.max(0,Math.min(100,water.level*100+water.h[i]+noise));
  const radius=36+height*.18;
  return [120+Math.cos(t)*radius,188-height*1.22+Math.sin(t)*radius*.24];
 });
 // Periodic cubic interpolation gives fills and outlines the same continuous
 // contour, including the left/right joins of the projected surface.
 const surface:Point[]=Array.from({length:96},(_,j)=>{
  const u=j/4,i=Math.floor(u),f=u-i;
  const a=coarse[(i+23)%24],b=coarse[i],d=coarse[(i+1)%24],e=coarse[(i+2)%24];
  return [0,1].map(k=>.5*((2*b[k])+(-a[k]+d[k])*f+(2*a[k]-5*b[k]+4*d[k]-e[k])*f*f+(-a[k]+3*b[k]-3*d[k]+e[k])*f*f*f)) as Point;
 });
 const drops=(front:boolean)=>{for(const p of water.drops){if((p.y>=0)!==front)continue;const x=120+p.x*(54/44),y=188+p.y*(54/44)*.24-p.z*1.22;const vx=p.vx*(54/44),vy=p.vy*(54/44)*.24-p.vz*1.22;const speed=Math.hypot(vx,vy),length=Math.min(3,speed*.014);const opacity=Math.min(1,p.life/.18);if(speed>30)line([x-vx/speed*length,y-vy/speed*length],[x,y],palette.droplet,opacity*.35);px(x,y,palette.droplet,1,opacity);}};

 ctx.fillStyle=palette.background;ctx.fillRect(0,0,240,240);
 drops(false);
 // The rear wall and water volume are quiet so the one-pixel contours stay crisp.
 poly([...top.slice(12),top[0],bottom[0],...bottom.slice(12).reverse()],palette.glass,.08);
 if(water.level>0){
  const back=[...surface.slice(48),surface[0],bottom[0],...bottom.slice(12).reverse()] as Point[];
  const front=[...surface.slice(0,49),...bottom.slice(0,13).reverse()] as Point[];
  poly(back,palette.water,.14);poly(front,palette.water,.28);
  poly(surface,palette.surface,.75);
  // Both visible sides of the water plane rise and fall with the same
  // irregular contour; there is no separate curling crest.
  smoothChain([...surface.slice(48),surface[0]],palette.waterHighlight,.72);
  smoothChain(surface.slice(0,49),palette.waterHighlight,.98);
  // Small deterministic glints reveal motion without changing the water physics.
  const motion=Math.min(1,Math.max(...water.v.map(Math.abs))/80);
  ctx.save();ctx.beginPath();front.forEach(([x,y],i)=>i?ctx.lineTo(x,y):ctx.moveTo(x,y));ctx.closePath();ctx.clip();
  for(let y=76;y<189;y+=4)for(let x=77;x<164;x+=5){const n=(x*13+y*19)%43;if(n<3)px(x,y,palette.waterHighlight,1,.16+motion*.16);}
  ctx.restore();
  for(const i of [2,5,9,14,19]){const [x,y]=surface[i*4];px(x+2,y-2,palette.waterHighlight,i===5?2:1,.65);}
 }
 // Narrow, slightly broken glass edges.
 chain([top[12],bottom[12],base[12]],palette.outline,.8);
 chain([top[0],bottom[0],base[0]],palette.outline,.8);
 chain([top[13],bottom[13]],palette.reflection,.9);
 chain([top[23],bottom[23]],palette.secondary,.8);
 poly([...bottom.slice(0,13),...base.slice(0,13).reverse()],palette.baseShadow,.75);
 poly(base,palette.base,.86);poly(bottom,palette.base,.7);
 chain(base.slice(0,13),palette.reflection,.75);chain(bottom.slice(12),palette.secondary,.7);
 for(const [x,y,w,color] of [[96,190,13,palette.secondary],[109,198,17,palette.reflection],[132,192,10,palette.secondary],[100,205,25,palette.baseShadow]] as const)ctx.fillStyle=color,ctx.fillRect(x,y,w,1);
 // Broken vertical reflections: 1 px, with deliberately empty gaps.
 const streak=(points:Point[],color:string,opacity:number)=>chain(points,color,opacity);
 streak([[82,78],[85,105],[89,143]],palette.reflection,.55);
 streak([[90,148],[93,170]],palette.reflection,.3);
 streak([[113,82],[114,125]],palette.reflection,.4);
 streak([[115,132],[115,176]],palette.reflection,.7);
 streak([[154,83],[151,114],[148,139]],palette.secondary,.45);
 streak([[147,147],[144,174]],palette.secondary,.3);
 chain([top[12],...top.slice(13),top[0]],palette.outline,.65);
 chain(top.slice(0,13),palette.rim,.95);
 const inner=top.map(([x,y]):Point=>[120+(x-120)*.92,62+(y-62)*.7]);
 chain([...inner,inner[0]],palette.outline,.65);
 for(const [x,y,w] of [[112,56,8],[88,64,4],[142,65,3],[107,81,3]])px(x,y,palette.reflection,w,.95);
 drops(true);
 target.imageSmoothingEnabled=false;target.clearRect(0,0,240,240);target.drawImage(raster,0,0);
}

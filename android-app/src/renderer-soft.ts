import {Water,config as c} from './physics';
import {DEFAULT_COLORS, type ColorScheme} from './colors';
// Reference-inspired glass raster. All drawing happens at 120px, enlarged with nearest-neighbour.
let palette: ColorScheme = { ...DEFAULT_COLORS };
export function setRenderColors(colors: ColorScheme){palette={...colors};}
const raster=document.createElement('canvas');raster.width=raster.height=120;
export function draw(target:CanvasRenderingContext2D,water:Water){
 const ctx=raster.getContext('2d')!;ctx.imageSmoothingEnabled=false;
 const poly=(p:number[][],fill:string,stroke?:string)=>{ctx.beginPath();p.forEach(([x,y],i)=>i?ctx.lineTo(Math.round(x),Math.round(y)):ctx.moveTo(Math.round(x),Math.round(y)));ctx.closePath();ctx.fillStyle=fill;ctx.fill();if(stroke){ctx.strokeStyle=stroke;ctx.lineWidth=1;ctx.stroke()}};
 const ring=(z:number,wave=false)=>Array.from({length:c.nodes},(_,i)=>{const t=i/c.nodes*Math.PI*2;const height=wave?Math.max(0,Math.min(100,z+water.h[i])):z;const radius=18+height*.09;return [60+Math.cos(t)*radius,94-height*.61+Math.sin(t)*radius*.24]});
 const line=(p:number[][],color:string)=>{ctx.strokeStyle=color;ctx.lineWidth=1;ctx.beginPath();p.forEach(([x,y],i)=>i?ctx.lineTo(Math.round(x),Math.round(y)):ctx.moveTo(Math.round(x),Math.round(y)));ctx.stroke()};
 const drops=(front:boolean)=>{for(const p of water.drops){
  if((p.y>=0)!==front)continue;
  // Use exactly the cup's orthographic projection at the lip.
  const x=60+p.x*(27/44),y=94+p.y*(27/44)*.24-p.z*.61;
  const vx=p.vx*(27/44),vy=p.vy*(27/44)*.24-p.vz*.61;
  const speed=Math.hypot(vx,vy),length=Math.min(2.5,speed*.025);
  const fade=Math.min(1,p.life/.18);
  ctx.globalAlpha=fade*.8;
  if(speed>24)line([[x-vx/(speed||1)*length,y-vy/(speed||1)*length],[x,y]],palette.secondary);
  ctx.fillStyle=palette.droplet;
  ctx.fillRect(Math.round(x),Math.round(y),1,1);
  ctx.globalAlpha=1;
 }};
 ctx.fillStyle=palette.background;ctx.fillRect(0,0,120,120);
 drops(false);
 const top=ring(100),bottom=ring(0),base=ring(-8),surface=ring(water.level*100,true);
 // Subtle glass shading against a plain black background.
 ctx.globalAlpha=.13;poly([...top.slice(12),top[0],bottom[0],...bottom.slice(12).reverse()],palette.glass);ctx.globalAlpha=1;
 if(water.level>0){
  // Darker rear volume and two light bands make the water read through the glass.
  ctx.globalAlpha=.18;poly([...surface.slice(12),surface[0],bottom[0],...bottom.slice(12).reverse()],palette.water);ctx.globalAlpha=1;
  ctx.globalAlpha=.31;poly([...surface.slice(0,13),...bottom.slice(0,13).reverse()],palette.water);ctx.globalAlpha=1;
  poly(surface,palette.surface);ctx.globalAlpha=.28;poly(surface,palette.waterHighlight);ctx.globalAlpha=1;
  line([...surface,surface[0]],palette.secondary);line(surface.slice(0,13),palette.waterHighlight);
  ctx.fillStyle=palette.waterHighlight;for(const i of [3,5,8,10]){const [x,y]=surface[i];ctx.fillRect(Math.round(x),Math.round(y),i===5?3:2,1)}
 }
 // Left/right walls and thick weighted base.
 line([top[12],bottom[12],base[12]],palette.outline);line([top[0],bottom[0],base[0]],palette.outline);
 poly([...bottom.slice(0,13),...base.slice(0,13).reverse()],palette.baseShadow,palette.outline);
 poly(base,palette.secondary,palette.outline);poly(bottom,palette.base,palette.outline);
 line(base.slice(0,13),palette.reflection);line(bottom.slice(12),palette.secondary);
 const foot=ring(-5);line(foot.slice(0,13),palette.baseShadow);
 ctx.fillStyle=palette.reflection;ctx.fillRect(51,99,11,1);ctx.fillRect(64,98,5,1);
 ctx.fillStyle=palette.baseShadow;ctx.fillRect(49,102,9,1);ctx.fillRect(67,101,5,1);
 // Transparent front face, then fixed vertical reflections.
 ctx.globalAlpha=.07;poly([...top.slice(0,13),...bottom.slice(0,13).reverse()],palette.glass);ctx.globalAlpha=1;
 const shard=(points:number[][],alpha:number)=>{ctx.globalAlpha=alpha;poly(points,palette.reflection);ctx.globalAlpha=1};
 shard([[40,39],[42,41],[47,85],[46,93],[44,88]],.32);
 shard([[57,42],[59,42],[60,97],[58,97]],.23);
 shard([[61,42],[62,42],[62,96],[61,96]],.42);
 shard([[48,42],[49,43],[51,81],[50,84]],.12);
 shard([[80,39],[78,58],[75,83],[75,88],[77,83],[82,39]],.3);
 line([[35,39],[39,68],[42,88]],palette.secondary);line([[84,38],[81,66],[78,87]],palette.secondary);
 // Caustics: small stepped arcs, not a solid opaque foot.
 line([[42,90],[47,87],[52,88]],palette.reflection);line([[65,88],[72,89],[77,92]],palette.secondary);
 line([[43,94],[47,95],[55,96]],palette.secondary);line([[63,95],[72,94],[76,93]],palette.reflection);
 ctx.fillStyle=palette.reflection;ctx.fillRect(56,88,8,1);ctx.fillRect(58,98,5,1);ctx.fillRect(59,101,3,1);
 ctx.fillStyle=palette.background;ctx.fillRect(45,94,8,1);ctx.fillRect(65,97,6,1);
 // The lip is drawn last to keep the silhouette readable.
 line([...top,top[0]],palette.outline);
 const inner=top.map(([x,y])=>[60+(x-60)*.91,31+(y-31)*.70]);
 line([...inner,inner[0]],palette.outline);
 line(top.slice(0,13),palette.rim);line(top.slice(13),palette.outline);
 ctx.fillStyle=palette.reflection;ctx.fillRect(56,28,6,1);ctx.fillRect(43,31,4,1);ctx.fillRect(71,40,4,1);
 drops(true);
 target.imageSmoothingEnabled=false;target.clearRect(0,0,240,240);target.drawImage(raster,0,0,240,240);
}


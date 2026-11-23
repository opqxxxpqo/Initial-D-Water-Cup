import {Water,config as c} from './physics';
export function draw(ctx:CanvasRenderingContext2D,water:Water){
 const project=(x:number,y:number,z:number)=>[120+x,181+y*.38-z*.9];
 const ring=(z:number,surface=false)=>Array.from({length:c.nodes},(_,i)=>{const t=i/c.nodes*Math.PI*2;return project(c.radius*Math.cos(t),c.radius*Math.sin(t),surface?Math.max(0,Math.min(c.height,z+water.h[i])):z)});
 const path=(points:number[][],fill:string,stroke?:string)=>{ctx.beginPath();points.forEach((p,i)=>i?ctx.lineTo(Math.round(p[0]),Math.round(p[1])):ctx.moveTo(Math.round(p[0]),Math.round(p[1])));ctx.closePath();ctx.fillStyle=fill;ctx.fill();if(stroke){ctx.strokeStyle=stroke;ctx.lineWidth=1;ctx.stroke()}};
 ctx.fillStyle='#050909';ctx.fillRect(0,0,240,240);const top=ring(100),bottom=ring(0),surface=ring(water.level*100,true);
 path([...top.slice(12),top[0],bottom[0],...bottom.slice(12).reverse()],'#132126','#49616a');
 if(water.level>0){path([...surface.slice(0,13),...bottom.slice(0,13).reverse()],'#127495');path(surface,'#48c5df','#b4f7ff');for(let i=1;i<12;i+=3){ctx.fillStyle='#e0fbff';ctx.fillRect(surface[i][0]-1,surface[i][1],3,1)}}
 path([...top.slice(0,13),...bottom.slice(0,13).reverse()],'#8bbdcc0c','#7d9da6');
 ctx.beginPath();top.forEach((p,i)=>i?ctx.lineTo(p[0],p[1]):ctx.moveTo(p[0],p[1]));ctx.closePath();ctx.strokeStyle='#d0e3e7';ctx.stroke();ctx.fillStyle='#b0d4df';ctx.fillRect(80,111,1,38);ctx.fillRect(83,116,1,14);
 for(const p of water.drops){const a=project(p.x,p.y,p.z);ctx.fillStyle='#8be7ff';ctx.fillRect(Math.round(a[0]),Math.round(a[1]),2,2)}
}
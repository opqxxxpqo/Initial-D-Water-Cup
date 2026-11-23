import type {MotionInput} from './motion_input';
export class MouseMotionInput {
 private x=0;private y=0;private last={x:0,y:0,t:0};
 begin(x:number,y:number,t:number){this.last={x,y,t};}
 clear(){this.x=0;this.y=0;}
 move(x:number,y:number,t:number,width:number,height:number){const dt=Math.max(8,t-this.last.t);this.x=Math.max(-4,Math.min(4,(x-this.last.x)/width/dt*850));this.y=Math.max(-4,Math.min(4,(y-this.last.y)/height/dt*850));this.last={x,y,t};}
 sample():MotionInput {this.x*=.94;this.y*=.94;if(Math.abs(this.x)<.0001)this.x=0;if(Math.abs(this.y)<.0001)this.y=0;return {timestampUs:this.last.t*1000,xG:this.x,yG:this.y,zG:0,valid:true,source:'mouse'};}
}

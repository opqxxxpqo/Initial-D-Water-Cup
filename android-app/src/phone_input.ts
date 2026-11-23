import { registerPlugin, type PluginListenerHandle } from '@capacitor/core';
import type { MotionInput } from './motion_input';
type SensorSample = Omit<MotionInput,'source'>;
const native = registerPlugin<{start():Promise<{gyroscope:boolean}>;stop():Promise<void>;calibrate():Promise<void>;addListener(event:'motion',callback:(sample:SensorSample)=>void):Promise<PluginListenerHandle>}>('PhoneMotion');
export class PhoneMotionInput {
 private latest:SensorSample|null=null;
 private receivedAt=0;
 private listener:PluginListenerHandle|null=null;
 async start(){this.listener=await native.addListener('motion',sample=>{this.latest=sample;this.receivedAt=performance.now()});return native.start();}
 async stop(){this.latest=null;await native.stop();await this.listener?.remove();this.listener=null;}
 async calibrate(){this.latest=null;await native.calibrate();}
 sample():MotionInput {const value=this.latest,valid=!!value&&performance.now()-this.receivedAt<180&&value.valid&&Number.isFinite(value.xG)&&Number.isFinite(value.yG);return valid?{...value!,source:'imu'}:{timestampUs:0,xG:0,yG:0,zG:0,valid:false,source:'imu'};}
}

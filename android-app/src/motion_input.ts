/** Calibrated vehicle frame. x=screen-right, y=screen-front (pad-down), gravity removed.
 * Dynamic acceleration in g; mouse source is an artistic proxy, not measured g.
 * Adapter owns calibration/filtering. Invalid sample feeds zero. Physics dt=1/120 s.
 */
export interface MotionInput { timestampUs:number; xG:number; yG:number; zG:number; valid:boolean; source:'mouse'|'simulated'|'imu'; }

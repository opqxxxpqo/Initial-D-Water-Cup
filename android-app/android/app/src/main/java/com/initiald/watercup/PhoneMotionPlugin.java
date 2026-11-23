package com.initiald.watercup;
import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.SystemClock;
import com.getcapacitor.JSObject;
import com.getcapacitor.Plugin;
import com.getcapacitor.PluginCall;
import com.getcapacitor.PluginMethod;
import com.getcapacitor.annotation.CapacitorPlugin;
@CapacitorPlugin(name="PhoneMotion")
public class PhoneMotionPlugin extends Plugin implements SensorEventListener {
 private SensorManager sensors;private Sensor accelerometer,gyroscope;
 private boolean active=false,gravityReady=false;
 private final float[] gravity=new float[3],neutral=new float[3],gyro=new float[3];
 private long lastEmitNs=0;
 @Override public void load(){sensors=(SensorManager)getContext().getSystemService(Context.SENSOR_SERVICE);accelerometer=sensors.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);gyroscope=sensors.getDefaultSensor(Sensor.TYPE_GYROSCOPE);}
 @PluginMethod public void start(PluginCall call){if(accelerometer==null){call.reject("手机没有加速度计");return;}active=true;getActivity().runOnUiThread(this::registerSensors);JSObject r=new JSObject();r.put("gyroscope",gyroscope!=null);call.resolve(r);}
 private void registerSensors(){if(!active)return;sensors.registerListener(this,accelerometer,SensorManager.SENSOR_DELAY_GAME);if(gyroscope!=null)sensors.registerListener(this,gyroscope,SensorManager.SENSOR_DELAY_GAME);}
 @PluginMethod public void calibrate(PluginCall call){if(gravityReady)System.arraycopy(gravity,0,neutral,0,3);call.resolve();}
 @PluginMethod public void stop(PluginCall call){active=false;sensors.unregisterListener(this);call.resolve();}
 @Override protected void handleOnPause(){sensors.unregisterListener(this);}
 @Override protected void handleOnResume(){if(active)getActivity().runOnUiThread(this::registerSensors);}
 @Override public void onSensorChanged(SensorEvent event){
  if(event.sensor.getType()==Sensor.TYPE_GYROSCOPE){for(int i=0;i<3;i++)gyro[i]=event.values[i];return;}
  if(event.sensor.getType()!=Sensor.TYPE_ACCELEROMETER)return;
  if(!gravityReady){System.arraycopy(event.values,0,gravity,0,3);System.arraycopy(event.values,0,neutral,0,3);gravityReady=true;}
  for(int i=0;i<3;i++)gravity[i]+=.035f*(event.values[i]-gravity[i]);
  long now=SystemClock.elapsedRealtimeNanos();if(now-lastEmitNs<20_000_000L)return;lastEmitNs=now;
  // Portrait screen x right, y down. Neutral posture comes from the first sample or Calibrate.
  float right=(event.values[0]-gravity[0]+.42f*(gravity[0]-neutral[0]))/SensorManager.GRAVITY_EARTH-.12f*gyro[2];
  float down=-(event.values[1]-gravity[1]+.42f*(gravity[1]-neutral[1]))/SensorManager.GRAVITY_EARTH+.12f*gyro[0];
  if(!Float.isFinite(right)||!Float.isFinite(down))return;
  JSObject sample=new JSObject();sample.put("timestampUs",now/1000L);sample.put("xG",Math.max(-2f,Math.min(2f,right)));sample.put("yG",Math.max(-2f,Math.min(2f,down)));sample.put("zG",0);sample.put("valid",true);notifyListeners("motion",sample);
 }
 @Override public void onAccuracyChanged(Sensor sensor,int accuracy){}
}

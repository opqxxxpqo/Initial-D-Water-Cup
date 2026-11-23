package com.initiald.watercup;
import com.getcapacitor.BridgeActivity;
import android.os.Bundle;
public class MainActivity extends BridgeActivity {
 @Override public void onCreate(Bundle state){registerPlugin(PhoneMotionPlugin.class);super.onCreate(state);}
}

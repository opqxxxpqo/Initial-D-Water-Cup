#include "touch_input.h"
#include "DEV_Config.h"
#include <Wire.h>

namespace {
volatile bool touchPending=false;
void IRAM_ATTR touchInterrupt(){touchPending=true;}
}

namespace cup {
bool TouchInput::read(uint8_t reg,uint8_t *out,size_t length,bool active){
    Wire.beginTransmission(0x15);Wire.write(reg);
    if(Wire.endTransmission(true)!=0){if(active)++errors_;else ++idleNacks_;return false;}
    if(Wire.requestFrom(uint8_t(0x15),static_cast<uint8_t>(length))!=length){
        while(Wire.available())Wire.read();if(active)++errors_;else ++idleNacks_;return false;
    }
    for(size_t i=0;i<length;++i)out[i]=Wire.read();
    return true;
}

bool TouchInput::begin(){
    // DEV_Module_Init already configured the one shared I2C bus on GPIO6/7.
    pinMode(Touch_RST_PIN,OUTPUT);digitalWrite(Touch_RST_PIN,LOW);delay(5);
    digitalWrite(Touch_RST_PIN,HIGH);delay(50);
    pinMode(Touch_INT_PIN,INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(Touch_INT_PIN),touchInterrupt,FALLING);
    ready_=read(0xA7,&chipId_,1);
    return ready_;
}

bool TouchInput::poll(TouchPoint &out){
    const uint32_t now=millis();
    if(uint32_t(now-lastReadMs_)<(tracking_?20u:40u))return false;
    const bool active=tracking_||touchPending;
    touchPending=false; // Bounded polling also recovers short missed IRQs.
    lastReadMs_=now;
    uint8_t raw[6]={};++reads_;
    if(!read(0x01,raw,sizeof raw,active)){
        if(tracking_ && uint32_t(now-lastGoodMs_)>150){
            tracking_=false;out.down=false;out.cancelled=true;out.atMs=now;return true;
        }
        return false;
    }
    const bool down=(raw[1]&0x0f)!=0;
    const uint16_t x=uint16_t((raw[2]&0x0f)<<8)|raw[3];
    const uint16_t y=uint16_t((raw[4]&0x0f)<<8)|raw[5];
    if(down && (x>=240||y>=240)){++errors_;return false;}
    if(!down&&!tracking_)return false;
    out.down=down;out.x=down?x:lastX_;out.y=down?y:lastY_;out.atMs=now;
    if(down){lastX_=x;lastY_=y;lastGoodMs_=now;}
    tracking_=down;
    return true;
}
} // namespace cup

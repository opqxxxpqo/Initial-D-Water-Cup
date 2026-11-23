#pragma once
// QMI8658C input adapter for the verified Waveshare Touch board.
// DEV_Module_Init() already owns Wire.begin() on SDA6/SCL7.
#include <Arduino.h>
#include <Wire.h>
#include <cmath>
#include <initializer_list>
#include "water_core.h"

namespace cup {
class ImuMotion {
public:
    bool begin() {
        Wire.setClock(400000);
        Wire.setTimeOut(20);
        uint8_t id=0;
        for(uint8_t candidate : {uint8_t(0x6B),uint8_t(0x6A)}) {
            if(read(candidate,0x00,&id,1) && id==0x05){address_=candidate;break;}
        }
        if(!address_)return false;
        // Same configuration and scales read back on the HWTEST board:
        // CTRL2=0x23: +/-8g, 4096 LSB/g; CTRL3=0x53: +/-512 dps, 64 LSB/dps.
        const uint8_t regs[]={0x08,0x02,0x03,0x04,0x06,0x08};
        const uint8_t values[]={0x00,0x60,0x23,0x53,0x00,0x03};
        for(unsigned i=0;i<sizeof regs;++i)if(!write(regs[i],values[i]))return false;
        delay(20);
        const uint8_t checkRegs[]={0x02,0x03,0x04,0x06,0x08};
        const uint8_t checkValues[]={0x60,0x23,0x53,0x00,0x03};
        for(unsigned i=0;i<sizeof checkRegs;++i){
            uint8_t actual=0;
            if(!read(address_,checkRegs[i],&actual,1)||actual!=checkValues[i])return false;
        }
        if(!readData())return false;
        for(int i=0;i<3;++i)neutral_[i]=accG_[i];
        ready_=true;
        lastGoodUs_=lastReadUs_=micros();
        return true;
    }

    bool poll(uint32_t nowUs) {
        if(!ready_||uint32_t(nowUs-lastReadUs_)<10000)return false;
        const float dt=lastReadUs_ ? float(uint32_t(nowUs-lastReadUs_))*1e-6f : .01f;
        lastReadUs_=nowUs;
        if(!readData())return false;
        lastGoodUs_=micros();
        const float targetX=limited(-gain_*deadband(accG_[1]-neutral_[1]),-2.5f,2.5f);
        const float backAxis=deadband(accG_[0]-neutral_[0]);
        // Rearward pitch has a small projected X-gravity delta at the user's
        // normal 45-degree grip. Lift its low-angle response before clipping.
        const float targetY=limited(gain_*(backAxis>0?backAxis*3.0f:backAxis),-2.5f,2.5f);
        // Short smoothing removes one-sample noise without making tilt sluggish.
        const float alpha=limited(dt/(.020f+dt),0.f,1.f);
        x_+=alpha*(targetX-x_);y_+=alpha*(targetY-y_);
        return true;
    }

    spill::MotionInput sample(uint32_t nowUs) const {
        spill::MotionInput out;
        out.timestampUs=lastGoodUs_;
        out.valid=ready_&&uint32_t(nowUs-lastGoodUs_)<180000;
        out.xG=out.valid?x_:0;
        out.yG=out.valid?y_:0;
        return out;
    }
    bool ready() const {return ready_;}
    float gain() const {return gain_;}
    void setGain(float value){gain_=limited(value,.6f,3.8f);}
    bool recenter(){
        if(!ready_||!readData())return false;
        for(int i=0;i<3;++i)neutral_[i]=accG_[i];
        x_=y_=0;
        lastGoodUs_=micros();
        return true;
    }
    uint8_t address() const {return address_;}
    uint32_t errors() const {return errors_;}
    const float *accG() const {return accG_;}
    const float *gyroDps() const {return gyroDps_;}
    const float *neutral() const {return neutral_;}

private:
    uint8_t address_=0;
    bool ready_=false;
    uint32_t lastReadUs_=0,lastGoodUs_=0,errors_=0;
    float accG_[3]={},gyroDps_[3]={},neutral_[3]={},x_=0,y_=0,gain_=2.8f;
    static float limited(float v,float lo,float hi){return v<lo?lo:v>hi?hi:v;}
    static float deadband(float v){return v>.012f?v-.012f:v<-.012f?v+.012f:0.f;}
    bool read(uint8_t address,uint8_t reg,uint8_t *out,size_t len){
        Wire.beginTransmission(address);Wire.write(reg);
        if(Wire.endTransmission(true)!=0){++errors_;return false;}
        if(Wire.requestFrom(address,static_cast<uint8_t>(len))!=len){
            while(Wire.available())Wire.read();++errors_;return false;
        }
        for(size_t i=0;i<len;++i)out[i]=Wire.read();
        return true;
    }
    bool write(uint8_t reg,uint8_t value){
        Wire.beginTransmission(address_);Wire.write(reg);Wire.write(value);
        if(Wire.endTransmission()!=0){++errors_;return false;}
        return true;
    }
    bool readData(){
        uint8_t bytes[12]={};
        if(!read(address_,0x35,bytes,sizeof bytes))return false;
        for(int i=0;i<3;++i){
            const int16_t a=static_cast<int16_t>(uint16_t(bytes[2*i])|(uint16_t(bytes[2*i+1])<<8));
            const int16_t g=static_cast<int16_t>(uint16_t(bytes[6+2*i])|(uint16_t(bytes[7+2*i])<<8));
            accG_[i]=a/4096.f;gyroDps_[i]=g/64.f;
            if(!std::isfinite(accG_[i])||!std::isfinite(gyroDps_[i])){++errors_;return false;}
        }
        return true;
    }
};
} // namespace cup

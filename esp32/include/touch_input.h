#pragma once
#include <Arduino.h>

namespace cup {
struct TouchPoint {
    bool down=false;
    bool cancelled=false;
    uint16_t x=0,y=0;
    uint32_t atMs=0;
};

class TouchInput {
public:
    bool begin();
    bool poll(TouchPoint &point);
    bool ready() const {return ready_;}
    uint8_t chipId() const {return chipId_;}
    uint32_t reads() const {return reads_;}
    uint32_t errors() const {return errors_;}
    uint32_t idleNacks() const {return idleNacks_;}
private:
    bool read(uint8_t reg,uint8_t *out,size_t length,bool active=true);
    bool ready_=false,tracking_=false;
    uint8_t chipId_=0;
    uint16_t lastX_=0,lastY_=0;
    uint32_t lastReadMs_=0,lastGoodMs_=0,reads_=0,errors_=0,idleNacks_=0;
};
} // namespace cup

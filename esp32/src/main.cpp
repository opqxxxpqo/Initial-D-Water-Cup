#include <Arduino.h>
#include <cmath>
#include "DEV_Config.h"
#include "LCD_1in28.h"
#include "water_core.h"
#include "water_renderer.h"
#include "motion_source.h"
#include "dirty_window.h"
#include "imu_motion.h"
#include "touch_input.h"
#include "menu_ui.h"

UWORD *BlackImage = nullptr; // Required by the verified Waveshare Touch LCD driver.
static spill::Water water;
static cup::ImuMotion imuInput;
static cup::TouchInput touchInput;
static cup::Renderer *renderer = nullptr;
static cup::MenuUi *menuUi = nullptr;
static cup::Window lastWindow;
static bool menuOpen=false;
static constexpr uint32_t kFrameIntervalUs = 50000; // 20 FPS target; physics stays at 120 Hz.
static constexpr double kPhysicsStep = 1.0 / 120.0;
static constexpr char kBuildId[] = "WATER_CUP_AXIS_TOUCH_20261111_H";

struct Gesture {
    bool down=false,consumed=false,draggingSlider=false;
    int startX=0,startY=0,lastX=0,lastY=0;
    uint32_t startMs=0;
};
static Gesture gesture;

static void showMenu(){
    menuOpen=true;menuUi->draw(imuInput.gain());
    LCD_1IN28_Display(BlackImage);
    Serial.println("MENU open long_press_ms=1000");
}
static void showCup(const char *reason){
    menuOpen=false;
    renderer->draw(water);
    LCD_1IN28_Display(BlackImage); // Clear the old menu over the entire panel.
    lastWindow=cup::Window{};
    Serial.printf("MENU close reason=%s\n",reason);
}
static void updateSlider(int x){
    const float gain=cup::MenuUi::gainAt(x);
    if(std::abs(gain-imuInput.gain())<.015f)return;
    imuInput.setGain(gain);
    menuUi->drawSlider(imuInput.gain());
    LCD_1IN28_DisplayWindows(27,149,215,174,BlackImage);
}
static void processTouch(const cup::TouchPoint &point){
    if(point.cancelled){gesture=Gesture{};return;}
    if(point.down){
        if(!gesture.down){
            gesture.down=true;gesture.consumed=false;
            gesture.startX=point.x;gesture.startY=point.y;
            gesture.startMs=point.atMs;
            gesture.draggingSlider=menuOpen&&cup::MenuUi::sliderHit(point.x,point.y);
        }
        gesture.lastX=point.x;gesture.lastY=point.y;
        if(menuOpen){
            if(gesture.draggingSlider&&!gesture.consumed)updateSlider(point.x);
        }else if(!gesture.consumed){
            const int movement=std::abs(gesture.lastX-gesture.startX)+std::abs(gesture.lastY-gesture.startY);
            const bool onCup=gesture.startX>=56&&gesture.startX<=184&&gesture.startY>=42&&gesture.startY<=216;
            if(movement>18||!onCup)gesture.consumed=true;
            if(onCup&&movement<=18&&uint32_t(point.atMs-gesture.startMs)>=1000){
                gesture.consumed=true;showMenu();
            }
        }
        return;
    }
    if(!gesture.down)return;
    if(menuOpen&&!gesture.consumed){
        const int movement=std::abs(gesture.lastX-gesture.startX)+std::abs(gesture.lastY-gesture.startY);
        if(gesture.draggingSlider){
            Serial.printf("MENU sensitivity=%.2f\n",imuInput.gain());
        }else if(movement<=18&&cup::MenuUi::resetHit(gesture.startX,gesture.startY)){
            water.reset();
            const bool zeroed=imuInput.recenter();
            Serial.printf("MENU reset water=0.75 imu_recenter=%u neutral_g=%+.3f,%+.3f,%+.3f\n",
                unsigned(zeroed),imuInput.neutral()[0],imuInput.neutral()[1],imuInput.neutral()[2]);
            showCup("reset");
        }else if(gesture.startY>=181&&gesture.lastY<=gesture.startY-55){
            showCup("swipe_up");
        }
    }
    gesture=Gesture{};
}

struct Perf {
    uint32_t frames=0, steps=0, imuReads=0, clippedUs=0, droppedSteps=0;
    uint32_t physicsSum=0, physicsMax=0, drawSum=0, drawMax=0;
    uint32_t spiSum=0, spiMax=0, frameSum=0, frameMax=0, uploadPixels=0;
    static void add(uint32_t us,uint32_t &sum,uint32_t &maximum) {
        sum+=us;if(us>maximum)maximum=us;
    }
};
static Perf perf;

void setup() {
    Serial.begin(115200);
    delay(50);
    Serial.printf("BOOT %s built=%s %s\n",kBuildId,__DATE__,__TIME__);
    BlackImage=static_cast<UWORD *>(ps_malloc(240*240*sizeof(UWORD)));
    if(!BlackImage){Serial.println("FATAL framebuffer PSRAM allocation failed");while(true)delay(1000);}
    DEV_Module_Init();
    LCD_1IN28_Init(HORIZONTAL);
    DEV_SET_PWM(100);
    const bool imuReady=imuInput.begin();
    if(imuReady){
        const float *neutral=imuInput.neutral();
        Serial.printf("INPUT IMU addr=0x%02X neutral_g=%+.3f,%+.3f,%+.3f gain=2.8 deadband=0.012g\n",
            imuInput.address(),neutral[0],neutral[1],neutral[2]);
    }else Serial.printf("WARNING IMU init failed errors=%u; INPUT simulated fallback\n",imuInput.errors());
    const bool touchReady=touchInput.begin();
    Serial.printf("TOUCH ready=%u address=0x15 chip_id=0x%02X irq=5 reset=13\n",
        unsigned(touchReady),touchInput.chipId());
    static cup::Renderer frameRenderer(BlackImage);
    renderer=&frameRenderer;
    static cup::MenuUi settingsMenu(BlackImage);
    menuUi=&settingsMenu;
    // The real IMU starts at the board's current orientation. The simulated
    // fallback alone is pre-rolled so it does not open with a frozen frame.
    if(!imuReady)for(int i=0;i<24;++i)water.update(cup::simulatedMotion(water.time));
    renderer->draw(water);
    LCD_1IN28_Display(BlackImage);
    Serial.printf("FIRST_FRAME uptime_ms=%u sim_time=%.3f\n",millis(),water.time);
    Serial.printf("LCD Waveshare Touch driver SPI80MHz 240x240 RGB565; flash=%u psram_total=%u psram_free=%u\n",
        ESP.getFlashChipSize(),ESP.getPsramSize(),ESP.getFreePsram());
    Serial.println(imuReady?"INPUT real QMI8658; relative boot orientation, no factory calibration":"INPUT simulated fallback");
}

void loop() {
    static uint32_t previousUs=micros(),lastFrameUs=previousUs,lastLogUs=previousUs;
    static double accumulator=0;
    const uint32_t now=micros();
    if(imuInput.poll(now))++perf.imuReads;
    cup::TouchPoint touch;
    if(touchInput.poll(touch))processTouch(touch);
    const uint32_t rawElapsed=now-previousUs;
    previousUs=now;
    uint32_t elapsed=rawElapsed;
    // A normal full-screen render takes about 49 ms on this board. Preserve
    // that elapsed time for physics; clamp only genuine long stalls.
    if(elapsed>100000){perf.clippedUs+=elapsed-100000;elapsed=100000;}
    if(menuOpen)accumulator=0;
    else accumulator+=double(elapsed)*1e-6;

    int stepsThisLoop=0;
    while(!menuOpen&&accumulator>=kPhysicsStep && stepsThisLoop<12){
        const uint32_t start=micros();
        water.update(imuInput.ready()?imuInput.sample(micros()):cup::simulatedMotion(water.time));
        Perf::add(micros()-start,perf.physicsSum,perf.physicsMax);
        ++perf.steps;++stepsThisLoop;
        accumulator-=kPhysicsStep;
    }
    if(accumulator>=kPhysicsStep){
        const uint32_t skipped=uint32_t(accumulator/kPhysicsStep);
        perf.droppedSteps+=skipped;
        accumulator=std::fmod(accumulator,kPhysicsStep);
    }

    if(!menuOpen&&uint32_t(now-lastFrameUs)>=kFrameIntervalUs){
        lastFrameUs=now;
        const uint32_t frameStart=micros();
        renderer->draw(water);
        const uint32_t drawEnd=micros();
        cup::Window current=cup::visibleWindow(water);
        cup::Window upload=current;
        upload.include(lastWindow); // Erase pixels occupied by old droplets.
        upload.clip();
        const uint32_t flushStart=micros();
        LCD_1IN28_DisplayWindows(upload.x0,upload.y0,upload.x1,upload.y1,BlackImage);
        lastWindow=current;
        const uint32_t frameEnd=micros();
        Perf::add(drawEnd-frameStart,perf.drawSum,perf.drawMax);
        Perf::add(frameEnd-flushStart,perf.spiSum,perf.spiMax);
        Perf::add(frameEnd-frameStart,perf.frameSum,perf.frameMax);
        perf.uploadPixels+=upload.pixels();
        ++perf.frames;
    }

    if(uint32_t(now-lastLogUs)>=2000000){
        const uint32_t period=now-lastLogUs;
        const spill::MotionInput input=imuInput.sample(micros());
        const float *acc=imuInput.accG(),*gyro=imuInput.gyroDps();
        Serial.printf("PERF fps=%.2f physics_hz=%.1f physics_us_avg=%u physics_us_max=%u draw_us_avg=%u draw_us_max=%u spi_us_avg=%u spi_us_max=%u frame_us_avg=%u frame_us_max=%u upload_px_avg=%u clipped_us=%u dropped_steps=%u imu_reads=%u imu_errors=%u input_valid=%u input_xy=%+.3f,%+.3f acc_g=%+.3f,%+.3f,%+.3f gyro_dps=%+.2f,%+.2f,%+.2f heap=%u psram_free=%u level=%.5f drops=%d mode=%s gain=%.2f touch_reads=%u touch_errors=%u touch_idle_nacks=%u\n",
            perf.frames*1000000.0/period,perf.steps*1000000.0/period,
            perf.steps?perf.physicsSum/perf.steps:0,perf.physicsMax,
            perf.frames?perf.drawSum/perf.frames:0,perf.drawMax,
            perf.frames?perf.spiSum/perf.frames:0,perf.spiMax,
            perf.frames?perf.frameSum/perf.frames:0,perf.frameMax,
            perf.frames?perf.uploadPixels/perf.frames:0,
            perf.clippedUs,perf.droppedSteps,perf.imuReads,imuInput.errors(),unsigned(input.valid),
            input.xG,input.yG,acc[0],acc[1],acc[2],gyro[0],gyro[1],gyro[2],
            ESP.getFreeHeap(),ESP.getFreePsram(),
            water.level,water.count,menuOpen?"menu":"cup",imuInput.gain(),touchInput.reads(),touchInput.errors(),touchInput.idleNacks());
        perf=Perf{};lastLogUs=now;
    }
    delay(1);
}

#pragma once
#include <Adafruit_BMP280.h>

class Device {

    public:

        Device();

        void setup();
        void loop();
        static void message_handler(char *, char *);
        static bool payloadReady;
        static char globalBuf[256];

        bool IsDown();

        int GetTankLevel();
        int GetAverageTankLevel();
        float ReadTemp(void);
        void TestData();
    private:
        bool _Down;
        bool _BMP_Found;
        Adafruit_BMP280 bmp; // I2C
        float _OutsideTemp= 0.0;
        float _InsideTemp= 0.0;
        float _Pressure= 0.0;
        unsigned long _SampleTime=0;
        int _Depth = 0;
};


extern Device device;
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

    private:
        bool _Down;
        bool _BMP_Found;
        Adafruit_BMP280 bmp; // I2C
};


extern Device device;
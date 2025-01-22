#pragma once

#define expireTime 7 * 24 * 60 * 60 * 1000 // 7 days

class DeviceRegisterer {
public:
   static void tryRegister();
private:   
   static void setClaim();
};
#ifndef REGISTERING_DEVICE_MANAGEMENT_H
#define REGISTERING_DEVICE_MANAGEMENT_H

#define expireTime 7 * 24 * 60 * 60 * 1000 // 7 days

class RegisteringDeviceManagement {
public:
   static void setClaim();
   static void checkIfDeviceIsRegistered();

private:   
};

#endif // REGISTERING_DEVICE_MANAGEMENT_H
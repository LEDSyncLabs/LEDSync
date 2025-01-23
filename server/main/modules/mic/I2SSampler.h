#pragma once

#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>

/**
 * Base Class for both the ADC and I2S sampler
 **/
class I2SSampler {
protected:
  i2s_port_t m_i2sPort = I2S_NUM_0;
  virtual void configureI2S() = 0;
  virtual void unConfigureI2S() {};
  virtual void processI2SData(void *samples, size_t count) {
  };

  void start();
public:
  I2SSampler(i2s_port_t i2sPort);
  virtual int read(int16_t *samples, int count) = 0;
  void stop();
};
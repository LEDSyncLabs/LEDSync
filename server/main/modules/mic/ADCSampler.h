#pragma once

#include "I2SSampler.h"
#include <inttypes.h>
#include "esp_log.h"
#include <functional>

class ADCSampler : public I2SSampler {
public:
  using Callback = void (*)(int16_t *samples, int count);
  ADCSampler(adc1_channel_t adc_channel, Callback callback);
  virtual int read(int16_t *samples, int count);

private:
  adc_unit_t m_adcUnit = ADC_UNIT_1;
  adc1_channel_t m_adcChannel;
  Callback callback;
  static void adcWriterTaskWrapper(void* param);


protected:
  void adcWriterTask();
  void configureI2S();
  void unConfigureI2S();
};
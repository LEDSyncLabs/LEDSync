#include "ADCSampler.h"

#define MIC_SAMPLE_SIZE 4096

void ADCSampler::adcWriterTask() {
  int16_t *samples = (int16_t *)malloc(sizeof(uint16_t) * MIC_SAMPLE_SIZE);

  if (!samples) {
    ESP_LOGI("main", "Failed to allocate memory for samples");
    return;
  }

  while (true) {
    int samples_read = read(samples, MIC_SAMPLE_SIZE);

    callback(samples, samples_read);
  }
}

void ADCSampler::adcWriterTaskWrapper(void* param) {
    ADCSampler* instance = static_cast<ADCSampler*>(param);
    instance->adcWriterTask();
}

ADCSampler::ADCSampler(adc1_channel_t adcChannel, Callback callback)
    : I2SSampler(I2S_NUM_0), callback(callback) {

  m_adcChannel = adcChannel;

  start();

  xTaskCreatePinnedToCore(
    adcWriterTaskWrapper,
    "ADC Writer Task",                   // Task name
    4096,                                // Stack size
    nullptr,                             // Task parameters
    1,                                   // Task priority
    nullptr,                             // Task handle
    1                                    // Core ID
  );
}

void ADCSampler::configureI2S() {
  // init ADC pad
  i2s_set_adc_mode(m_adcUnit, m_adcChannel);
  // enable the adc
  i2s_adc_enable(m_i2sPort);
}

void ADCSampler::unConfigureI2S() {
  // make sure ot do this or the ADC is locked
  i2s_adc_disable(m_i2sPort);
}

int ADCSampler::read(int16_t *samples, int count) {
  // read from i2s
  size_t bytes_read = 0;
  i2s_read(m_i2sPort, samples, sizeof(int16_t) * count, &bytes_read,
           portMAX_DELAY);
  int samples_read = bytes_read / sizeof(int16_t);
  for (int i = 0; i < samples_read; i++) {
    samples[i] = (2048 - (uint16_t(samples[i]) & 0xfff)) * 15;
  }
  return samples_read;
}
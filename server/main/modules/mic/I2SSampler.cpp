
#include "I2SSampler.h"
#include "driver/i2s.h"

#define MIC_SAMPLE_RATE 16384

I2SSampler::I2SSampler(i2s_port_t i2sPort)
    : m_i2sPort(i2sPort) {}

void I2SSampler::start() {
  i2s_config_t i2s_config = {
      .mode =
          (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
      .sample_rate = MIC_SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_I2S_LSB,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 4,
      .dma_buf_len = 1024,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0};

  // install and start i2s driver
  i2s_driver_install(m_i2sPort, &i2s_config, 0, NULL);
  // set up the I2S configuration from the subclass
  configureI2S();
}

void I2SSampler::stop() {
  // clear any I2S configuration
  unConfigureI2S();
  // stop the i2S driver
  i2s_driver_uninstall(m_i2sPort);
}
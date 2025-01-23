#include "ir.hpp"

#define EXAMPLE_IR_RESOLUTION_HZ 1000000 // 1MHz resolution, 1 tick = 1us
#define EXAMPLE_IR_RX_GPIO_NUM GPIO_NUM_15
#define EXAMPLE_IR_NEC_DECODE_MARGIN                                           \
  300 // Tolerance for parsing RMT symbols into bit stream

/**
 * @brief NEC timing spec
 */
#define NEC_LEADING_CODE_DURATION_0 9000
#define NEC_LEADING_CODE_DURATION_1 4500
#define NEC_PAYLOAD_ZERO_DURATION_0 560
#define NEC_PAYLOAD_ZERO_DURATION_1 560
#define NEC_PAYLOAD_ONE_DURATION_0 560
#define NEC_PAYLOAD_ONE_DURATION_1 1690
#define NEC_REPEAT_CODE_DURATION_0 9000
#define NEC_REPEAT_CODE_DURATION_1 2250

QueueHandle_t IR::gpio_evt_queue = NULL;
std::vector<IR::Listener> IR::listeners;
IR::Callback IR::default_callback = NULL;
rmt_symbol_word_t IR::raw_symbols[64];
uint16_t IR::s_nec_code_address = 0;
uint16_t IR::s_nec_code_command = 0;
rmt_channel_handle_t IR::rx_channel = NULL;
const rmt_receive_config_t IR::receive_config = {
    .signal_range_min_ns = 1250,
    .signal_range_max_ns = 12000000,
};

IR::IR() {}

IR::~IR() { vQueueDelete(gpio_evt_queue); }

void IR::start() {
  if (gpio_evt_queue != NULL) {
    ESP_LOGW("IR", "IR handler already started");
    return;
  }

  gpio_evt_queue = xQueueCreate(10, sizeof(rmt_rx_done_event_data_t));
  xTaskCreate(ir_task, "ir_task", 2048, this, 10, NULL);

  rmt_rx_channel_config_t rx_channel_cfg = {
      .gpio_num = EXAMPLE_IR_RX_GPIO_NUM,
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = EXAMPLE_IR_RESOLUTION_HZ,
      .mem_block_symbols = 64,
  };
  ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_channel_cfg, &rx_channel));

  rmt_rx_event_callbacks_t cbs = {
      .on_recv_done = rmt_rx_done_callback,
  };
  ESP_ERROR_CHECK(
      rmt_rx_register_event_callbacks(rx_channel, &cbs, gpio_evt_queue));

  ESP_ERROR_CHECK(rmt_enable(rx_channel));
  ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols),
                              &receive_config));
}

void IR::addListener(uint16_t command, Callback callback) {
  listeners.push_back({command, callback});
}

void IR::addListener(Callback callback) { default_callback = callback; }

bool IR::rmt_rx_done_callback(rmt_channel_handle_t channel,
                              const rmt_rx_done_event_data_t *edata,
                              void *user_data) {
  BaseType_t high_task_wakeup = pdFALSE;
  QueueHandle_t receive_queue = (QueueHandle_t)user_data;
  xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
  return high_task_wakeup == pdTRUE;
}

void IR::ir_task(void *arg) {
  IR *ir_instance = static_cast<IR *>(arg);
  rmt_rx_done_event_data_t rx_data;
  while (1) {
    if (xQueueReceive(gpio_evt_queue, &rx_data, portMAX_DELAY)) {
      ir_instance->parse_nec_frame(rx_data.received_symbols,
                                   rx_data.num_symbols);
      if (default_callback) {
        default_callback(s_nec_code_command);
      }
      for (const auto &listener : listeners) {
        if (listener.command == s_nec_code_command) {
          listener.callback(s_nec_code_command);
        }
      }
      // Wait for next command
      ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols),
                                  &receive_config));
    }
  }
}

bool IR::nec_check_in_range(uint32_t signal_duration, uint32_t spec_duration) {
  return (signal_duration < (spec_duration + EXAMPLE_IR_NEC_DECODE_MARGIN)) &&
         (signal_duration > (spec_duration - EXAMPLE_IR_NEC_DECODE_MARGIN));
}

bool IR::nec_parse_logic0(rmt_symbol_word_t *rmt_nec_symbols) {
  return nec_check_in_range(rmt_nec_symbols->duration0,
                            NEC_PAYLOAD_ZERO_DURATION_0) &&
         nec_check_in_range(rmt_nec_symbols->duration1,
                            NEC_PAYLOAD_ZERO_DURATION_1);
}

bool IR::nec_parse_logic1(rmt_symbol_word_t *rmt_nec_symbols) {
  return nec_check_in_range(rmt_nec_symbols->duration0,
                            NEC_PAYLOAD_ONE_DURATION_0) &&
         nec_check_in_range(rmt_nec_symbols->duration1,
                            NEC_PAYLOAD_ONE_DURATION_1);
}

bool IR::nec_parse_frame(rmt_symbol_word_t *rmt_nec_symbols) {
  rmt_symbol_word_t *cur = rmt_nec_symbols;
  uint16_t address = 0;
  uint16_t command = 0;
  bool valid_leading_code =
      nec_check_in_range(cur->duration0, NEC_LEADING_CODE_DURATION_0) &&
      nec_check_in_range(cur->duration1, NEC_LEADING_CODE_DURATION_1);
  if (!valid_leading_code) {
    return false;
  }
  cur++;
  for (int i = 0; i < 16; i++) {
    if (nec_parse_logic1(cur)) {
      address |= 1 << i;
    } else if (nec_parse_logic0(cur)) {
      address &= ~(1 << i);
    } else {
      return false;
    }
    cur++;
  }
  for (int i = 0; i < 16; i++) {
    if (nec_parse_logic1(cur)) {
      command |= 1 << i;
    } else if (nec_parse_logic0(cur)) {
      command &= ~(1 << i);
    } else {
      return false;
    }
    cur++;
  }
  s_nec_code_address = address;
  s_nec_code_command = command;
  return true;
}

bool IR::nec_parse_frame_repeat(rmt_symbol_word_t *rmt_nec_symbols) {
  return nec_check_in_range(rmt_nec_symbols->duration0,
                            NEC_REPEAT_CODE_DURATION_0) &&
         nec_check_in_range(rmt_nec_symbols->duration1,
                            NEC_REPEAT_CODE_DURATION_1);
}

void IR::parse_nec_frame(rmt_symbol_word_t *rmt_nec_symbols,
                         size_t symbol_num) {
  bool frame_parsed = false;

  if (symbol_num == 34) {
    frame_parsed = nec_parse_frame(rmt_nec_symbols);
  } else if (symbol_num == 2) {
    frame_parsed = nec_parse_frame_repeat(rmt_nec_symbols);
  }

#ifdef DEBUG_FRAME
  printf("NEC frame start---\r\n");
  for (size_t i = 0; i < symbol_num; i++) {
    printf("{%d:%d},{%d:%d}\r\n", rmt_nec_symbols[i].level0,
           rmt_nec_symbols[i].duration0, rmt_nec_symbols[i].level1,
           rmt_nec_symbols[i].duration1);
  }
  printf("---NEC frame end: ");

  if (frame_parsed) {
    printf("Address=%04X, Command=%04X\r\n\r\n", s_nec_code_address,
           s_nec_code_command);
  } else {
    printf("Unknown NEC frame\r\n\r\n");
  }
#endif
}
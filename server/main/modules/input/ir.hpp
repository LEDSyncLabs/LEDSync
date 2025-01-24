#pragma once

#include "driver/rmt_rx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <functional>
#include <vector>


class IR {
public:
  using Callback = std::function<void(uint16_t command)>;

  IR();
  ~IR();

  /// @brief Add listener for specific command
  /// @param command The command to listen for
  /// @param callback The callback function to be called when the command is
  /// received
  void addListener(uint16_t command, Callback callback);

  /// @brief Add listener for all commands
  /// @param callback The callback function to be called for any command
  void addListener(Callback callback);

private:
  void start();

  struct Listener {
    uint16_t command;
    Callback callback;
  };

  static void ir_task(void *arg);
  static bool rmt_rx_done_callback(rmt_channel_handle_t channel,
                                   const rmt_rx_done_event_data_t *edata,
                                   void *user_data);
  void parse_nec_frame(rmt_symbol_word_t *rmt_nec_symbols, size_t symbol_num);
  bool nec_parse_frame(rmt_symbol_word_t *rmt_nec_symbols);
  bool nec_parse_frame_repeat(rmt_symbol_word_t *rmt_nec_symbols);

  static bool nec_check_in_range(uint32_t signal_duration,
                                 uint32_t spec_duration);
  static bool nec_parse_logic0(rmt_symbol_word_t *rmt_nec_symbols);
  static bool nec_parse_logic1(rmt_symbol_word_t *rmt_nec_symbols);

  static QueueHandle_t gpio_evt_queue;
  static std::vector<Listener> listeners;
  static Callback default_callback;
  static rmt_symbol_word_t raw_symbols[64];
  static uint16_t s_nec_code_address;
  static uint16_t s_nec_code_command;
  static rmt_channel_handle_t rx_channel;
  static const rmt_receive_config_t receive_config;

  int64_t last_command_time;
  static constexpr int64_t debounce_time_us = 300000; // 300ms debounce time

  friend class Input;
};
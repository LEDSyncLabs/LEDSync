#include "lcd.h"
#include "colors.h"
#include "commands.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_system.h"
#include "font.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 This code displays some fancy graphics on the 320x240 LCD on an ESP-WROVER_KIT
 board. This example demonstrates the use of both spi_device_transmit as well as
 spi_device_queue_trans/spi_device_get_trans_result and pre-transmit callbacks.

 Some info about the ILI9341/ST7789V: It has an C/D line, which is connected to
 a GPIO here. It expects this line to be low for a command and high for data. We
 use a pre-transmit callback here to control that line: every transaction has as
 the user-definable argument the needed state of the D/C line and just before
 the transaction is sent, the callback will set this line to the correct state.
*/



static spi_device_handle_t spi;
static uint16_t display_buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT] = {0};
static uint16_t write_buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT] = {0};
static uint16_t *buffer_lines[2];

/*
 The LCD needs a bunch of command/argument values to be initialized. They
 are stored in this struct.
*/
typedef struct {
  uint8_t cmd;
  uint8_t data[16];
  uint8_t databytes; // No of data in data; bit 7 = delay after set; 0xFF = end
                     // of cmds.
} lcd_init_cmd_t;

DRAM_ATTR static const lcd_init_cmd_t ili_init_cmds[] = {
    /* Power contorl B, power control = 0, DC_ENA = 1 */
    {0xCF, {0x00, 0x83, 0X30}, 3},
    /* Power on sequence control,
     * cp1 keeps 1 frame, 1st frame enable
     * vcl = 0, ddvdh=3, vgh=1, vgl=2
     * DDVDH_ENH=1
     */
    {0xED, {0x64, 0x03, 0X12, 0X81}, 4},
    /* Driver timing control A,
     * non-overlap=default +1
     * EQ=default - 1, CR=default
     * pre-charge=default - 1
     */
    {0xE8, {0x85, 0x01, 0x79}, 3},
    /* Power control A, Vcore=1.6V, DDVDH=5.6V */
    {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
    /* Pump ratio control, DDVDH=2xVCl */
    {0xF7, {0x20}, 1},
    /* Driver timing control, all=0 unit */
    {0xEA, {0x00, 0x00}, 2},
    /* Power control 1, GVDD=4.75V */
    {0xC0, {0x26}, 1},
    /* Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3 */
    {0xC1, {0x11}, 1},
    /* VCOM control 1, VCOMH=4.025V, VCOML=-0.950V */
    {0xC5, {0x35, 0x3E}, 2},
    /* VCOM control 2, VCOMH=VMH-2, VCOML=VML-2 */
    {0xC7, {0xBE}, 1},
    /* Memory access contorl, MX=MY=0, MV=1, ML=0, BGR=1, MH=0 */
    {0x36, {0x28}, 1},
    /* Pixel format, 16bits/pixel for RGB/MCU interface */
    {0x3A, {0x55}, 1},
    /* Frame rate control, f=fosc, 70Hz fps */
    {0xB1, {0x00, 0x1B}, 2},
    /* Enable 3G, disabled */
    {0xF2, {0x08}, 1},
    /* Gamma set, curve 1 */
    {0x26, {0x01}, 1},
    /* Positive gamma correction */
    {0xE0,
     {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02,
      0x07, 0x05, 0x00},
     15},
    /* Negative gamma correction */
    {0XE1,
     {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D,
      0x38, 0x3A, 0x1F},
     15},
    /* Column address set, SC=0, EC=0xEF */
    {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
    /* Page address set, SP=0, EP=0x013F */
    {0x2B, {0x00, 0x00, 0x01, 0x3f}, 4},
    /* Memory write */
    {0x2C, {0}, 0},
    /* Entry mode set, Low vol detect disabled, normal display */
    {0xB7, {0x07}, 1},
    /* Display function control */
    {0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
    /* Sleep out */
    {0x11, {0}, 0x80},
    /* Display on */
    {0x29, {0}, 0x80},
    {0, {0}, 0xff},
};

// void lcd_display_clear(uint16_t color);
// void lcd_display_update();

/* Send a command to the LCD. Uses spi_device_polling_transmit, which waits
 * until the transfer is complete.
 *
 * Since command transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more
 * than just waiting for the transaction to complete.
 */
void lcd_cmd(const uint8_t cmd, bool keep_cs_active) {
  esp_err_t ret;
  spi_transaction_t t;
  memset(&t, 0, sizeof(t)); // Zero out the transaction
  t.length = 8;             // Command is 8 bits
  t.tx_buffer = &cmd;       // The data is the cmd itself
  t.user = (void *)0;       // D/C needs to be set to 0
  if (keep_cs_active) {
    t.flags = SPI_TRANS_CS_KEEP_ACTIVE; // Keep CS active after data transfer
  }
  ret = spi_device_polling_transmit(spi, &t); // Transmit!
  assert(ret == ESP_OK);                      // Should have had no issues.
}

/* Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
 * transfer is complete.
 *
 * Since data transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
void lcd_data(const uint8_t *data, int len) {
  esp_err_t ret;
  spi_transaction_t t;
  if (len == 0)
    return;                 // no need to send anything
  memset(&t, 0, sizeof(t)); // Zero out the transaction
  t.length = len * 8;       // Len is in bytes, transaction length is in bits.
  t.tx_buffer = data;       // Data
  t.user = (void *)1;       // D/C needs to be set to 1
  ret = spi_device_polling_transmit(spi, &t); // Transmit!
  assert(ret == ESP_OK);                      // Should have had no issues.
}

// This function is called (in irq context!) just before a transmission starts.
// It will set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *t) {
  int dc = (int)t->user;
  gpio_set_level(PIN_NUM_DC, dc);
}

// Initialize the display
void lcd_init() {
  esp_err_t ret;
  spi_bus_config_t buscfg = {.mosi_io_num = PIN_NUM_MOSI,
                             .miso_io_num = PIN_NUM_MISO,
                             .sclk_io_num = PIN_NUM_CLK,
                             .quadwp_io_num = -1,
                             .quadhd_io_num = -1,
                             .max_transfer_sz =
                                 PARALLEL_LINES * DISPLAY_REAL_WIDTH * 2 + 8};
  spi_device_interface_config_t devcfg = {
      .mode = 0,                          // SPI mode 0
      .clock_speed_hz = 10 * 1000 * 1000, // Clock out at 10 MHz
      .spics_io_num = PIN_NUM_CS,         // CS pin
      .queue_size = 7, // We want to be able to queue 7 transactions at a time
      .pre_cb = lcd_spi_pre_transfer_callback, // Specify pre-transfer callback
                                               // to handle D/C line
  };
  // Initialize the SPI bus
  ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
  ESP_ERROR_CHECK(ret);
  // Attach the LCD to the SPI bus
  ret = spi_bus_add_device(LCD_HOST, &devcfg, &spi);
  ESP_ERROR_CHECK(ret);

  int cmd = 0;

  // Initialize non-SPI GPIOs
  gpio_config_t io_conf = {};
  io_conf.pin_bit_mask =
      ((1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST) | (1ULL << PIN_NUM_BCKL));
  // ((1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST));
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);

  // Reset the display
  gpio_set_level(PIN_NUM_RST, 0);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  gpio_set_level(PIN_NUM_RST, 1);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  // Send all the commands
  while (ili_init_cmds[cmd].databytes != 0xff) {
    lcd_cmd(ili_init_cmds[cmd].cmd, false);
    lcd_data(ili_init_cmds[cmd].data, ili_init_cmds[cmd].databytes & 0x1F);
    if (ili_init_cmds[cmd].databytes & 0x80) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    cmd++;
  }

  for (int i = 0; i < 2; i++) {
    // buffer_lines[i] = (uint16_t *)heap_caps_malloc(
    //     DISPLAY_REAL_WIDTH * PARALLEL_LINES * sizeof(uint16_t),
    //     MALLOC_CAP_DMA);
    buffer_lines[i] = (uint16_t *)malloc(DISPLAY_REAL_WIDTH * PARALLEL_LINES *
                                         sizeof(uint16_t));
    assert(buffer_lines[i] != NULL);
  }

  lcd_display_clear(LCD_COLOR_BLACK);
  lcd_display_update();

  // Enable backlight
  gpio_set_level(PIN_NUM_BCKL, 1);
}

/* To send a set of lines we have to send a command, 2 data bytes, another
 * command, 2 more data bytes and another command before sending the line data
 * itself; a total of 6 transactions. (We can't put all of this in just one
 * transaction because the D/C line needs to be toggled in the middle.) This
 * routine queues these commands up as interrupt transactions so they get sent
 * faster (compared to calling spi_device_transmit several times), and at the
 * mean while the lines for next transactions can get calculated.
 */
void send_lines(int ypos, uint16_t *linedata) {
  esp_err_t ret;
  int x;
  // Transaction descriptors. Declared static so they're not allocated on the
  // stack; we need this memory even when this function is finished because the
  // SPI driver needs access to it even while we're already calculating the next
  // line.
  static spi_transaction_t trans[6];

  // In theory, it's better to initialize trans and data only once and hang on
  // to the initialized variables. We allocate them on the stack, so we need to
  // re-init them each call.
  for (x = 0; x < 6; x++) {
    memset(&trans[x], 0, sizeof(spi_transaction_t));
    if ((x & 1) == 0) {
      // Even transfers are commands
      trans[x].length = 8;
      trans[x].user = (void *)0;
    } else {
      // Odd transfers are data
      trans[x].length = 8 * 4;
      trans[x].user = (void *)1;
    }
    trans[x].flags = SPI_TRANS_USE_TXDATA;
  }
  trans[0].tx_data[0] = 0x2A;                             // Column Address Set
  trans[1].tx_data[0] = 0;                                // Start Col High
  trans[1].tx_data[1] = 0;                                // Start Col Low
  trans[1].tx_data[2] = (DISPLAY_REAL_WIDTH - 1) >> 8;    // End Col High
  trans[1].tx_data[3] = (DISPLAY_REAL_WIDTH - 1) & 0xff;  // End Col Low
  trans[2].tx_data[0] = 0x2B;                             // Page address set
  trans[3].tx_data[0] = ypos >> 8;                        // Start page high
  trans[3].tx_data[1] = ypos & 0xff;                      // start page low
  trans[3].tx_data[2] = (ypos + PARALLEL_LINES - 1) >> 8; // end page high
  trans[3].tx_data[3] = (ypos + PARALLEL_LINES - 1) & 0xff; // end page low
  trans[4].tx_data[0] = 0x2C;                               // memory write
  trans[5].tx_buffer = linedata; // finally send the line data
  trans[5].length =
      DISPLAY_REAL_WIDTH * 2 * 8 * PARALLEL_LINES; // Data length, in bits
  trans[5].flags = 0; // undo SPI_TRANS_USE_TXDATA flag

  // Queue all transactions.
  for (x = 0; x < 6; x++) {
    ret = spi_device_queue_trans(spi, &trans[x], portMAX_DELAY);
    assert(ret == ESP_OK);
  }

  // When we are here, the SPI driver is busy (in the background) getting the
  // transactions sent. That happens mostly using DMA, so the CPU doesn't have
  // much to do here. We're not going to wait for the transaction to finish
  // because we may as well spend the time calculating the next line. When that
  // is done, we can call send_line_finish, which will wait for the transfers to
  // be done and check their status.
}

void send_line_finish() {
  spi_transaction_t *rtrans;
  esp_err_t ret;

  // Wait for all 6 transactions to be done and get back the results.
  for (int x = 0; x < 6; x++) {
    ret = spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
    assert(ret == ESP_OK);
  }
}

int display_to_buffer(int x, int y) { return y * DISPLAY_WIDTH + x; }

int sending_line = -1;
int calc_line = 0;

// Simple routine to generate some patterns and send them to the LCD. Don't
// expect anything too impressive. Because the SPI driver handles transactions
// in the background, we can calculate the next line while the previous one is
// being sent.
void lcd_display_update() {
  memcpy(display_buffer, write_buffer, sizeof(write_buffer));

  if (sending_line != -1) {
    send_line_finish();
    sending_line = -1;
    calc_line = 0;
  }

  for (int lineset = 0; lineset < DISPLAY_HEIGHT;
       lineset += PARALLEL_LINES / DISPLAY_SCALE) {
    uint16_t *ptr = buffer_lines[calc_line];

    for (int y = lineset; y < lineset + PARALLEL_LINES / DISPLAY_SCALE; y++) {
      for (int x = 0; x < DISPLAY_WIDTH; x++) {

        for (int dy = 0; dy < DISPLAY_SCALE; dy++) {
          for (int dx = 0; dx < DISPLAY_SCALE; dx++) {
            int px = x * DISPLAY_SCALE + dx;
            int py = (y - lineset) * DISPLAY_SCALE + dy;
            ptr[py * DISPLAY_REAL_WIDTH + px] =
                display_buffer[display_to_buffer(x, y)];
          }
        }
      }
    }

    if (sending_line != -1) {
      send_line_finish();
    }

    sending_line = calc_line;
    calc_line = (calc_line == 1) ? 0 : 1;

    send_lines(DISPLAY_SCALE * lineset, buffer_lines[sending_line]);

    // The line set is queued up for sending now; the actual sending happens
    // in the background. We can go on to calculate the next line set as long
    // as we do not touch line[sending_line]; the SPI sending process is still
    // reading from that.
  }
}

void memset16(void *m, uint16_t val, size_t count) {
  uint16_t *buf = (uint16_t *)m;

  while (count--) {
    *buf++ = val;
  }
}

void lcd_display_clear(uint16_t color) {
  size_t buffer_size = DISPLAY_WIDTH * DISPLAY_HEIGHT;

  memset16(write_buffer, color, buffer_size);
}

void lcd_pixel_put(int x, int y, uint16_t color) {
  if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) {
    return;
  }

  write_buffer[display_to_buffer(x, y)] = color;
}

void lcd_draw_char(int x, int y, char c, uint16_t color) {
  if (c < FONT_BOUND_LOWER || c > FONT_BOUND_UPPER) {
    return;
  }

  for (int col = 0; col < 5; col++) {
    uint8_t fontcol = font_default[c - 32][col];

    for (int row = 0; row < 8; row++) {
      if (fontcol & (1 << row)) {
        lcd_pixel_put(x + col, y + row, color);
      }
    }
  }
}

void lcd_draw_text(int x, int y, char *text, uint16_t color) {
  int char_x = x;
  int char_y = y;
  char *char_ptr = text;

  while (*char_ptr) {
    lcd_draw_char(char_x, char_y, *char_ptr, color);

    if (char_x + FONT_CHAR_WIDTH + 1 >= DISPLAY_WIDTH) {
      if (char_y + FONT_CHAR_HEIGHT + 1 >= DISPLAY_HEIGHT) {
        // Out of space
        return;
      }

      char_y += FONT_CHAR_HEIGHT + 1;
      char_x = x;
    } else {
      char_x += FONT_CHAR_WIDTH + 1;
    }

    char_ptr++;
  }
}

void lcd_invert_set(bool invert) {
  lcd_cmd(invert ? LCD_DINVON : LCD_DINVOFF, false);
}

void lcd_draw_rect_outline(int x, int y, int w, int h, uint16_t color) {
  for (int col = x; col < x + w; col++) {
    lcd_pixel_put(col, y, color);
    lcd_pixel_put(col, y + h, color);
  }

  for (int row = y; row < y + h; row++) {
    lcd_pixel_put(x, row, color);
    lcd_pixel_put(x + w, row, color);
  }
}

// Bresenham's line algorithm
void lcd_draw_line(int x0, int y0, int x1, int y1, uint16_t color) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2;

  while (true) {
    lcd_pixel_put(x0, y0, color);
    if (x0 == x1 && y0 == y1)
      break;
    e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void lcd_draw_rect_filled(int x, int y, int w, int h, uint16_t color) {
  for (int col = x; col < x + w; col++) {
    for (int row = y; row < y + h; row++) {
      lcd_pixel_put(col, row, color);
    }
  }
}

// Bresenham's circle algorithm
void lcd_draw_circle(int cx, int cy, int r, u_int16_t color) {
  int x = 0;
  int y = -r;
  int p = -r;

  while (x < -y) {
    if (p > 0) {
      ++y;
      p += 2 * (x + y) + 1;
    } else {
      p += 2 * x + 1;
    }

    lcd_pixel_put(cx + x, cy + y, color);
    lcd_pixel_put(cx - x, cy + y, color);
    lcd_pixel_put(cx + x, cy - y, color);
    lcd_pixel_put(cx - x, cy - y, color);
    lcd_pixel_put(cx + y, cy + x, color);
    lcd_pixel_put(cx - y, cy + x, color);
    lcd_pixel_put(cx + y, cy - x, color);
    lcd_pixel_put(cx - y, cy - x, color);

    ++x;
  }
}

void lcd_draw_disk(int cx, int cy, int r, u_int16_t color) {
  int x = 0;
  int y = -r;
  int p = -r;

  while (x < -y) {
    if (p > 0) {
      ++y;
      p += 2 * (x + y) + 1;
    } else {
      p += 2 * x + 1;
    }

    lcd_draw_line(cx - x, cy + y, cx + x, cy + y, color);
    lcd_draw_line(cx - x, cy - y, cx + x, cy - y, color);
    lcd_draw_line(cx + y, cy + x, cx - y, cy + x, color);
    lcd_draw_line(cx + y, cy - x, cx - y, cy - x, color);

    ++x;
  }
}

void lcd_demo() {
  lcd_display_clear(LCD_COLOR_BLACK);

  lcd_draw_text(6, 6, "Demo: LCD", LCD_COLOR_WHITE);
  lcd_display_update();

  vTaskDelay(5000 / portTICK_PERIOD_MS);

  lcd_display_clear(LCD_COLOR_BLACK);
  lcd_draw_text(6, 6, "Demo: LCD - czyszczenie", LCD_COLOR_WHITE);
  lcd_draw_text(6, 26, "Demo: LCD - czyszczenie", LCD_COLOR_WHITE);
  lcd_draw_text(6, 46, "Demo: LCD - czyszczenie", LCD_COLOR_WHITE);
  lcd_draw_text(6, 66, "Demo: LCD - czyszczenie", LCD_COLOR_WHITE);
  lcd_draw_text(6, 86, "Demo: LCD - czyszczenie", LCD_COLOR_WHITE);
  lcd_draw_text(6, 106, "Demo: LCD - czyszczenie", LCD_COLOR_WHITE);
  lcd_display_update();

  vTaskDelay(5000 / portTICK_PERIOD_MS);

  lcd_display_clear(LCD_COLOR_BLACK);
  lcd_draw_text(6, 6, "Demo: LCD - linie", LCD_COLOR_WHITE);
  lcd_draw_line(20, 40, 80, 40, LCD_COLOR_WHITE);
  lcd_draw_line(40, 50, 40, 100, LCD_COLOR_WHITE);
  lcd_draw_line(60, 60, 120, 110, LCD_COLOR_WHITE);
  lcd_display_update();

  vTaskDelay(5000 / portTICK_PERIOD_MS);

  lcd_display_clear(LCD_COLOR_BLACK);
  lcd_draw_text(6, 6, "Demo: LCD - ksztalty", LCD_COLOR_WHITE);
  lcd_draw_rect_outline(20, 40, 40, 20, LCD_COLOR_WHITE);
  lcd_draw_rect_filled(40, 80, 40, 20, LCD_COLOR_WHITE);
  lcd_draw_circle(100, 70, 20, LCD_COLOR_WHITE);
  lcd_draw_disk(100, 110, 20, LCD_COLOR_WHITE);
  lcd_display_update();

  vTaskDelay(5000 / portTICK_PERIOD_MS);

  lcd_display_clear(LCD_COLOR_BLACK);
  lcd_draw_text(6, 6, "Demo: LCD - kolory", LCD_COLOR_RED);
  lcd_draw_rect_outline(20, 40, 40, 20, LCD_COLOR_YELLOW);
  lcd_draw_rect_filled(40, 80, 40, 20, LCD_COLOR_BLUE);
  lcd_draw_circle(100, 70, 20, LCD_COLOR_GREEN);
  lcd_draw_disk(100, 110, 20, LCD_COLOR_BROWN);
  lcd_draw_line(20, 40, 80, 40, LCD_COLOR_AQUA);
  lcd_draw_line(40, 50, 40, 100, LCD_COLOR_ORANGE);
  lcd_draw_line(60, 60, 120, 110, LCD_COLOR_PURPLE);
  lcd_display_update();

  vTaskDelay(5000 / portTICK_PERIOD_MS);

  lcd_display_clear(LCD_COLOR_BLACK);
  lcd_draw_text(6, 6, "Demo: LCD - inwersja", LCD_COLOR_RED);
  lcd_draw_rect_outline(20, 40, 40, 20, LCD_COLOR_YELLOW);
  lcd_draw_rect_filled(40, 80, 40, 20, LCD_COLOR_BLUE);
  lcd_draw_circle(100, 70, 20, LCD_COLOR_GREEN);
  lcd_draw_disk(100, 110, 20, LCD_COLOR_BROWN);
  lcd_draw_line(20, 40, 80, 40, LCD_COLOR_AQUA);
  lcd_draw_line(40, 50, 40, 100, LCD_COLOR_ORANGE);
  lcd_draw_line(60, 60, 120, 110, LCD_COLOR_PURPLE);
  lcd_invert_set(true);
  lcd_display_update();

  vTaskDelay(5000 / portTICK_PERIOD_MS);

  lcd_display_clear(LCD_COLOR_BLACK);
  lcd_draw_text(6, 6, "To wszystko :)", LCD_COLOR_GREEN);
  lcd_invert_set(false);
  lcd_display_update();
}

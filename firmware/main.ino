#include "tusb_config.h"
#include <Adafruit_TinyUSB.h>

Adafruit_USBH_Host USBHost;

// GPIO pin definitions matching your wiring
#define UP_PIN    2
#define DOWN_PIN  4
#define LEFT_PIN  6
#define RIGHT_PIN 8
#define FIRE1_PIN 10
#define FIRE2_PIN 12

bool device_connected = false;
bool hid_connected = false;
tusb_desc_device_t dev_desc;

void setup() {
  pinMode(UP_PIN,      OUTPUT);
  pinMode(DOWN_PIN,    OUTPUT);
  pinMode(LEFT_PIN,    OUTPUT);
  pinMode(RIGHT_PIN,   OUTPUT);
  pinMode(FIRE1_PIN,   OUTPUT);
  pinMode(FIRE2_PIN,   OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // All pins released at startup
  digitalWrite(UP_PIN,    HIGH);
  digitalWrite(DOWN_PIN,  HIGH);
  digitalWrite(LEFT_PIN,  HIGH);
  digitalWrite(RIGHT_PIN, HIGH);
  digitalWrite(FIRE1_PIN, HIGH);
  digitalWrite(FIRE2_PIN, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);

  delay(1000); // Wait 1 second before starting USB Host

  USBHost.begin(0);
}

void device_descriptor_cb(tuh_xfer_t *xfer) {
  if (xfer->result == XFER_RESULT_SUCCESS) {
    tusb_desc_device_t *desc = (tusb_desc_device_t *)xfer->buffer;
    if (desc->bDeviceClass == 0) {
      // Class 0 confirmed
    }
  }
}

void tuh_mount_cb(uint8_t daddr) {
  device_connected = true;
  hid_connected = false;
  uint16_t vid, pid;
  tuh_vid_pid_get(daddr, &vid, &pid);
  tuh_descriptor_get_device(daddr, &dev_desc, 18, device_descriptor_cb, 0);
  tuh_hid_receive_report(daddr, 0);
}

void tuh_umount_cb(uint8_t daddr) {
  device_connected = false;
  hid_connected = false;
  // Release all pins on disconnect
  digitalWrite(UP_PIN,    HIGH);
  digitalWrite(DOWN_PIN,  HIGH);
  digitalWrite(LEFT_PIN,  HIGH);
  digitalWrite(RIGHT_PIN, HIGH);
  digitalWrite(FIRE1_PIN, HIGH);
  digitalWrite(FIRE2_PIN, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                       uint8_t const* desc_report, uint16_t desc_len) {
  hid_connected = true;
  if (desc_report == NULL || desc_len == 0) {
    tuh_hid_set_protocol(dev_addr, instance, HID_PROTOCOL_BOOT);
  } else {
    tuh_hid_set_protocol(dev_addr, instance, HID_PROTOCOL_REPORT);
  }
  tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
  hid_connected = false;
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                  uint8_t const* report, uint16_t len) {
  // Read axes and buttons
  uint8_t x = report[0];
  uint8_t y = report[1];
  uint8_t buttons = report[5];

  // Translate to GPIO pins — LOW = pressed, HIGH = released
  digitalWrite(LEFT_PIN,  (x < 0x40) ? LOW : HIGH);
  digitalWrite(RIGHT_PIN, (x > 0xC0) ? LOW : HIGH);
  digitalWrite(UP_PIN,    (y < 0x40) ? LOW : HIGH);
  digitalWrite(DOWN_PIN,  (y > 0xC0) ? LOW : HIGH);
  digitalWrite(FIRE2_PIN, (buttons & 0x20) ? LOW : HIGH);
  digitalWrite(FIRE1_PIN, (buttons & 0x10) ? LOW : HIGH);

  // LED solid when receiving data
  digitalWrite(LED_BUILTIN, HIGH);

  tuh_hid_receive_report(dev_addr, instance);
}

void loop() {
  USBHost.task();
}

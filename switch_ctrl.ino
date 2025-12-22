#include <Adafruit_TinyUSB.h>

#define LED_PIN 25

// ----------------------------------------------------------------
// レポートディスクリプタ（スティック・十字キー対応）
// ----------------------------------------------------------------
uint8_t const desc_hid_report[] = {
  0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x15, 0x00, 
  0x25, 0x01, 0x35, 0x00, 0x45, 0x01, 0x75, 0x01, 
  0x95, 0x10, 0x05, 0x09, 0x19, 0x01, 0x29, 0x10, 
  0x81, 0x02, 0x05, 0x01, 0x25, 0x07, 0x46, 0x3B, 
  0x01, 0x75, 0x04, 0x95, 0x01, 0x65, 0x14, 0x09, 
  0x39, 0x81, 0x42, 0x65, 0x00, 0x95, 0x01, 0x81, 
  0x01, 0x26, 0xFF, 0x00, 0x46, 0xFF, 0x00, 0x09, 
  0x30, 0x09, 0x31, 0x09, 0x32, 0x09, 0x35, 0x75, 
  0x08, 0x95, 0x04, 0x81, 0x02, 0x06, 0x00, 0xFF, 
  0x09, 0x20, 0x95, 0x01, 0x81, 0x02, 0xC0
};

// ボタン定義
#define BTN_Y      0x0001
#define BTN_B      0x0002
#define BTN_A      0x0004
#define BTN_X      0x0008
#define BTN_L      0x0010
#define BTN_R      0x0020
#define BTN_ZL     0x0040
#define BTN_ZR     0x0080
#define BTN_MINUS  0x0100
#define BTN_PLUS   0x0200
#define BTN_LCLICK 0x0400 // L3
#define BTN_RCLICK 0x0800 // R3
#define BTN_HOME   0x1000
#define BTN_CAP    0x2000

// 十字キー定義
#define HAT_TOP          0x00
#define HAT_TOP_RIGHT    0x01
#define HAT_RIGHT        0x02
#define HAT_BOTTOM_RIGHT 0x03
#define HAT_BOTTOM       0x04
#define HAT_BOTTOM_LEFT  0x05
#define HAT_LEFT         0x06
#define HAT_TOP_LEFT     0x07
#define HAT_CENTER       0x08

struct SwitchReport {
  uint16_t buttons;
  uint8_t  hat;
  uint8_t  lx; uint8_t  ly;
  uint8_t  rx; uint8_t  ry;
  uint8_t  vendor;
};

Adafruit_USBD_HID usb_hid;
SwitchReport report;

void setup() {
  // ★重要: ここで身分証明書を設定します
  // 0x0092 (POKKEN) は実績のある安定したIDです
  USBDevice.setID(0x0F0D, 0x0092);
  USBDevice.setProductDescriptor("POKKEN CONTROLLER");
  USBDevice.setManufacturerDescriptor("HORI CO.,LTD.");
  
  pinMode(LED_PIN, OUTPUT);
  for(int i=0; i<3; i++) {
    digitalWrite(LED_PIN, HIGH); delay(100);
    digitalWrite(LED_PIN, LOW);  delay(100);
  }

  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.setStringDescriptor("POKKEN CONTROLLER");
  usb_hid.begin();

  while( !USBDevice.mounted() ) delay(1);

  Serial1.setRX(1);
  Serial1.setTX(0);
  Serial1.begin(9600);
  resetReport();
}

void loop() {
  if (Serial1.available() > 0) {
    char cmd = Serial1.read();
    digitalWrite(LED_PIN, HIGH);

    switch (cmd) {
      // ボタン
      case 'z': press(BTN_A); break;
      case 'x': press(BTN_B); break;
      case 's': press(BTN_X); break;
      case 'a': press(BTN_Y); break;
      case 'q': press(BTN_L); break;
      case 'w': press(BTN_R); break;
      case 'e': press(BTN_ZL); break;
      case 'r': press(BTN_ZR); break;
      case 'n': press(BTN_PLUS); break;
      case 'm': press(BTN_MINUS); break;
      case 'h': press(BTN_HOME); break;
      case 'c': press(BTN_CAP); break;
      
      // スティック押し込み
      case '3': press(BTN_LCLICK); break; // L3
      case '4': press(BTN_RCLICK); break; // R3

      // 移動 (左スティック)
      case 'I': moveStick(128, 0); break;   // 上
      case 'K': moveStick(128, 255); break; // 下
      case 'J': moveStick(0, 128); break;   // 左
      case 'L': moveStick(255, 128); break; // 右

      // 十字キー
      case 'U': pressHat(HAT_TOP); break;
      case 'D': pressHat(HAT_BOTTOM); break;
      case 'F': pressHat(HAT_LEFT); break;
      case 'G': pressHat(HAT_RIGHT); break;
    }
    digitalWrite(LED_PIN, LOW);
  }
}

void press(uint16_t btn) {
  resetReport();
  report.buttons = btn;
  usb_hid.sendReport(0, &report, sizeof(report));
  delay(100);
  report.buttons = 0;
  usb_hid.sendReport(0, &report, sizeof(report));
  delay(50);
}

void pressHat(uint8_t hat_dir) {
  resetReport();
  report.hat = hat_dir;
  usb_hid.sendReport(0, &report, sizeof(report));
  delay(100); 
  report.hat = HAT_CENTER;
  usb_hid.sendReport(0, &report, sizeof(report));
  delay(50);
}

void moveStick(uint8_t lx, uint8_t ly) {
  resetReport();
  report.lx = lx; report.ly = ly;
  usb_hid.sendReport(0, &report, sizeof(report));
  delay(300); // 0.3秒動く
  report.lx = 128; report.ly = 128;
  usb_hid.sendReport(0, &report, sizeof(report));
  delay(50);
}

void resetReport() {
  report.buttons = 0;
  report.hat = HAT_CENTER;
  report.lx = 128; report.ly = 128;
  report.rx = 128; report.ry = 128;
  report.vendor = 0;
}
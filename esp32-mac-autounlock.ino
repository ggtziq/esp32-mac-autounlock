/*
 * ESP32 Mac 開機自動解鎖
 * 以藍牙 HID 鍵盤身分，在 Mac 冷啟動後於 FileVault 解鎖畫面輸入密碼。
 * 依賴：ESP32-BLE-Keyboard (T-vK) 0.3.2，esp32 核心 2.0.17
 *
 * 燒錄前把 PASSWORD 改成真密碼；commit 前務必改回 CHANGE_ME。
 */
#include <BleKeyboard.h>
#include <BLEDevice.h>
#include <Preferences.h>
#include <esp_gap_ble_api.h>

// ===== 設定區 =====
#define TEST_MODE        1       // 1=只送假字元；確認時機沒問題後改 0
#define SEND_ENTER       0       // 先確認密碼正確，再改 1
#define WAIT_AFTER_AUTH  5000    // 認證後等待毫秒數
#define MIN_DOWN_MS      30000   // 斷線超過這個時間才視為冷啟動
const char* PASSWORD  = "CHANGE_ME";   // 燒錄前改真密碼，commit 前改回
const char* TEST_TEXT = "abc";

const int LED_PIN  = 2;
const int PAIR_PIN = 0;          // BOOT 鍵

BleKeyboard bleKeyboard("ESP32 Unlock KB", "DIY", 100);
Preferences prefs;

uint8_t allowedAddr[6];
bool hasAllowed = false;
bool pairingMode = false;

volatile bool authOk = false;
volatile uint32_t authTime = 0;
volatile bool pendingSave = false;
uint8_t newAddr[6];

bool typedThisConn = false;
bool wasConnected = false;
bool firstConnSinceBoot = true;
uint32_t lastDisconnect = 0;

void gapHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
  if (event != ESP_GAP_BLE_AUTH_CMPL_EVT) return;
  if (!param->ble_security.auth_cmpl.success) return;

  uint8_t *a = param->ble_security.auth_cmpl.bd_addr;
  Serial.printf("認證完成，位址 %02X:%02X:%02X:%02X:%02X:%02X\n",
                a[0], a[1], a[2], a[3], a[4], a[5]);

  if (pairingMode) {                       // 配對模式：只記錄，不打字
    memcpy(newAddr, a, 6);
    pendingSave = true;
    return;
  }
  if (hasAllowed && memcmp(a, allowedAddr, 6) == 0) {
    authTime = millis();
    authOk = true;
  } else {
    Serial.println("位址不符，移除配對並斷線");
    esp_ble_remove_bond_device(a);
    esp_ble_gap_disconnect(a);
  }
}

void typePassword() {
  for (int i = 0; i < 30; i++) {           // 先清空欄位，避免殘留字元
    bleKeyboard.write(KEY_BACKSPACE);
    delay(20);
  }
#if TEST_MODE
  bleKeyboard.print(TEST_TEXT);
  Serial.println("[測試] 已送出假字元");
#else
  bleKeyboard.print(PASSWORD);
  if (SEND_ENTER) {
    delay(200);
    bleKeyboard.write(KEY_RETURN);
  }
  Serial.println("已輸入密碼");
#endif
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(PAIR_PIN, INPUT_PULLUP);

  prefs.begin("unlock", false);
  hasAllowed = (prefs.getBytes("mac", allowedAddr, 6) == 6);

  // 尚未記錄任何 Mac 時，自動進入配對模式
  pairingMode = !hasAllowed;

  // 開機後 5 秒內按一下 BOOT，也可以進入配對模式（LED 慢閃）
  Serial.println("開機 5 秒內按下 BOOT 可進入配對模式");
  uint32_t t = millis();
  while (millis() - t < 5000) {
    digitalWrite(LED_PIN, (millis() / 500) % 2);
    if (digitalRead(PAIR_PIN) == LOW) {
      pairingMode = true;
      break;
    }
    delay(20);
  }
  digitalWrite(LED_PIN, LOW);

  Serial.println(pairingMode ? "配對模式：請用 Mac 連線" : "一般模式");
  Serial.println(hasAllowed ? "已有記錄的 Mac 位址" : "尚未記錄任何 Mac");

  bleKeyboard.begin();
  BLEDevice::setCustomGapHandler(gapHandler);
}

void loop() {
  bool conn = bleKeyboard.isConnected();

  if (conn && !wasConnected) {
    typedThisConn = false;
    Serial.println("已連線");
  }
  if (!conn && wasConnected) {
    lastDisconnect = millis();
    authOk = false;
    Serial.println("已斷線");
  }
  wasConnected = conn;

  if (pendingSave) {                       // 在主迴圈寫入 flash
    prefs.putBytes("mac", newAddr, 6);
    memcpy(allowedAddr, newAddr, 6);
    hasAllowed = true;
    pairingMode = false;
    pendingSave = false;
    Serial.println("已記錄此 Mac，請按 EN 重開機");
  }

  if (conn && authOk && !typedThisConn && millis() - authTime >= WAIT_AFTER_AUTH) {
    typedThisConn = true;
    bool coldStart = firstConnSinceBoot || (millis() - lastDisconnect >= MIN_DOWN_MS);
    firstConnSinceBoot = false;
    if (coldStart) typePassword();
    else Serial.println("斷線時間太短，本次不送");
  }

  // LED：配對模式快閃、已連線恆亮、其餘熄滅
  if (pairingMode) digitalWrite(LED_PIN, (millis() / 150) % 2);
  else digitalWrite(LED_PIN, conn ? HIGH : LOW);

  delay(50);
}

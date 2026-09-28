# ESP32 Mac 開機自動解鎖

用 ESP32 模擬藍牙鍵盤，在 Mac 冷啟動後，於 FileVault 解鎖畫面自動輸入密碼並按下 Enter。

## 目的

Mac 開啟 FileVault 後，每次冷啟動都要在解鎖畫面手動輸入密碼。
本專案讓 ESP32 扮演一支已配對的藍牙鍵盤，開機後自動代為輸入，
適合不方便在場輸入的情境。

## 使用場景

- Mac mini / iMac 等桌機，停電後或遠端重開機後，需要自動進入系統
- 搭配「停電後自動啟動」或智慧插座，讓 Mac 可以無人值守重開機
- ESP32 需要由獨立電源供電（不能靠 Mac 的 USB，Mac 關機後會斷電）

## 運作方式

1. ESP32 以藍牙 HID 鍵盤身分與 Mac 配對，並記錄該 Mac 的藍牙位址
2. Mac 開機後連上 ESP32，通過認證的位址才會被接受，其他裝置一律拒絕
3. 連線後等待 5 秒，先清空欄位，再輸入密碼與 Enter
4. 同一次連線只輸入一次；斷線不到 30 秒又重連（如睡眠喚醒）不會輸入

## 硬體與環境

- ESP32-WROOM-32（一般 DevKit）
- Arduino IDE
- esp32 by Espressif **2.0.17**
- 函式庫：[ESP32-BLE-Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard)（T-vK 版，0.3.2）
- 開發板選 **ESP32 Dev Module**，Partition Scheme 選 **Huge APP (3MB No OTA/1MB SPIFFS)**

## 設定

1. 複製 `secrets.example.h` 為 `secrets.h`，填入 Mac 登入密碼
```cpp
   const char* PASSWORD = "你的密碼";
```
2. 先在 `unlock_kb.ino` 設定測試模式，只送假字元、不按 Enter
```cpp
   #define TEST_MODE 1
```
3. 上傳程式（建議開啟 Erase All Flash Before Sketch Upload）
4. Mac「系統設定 → 藍牙」移除舊的 `ESP32 Unlock KB`（如果有）
5. ESP32 重開機。尚未記錄任何 Mac 時會自動進入配對模式，
   用 Mac 連線 `ESP32 Unlock KB`，序列埠出現「已記錄此 Mac」即完成
6. 按 EN 重開機，序列埠顯示「已有記錄的 Mac 位址」表示進入一般模式
7. Mac 關機再冷啟動，確認解鎖畫面有出現圓點
8. 確認無誤後改用正式設定，並重新上傳
```cpp
   #define TEST_MODE   0
   #define SEND_ENTER  0   // 先不送 Enter，確認密碼正確後再改為 1
```

### 重新配對

ESP32 開機後 5 秒內按一下 BOOT 鍵，會進入配對模式，可綁定另一台 Mac。

### 可調整的參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `WAIT_AFTER_AUTH` | 5000 | 認證後等待多久才輸入（毫秒） |
| `MIN_DOWN_MS` | 30000 | 斷線超過多久才視為冷啟動（毫秒） |

## 安全注意事項

- 密碼以明文存在 ESP32 韌體中，取得板子的人可以讀出密碼。
  這個做法只能防「單獨被偷走的 Mac」，防不了「連 ESP32 一起被拿走」。
- 第一次配對請在自己家中、身邊沒有陌生人時進行，配對完成後才會鎖定。
- Mac 已登入時，若 ESP32 斷線超過 `MIN_DOWN_MS` 後重連，密碼與 Enter
  會被輸入到目前聚焦的視窗，請視使用習慣調整此參數。
- `secrets.h` 已列入 `.gitignore`，請勿提交。若密碼曾被 commit，請直接更換 Mac 登入密碼。

## 授權

MIT License

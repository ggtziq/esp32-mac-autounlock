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
2. Mac 開機後連上 ESP32，位址相符才會被接受，其他裝置一律拒絕
3. 連線並通過認證後等待 5 秒，先清空欄位，再輸入密碼與 Enter
4. ESP32 開機後的第一次連線一律輸入；之後同一次開機期間，斷線不到
   30 秒又重連（如睡眠喚醒）不會輸入，超過 30 秒視為冷啟動再次輸入
5. 同一次連線只輸入一次

## 硬體與環境

- ESP32-WROOM-32（一般 DevKit）
- Arduino IDE
- esp32 by Espressif **2.0.17**
- 函式庫：[ESP32-BLE-Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard)（T-vK 版，0.3.2）
- 開發板選 **ESP32 Dev Module**，Partition Scheme 選 **Huge APP (3MB No OTA/1MB SPIFFS)**

## 設定

1. 用 Arduino IDE 開啟 `unlock_kb.ino`，確認設定區為測試模式
   （只送假字元 `abc`，不送 Enter）：
```cpp
   #define TEST_MODE   1
   #define SEND_ENTER  0
```
2. 上傳程式（建議開啟 Erase All Flash Before Sketch Upload）
3. Mac「系統設定 → 藍牙」移除舊的 `ESP32 Unlock KB`（如果有）
4. ESP32 重開機。尚未記錄任何 Mac 時會自動進入配對模式，
   用 Mac 連線 `ESP32 Unlock KB`，序列埠出現「已記錄此 Mac」即完成
5. 按 EN 重開機，序列埠顯示「已有記錄的 Mac 位址」表示進入一般模式
6. Mac 關機再冷啟動，確認解鎖畫面有出現圓點（`abc`）
7. 將 `PASSWORD` 改成 Mac 登入密碼，並改為正式設定後重新上傳：
```cpp
   const char* PASSWORD = "你的密碼";
   #define TEST_MODE   0
   #define SEND_ENTER  0   // 先確認密碼正確，再改為 1
```
8. 冷啟動後檢查圓點數量、手動按 Enter 能解鎖，確認無誤後將 `SEND_ENTER` 改為 `1`

### 重新配對

ESP32 開機後 5 秒內按一下 BOOT 鍵，會進入配對模式，可綁定另一台 Mac。
先在 Mac 的藍牙設定中移除舊的 `ESP32 Unlock KB` 再配對。

### 可調整的參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `WAIT_AFTER_AUTH` | 5000 | 認證後等待多久才輸入（毫秒） |
| `MIN_DOWN_MS` | 30000 | 斷線超過多久才視為冷啟動（毫秒） |

## 注意事項

- 開機後有 5 秒的配對等待期，這段時間藍牙尚未啟動，Mac 不會連上。
- 密碼以明文存在 ESP32 韌體中，取得板子的人可以讀出密碼。
  這個做法只能防「單獨被偷走的 Mac」，防不了「連 ESP32 一起被拿走」。
- 第一次配對請在自己家中、身邊沒有陌生人時進行，配對完成後才會鎖定。
- ESP32 每次重新通電後的第一次連線都會輸入密碼。若 Mac 已登入時
  ESP32 意外斷電重啟，密碼會被輸入到目前聚焦的視窗，請確保供電穩定。
- Mac 已登入時，若 ESP32 斷線超過 `MIN_DOWN_MS` 後重連，同樣會輸入密碼，
  請視使用習慣調整此參數。
- 密碼以美式鍵盤按鍵碼送出，Mac 若使用其他鍵盤配置，特殊符號可能對不上。
- 密碼直接寫在 `unlock_kb.ino` 中，**提交前請確認 `PASSWORD` 為 `CHANGE_ME`**。
  若密碼曾被 commit，請直接更換 Mac 登入密碼。

## 授權

MIT License

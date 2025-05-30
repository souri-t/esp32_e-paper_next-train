#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <SPI.h>

// 電子ペーパー設定（WaveShare e-Paper ESP32 Driver Board + 1.54inch用）
#define EPD_CS 15
#define EPD_DC 27
#define EPD_RST 26
#define EPD_BUSY 25
#define EPD_SCK 13
#define EPD_MISO 12
#define EPD_MOSI 14

GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// WiFi設定管理
Preferences preferences;
WebServer server(80);
String savedSSID = "";
String savedPassword = "";
bool isConfigMode = false;
bool wifiConnected = false;

// 時刻設定
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 9 * 3600;  // JST (UTC+9)
const int daylightOffset_sec = 0;

// 発車時刻データ構造
struct TrainSchedule {
  int hour;
  int minute;
  String destination;
  String trainType;
};

// 最寄駅の発車時刻データ（例：平日ダイヤ）
TrainSchedule weekdaySchedule[] = {
  { 5, 7, "代々木上原", "普通" },
  { 5, 27, "代々木上原", "普通" },
  { 5, 40, "代々木上原", "普通" },
  { 5, 46, "代々木上原", "普通" },
  { 6, 0, "代々木上原", "普通" },
  { 6, 7, "綾瀬", "普通" },
  { 6, 14, "代々木上原", "普通" },
  { 6, 20, "霞ヶ関", "普通" },
  { 6, 26, "綾瀬", "普通" },
  { 6, 33, "代々木上原", "普通" },
  { 6, 39, "代々木上原", "普通" },
  { 6, 44, "綾瀬", "普通" },
  { 6, 51, "代々木上原", "普通" },
  { 6, 56, "綾瀬", "普通" },
  { 7, 3, "成城学園前", "普通" },
  { 7, 14, "相模大野", "普通" },
  { 7, 20, "代々木上原", "普通" },
  { 7, 27, "綾瀬", "普通" },
  { 7, 37, "代々木上原", "普通" },
  { 7, 45, "綾瀬", "普通" },
  { 7, 54, "向ヶ丘遊園", "普通" },
  { 8, 1, "綾瀬", "普通" },
  { 8, 12, "成城学園前", "普通" },
  { 8, 19, "綾瀬", "普通" },
  { 8, 29, "代々木上原", "普通" },
  { 8, 34, "綾瀬", "普通" },
  { 8, 47, "綾瀬", "普通" },
  { 8, 57, "代々木上原", "普通" },
  { 9, 4, "綾瀬", "普通" },
  { 9, 16, "代々木上原", "普通" },
  { 9, 25, "綾瀬", "普通" },
  { 9, 37, "綾瀬", "普通" },
  { 9, 53, "綾瀬", "普通" },
  { 10, 5, "代々木上原", "普通" },
  { 10, 17, "綾瀬", "普通" },
  { 10, 27, "代々木上原", "普通" },
  { 10, 43, "綾瀬", "普通" },
  { 10, 58, "代々木上原", "普通" },
  { 11, 6, "綾瀬", "普通" },
  { 11, 17, "代々木上原", "普通" },
  { 11, 26, "綾瀬", "普通" },
  { 11, 37, "代々木上原", "普通" },
  { 11, 47, "代々木上原", "普通" },
  { 11, 56, "綾瀬", "普通" },
  { 12, 7, "代々木上原", "普通" },
  { 12, 17, "代々木上原", "普通" },
  { 12, 26, "綾瀬", "普通" },
  { 12, 37, "代々木上原", "普通" },
  { 12, 46, "綾瀬", "普通" },
  { 12, 56, "綾瀬", "普通" },
  { 13, 7, "代々木上原", "普通" },
  { 13, 17, "代々木上原", "普通" },
  { 13, 26, "綾瀬", "普通" },
  { 13, 37, "代々木上原", "普通" },
  { 13, 46, "綾瀬", "普通" },
  { 13, 57, "代々木上原", "普通" },
  { 14, 6, "綾瀬", "普通" },
  { 14, 17, "代々木上原", "普通" },
  { 14, 26, "綾瀬", "普通" },
  { 14, 37, "代々木上原", "普通" },
  { 14, 46, "綾瀬", "普通" },
  { 14, 57, "代々木上原", "普通" },
  { 15, 6, "綾瀬", "普通" },
  { 15, 17, "代々木上原", "普通" },
  { 15, 26, "綾瀬", "普通" },
  { 15, 37, "代々木上原", "普通" },
  { 15, 46, "綾瀬", "普通" },
  { 15, 57, "綾瀬", "普通" },
  { 16, 7, "代々木上原", "普通" },
  { 16, 17, "綾瀬", "普通" },
  { 16, 27, "代々木上原", "普通" },
  { 16, 38, "綾瀬", "普通" },
  { 16, 47, "代々木上原", "普通" },
  { 16, 57, "代々木上原", "普通" },
  { 17, 6, "代々木上原", "普通" },
  { 17, 16, "綾瀬", "普通" },
  { 17, 25, "代々木上原", "普通" },
  { 17, 32, "向ヶ丘遊園", "普通" },
  { 17, 44, "伊勢原", "普通" },
  { 17, 56, "代々木上原", "普通" },
  { 18, 2, "綾瀬", "普通" },
  { 18, 10, "向ヶ丘遊園（準急）", "普通" },
  { 18, 15, "代々木上原", "普通" },
  { 18, 24, "伊勢原（急行）", "普通" },
  { 18, 31, "向ヶ丘遊園（準急）", "普通" },
  { 18, 37, "綾瀬", "普通" },
  { 18, 45, "伊勢原（急行）", "普通" },
  { 18, 53, "綾瀬", "普通" },
  { 18, 57, "綾瀬", "普通" },
  { 19, 5, "代々木上原", "普通" },
  { 19, 10, "綾瀬", "普通" },
  { 19, 16, "代々木上原", "普通" },
  { 19, 24, "綾瀬", "普通" },
  { 19, 30, "代々木上原", "普通" },
  { 19, 37, "綾瀬", "普通" },
  { 19, 44, "綾瀬", "普通" },
  { 19, 54, "綾瀬", "普通" },
  { 20, 7, "綾瀬", "普通" },
  { 20, 19, "綾瀬", "普通" },
  { 20, 26, "綾瀬", "普通" },
  { 20, 34, "綾瀬", "普通" },
  { 20, 46, "綾瀬", "普通" },
  { 20, 58, "綾瀬", "普通" },
  { 21, 6, "代々木上原", "普通" },
  { 21, 15, "綾瀬", "普通" },
  { 21, 28, "綾瀬", "普通" },
  { 21, 35, "代々木上原", "普通" },
  { 21, 44, "綾瀬", "普通" },
  { 21, 55, "代々木上原", "普通" },
  { 22, 6, "綾瀬", "普通" },
  { 22, 22, "代々木上原", "普通" },
  { 22, 30, "綾瀬", "普通" },
  { 22, 43, "綾瀬", "普通" },
  { 22, 55, "綾瀬", "普通" },
  { 23, 7, "綾瀬", "普通" },
  { 23, 22, "代々木上原", "普通" },
  { 23, 36, "綾瀬", "普通" },
  { 23, 50, "代々木上原", "普通" },
  { 24, 8, "綾瀬", "普通" }
};
const int weekdayScheduleSize = sizeof(weekdaySchedule) / sizeof(TrainSchedule);

// 最寄駅の発車時刻データ（例：休日ダイヤ）
TrainSchedule holidaySchedule[] = {
  { 5, 7, "代々木上原", "普通" },
  { 5, 27, "代々木上原", "普通" },
  { 5, 40, "代々木上原", "普通" },
  { 5, 46, "代々木上原", "普通" },
  { 6, 0, "代々木上原", "普通" },
  { 6, 7, "綾瀬", "普通" },
  { 6, 14, "代々木上原", "普通" },
  { 6, 20, "霞ヶ関", "普通" },
  { 6, 26, "綾瀬", "普通" },
  { 6, 33, "代々木上原", "普通" },
  { 6, 39, "代々木上原", "普通" },
  { 6, 44, "綾瀬", "普通" },
  { 6, 51, "代々木上原", "普通" },
  { 6, 56, "綾瀬", "普通" },
  { 7, 3, "成城学園前", "普通" },
  { 7, 14, "相模大野", "普通" },
  { 7, 20, "代々木上原", "普通" },
  { 7, 27, "綾瀬", "普通" },
  { 7, 37, "代々木上原", "普通" },
  { 7, 45, "綾瀬", "普通" },
  { 7, 54, "向ヶ丘遊園", "普通" },
  { 8, 1, "綾瀬", "普通" },
  { 8, 12, "成城学園前", "普通" },
  { 8, 19, "綾瀬", "普通" },
  { 8, 29, "代々木上原", "普通" },
  { 8, 34, "綾瀬", "普通" },
  { 8, 47, "綾瀬", "普通" },
  { 8, 57, "代々木上原", "普通" },
  { 9, 4, "綾瀬", "普通" },
  { 9, 16, "代々木上原", "普通" },
  { 9, 25, "綾瀬", "普通" },
  { 9, 37, "綾瀬", "普通" },
  { 9, 53, "綾瀬", "普通" },
  { 10, 5, "代々木上原", "普通" },
  { 10, 17, "綾瀬", "普通" },
  { 10, 27, "代々木上原", "普通" },
  { 10, 43, "綾瀬", "普通" },
  { 10, 58, "代々木上原", "普通" },
  { 11, 6, "綾瀬", "普通" },
  { 11, 17, "代々木上原", "普通" },
  { 11, 26, "綾瀬", "普通" },
  { 11, 37, "代々木上原", "普通" },
  { 11, 47, "代々木上原", "普通" },
  { 11, 56, "綾瀬", "普通" },
  { 12, 7, "代々木上原", "普通" },
  { 12, 17, "代々木上原", "普通" },
  { 12, 26, "綾瀬", "普通" },
  { 12, 37, "代々木上原", "普通" },
  { 12, 46, "綾瀬", "普通" },
  { 12, 56, "綾瀬", "普通" },
  { 13, 7, "代々木上原", "普通" },
  { 13, 17, "代々木上原", "普通" },
  { 13, 26, "綾瀬", "普通" },
  { 13, 37, "代々木上原", "普通" },
  { 13, 46, "綾瀬", "普通" },
  { 13, 57, "代々木上原", "普通" },
  { 14, 6, "綾瀬", "普通" },
  { 14, 17, "代々木上原", "普通" },
  { 14, 26, "綾瀬", "普通" },
  { 14, 37, "代々木上原", "普通" },
  { 14, 46, "綾瀬", "普通" },
  { 14, 57, "代々木上原", "普通" },
  { 15, 6, "綾瀬", "普通" },
  { 15, 17, "代々木上原", "普通" },
  { 15, 26, "綾瀬", "普通" },
  { 15, 37, "代々木上原", "普通" },
  { 15, 46, "綾瀬", "普通" },
  { 15, 57, "綾瀬", "普通" },
  { 16, 7, "代々木上原", "普通" },
  { 16, 17, "綾瀬", "普通" },
  { 16, 27, "代々木上原", "普通" },
  { 16, 38, "綾瀬", "普通" },
  { 16, 47, "代々木上原", "普通" },
  { 16, 57, "代々木上原", "普通" },
  { 17, 6, "代々木上原", "普通" },
  { 17, 16, "綾瀬", "普通" },
  { 17, 25, "代々木上原", "普通" },
  { 17, 32, "向ヶ丘遊園", "普通" },
  { 17, 44, "伊勢原", "普通" },
  { 17, 56, "代々木上原", "普通" },
  { 18, 2, "綾瀬", "普通" },
  { 18, 10, "向ヶ丘遊園（準急）", "普通" },
  { 18, 15, "代々木上原", "普通" },
  { 18, 24, "伊勢原（急行）", "普通" },
  { 18, 31, "向ヶ丘遊園（準急）", "普通" },
  { 18, 37, "綾瀬", "普通" },
  { 18, 45, "伊勢原（急行）", "普通" },
  { 18, 53, "綾瀬", "普通" },
  { 18, 57, "綾瀬", "普通" },
  { 19, 5, "代々木上原", "普通" },
  { 19, 10, "綾瀬", "普通" },
  { 19, 16, "代々木上原", "普通" },
  { 19, 24, "綾瀬", "普通" },
  { 19, 30, "代々木上原", "普通" },
  { 19, 37, "綾瀬", "普通" },
  { 19, 44, "綾瀬", "普通" },
  { 19, 54, "綾瀬", "普通" },
  { 20, 7, "綾瀬", "普通" },
  { 20, 19, "綾瀬", "普通" },
  { 20, 26, "綾瀬", "普通" },
  { 20, 34, "綾瀬", "普通" },
  { 20, 46, "綾瀬", "普通" },
  { 20, 58, "綾瀬", "普通" },
  { 21, 6, "代々木上原", "普通" },
  { 21, 15, "綾瀬", "普通" },
  { 21, 28, "綾瀬", "普通" },
  { 21, 35, "代々木上原", "普通" },
  { 21, 44, "綾瀬", "普通" },
  { 21, 55, "代々木上原", "普通" },
  { 22, 6, "綾瀬", "普通" },
  { 22, 22, "代々木上原", "普通" },
  { 22, 30, "綾瀬", "普通" },
  { 22, 43, "綾瀬", "普通" },
  { 22, 55, "綾瀬", "普通" },
  { 23, 7, "綾瀬", "普通" },
  { 23, 22, "代々木上原", "普通" },
  { 23, 36, "綾瀬", "普通" },
  { 23, 50, "代々木上原", "普通" },
  { 24, 8, "綾瀬", "普通" }
};
const int holidayScheduleSize = sizeof(holidaySchedule) / sizeof(TrainSchedule);

// 現在のダイヤ種別
TrainSchedule* currentSchedule;
int currentScheduleSize;

void setup() {
  Serial.begin(115200);
  Serial.println("発車時刻表示システム開始");
  // SPI設定（Driver Board用）
  SPI.begin(EPD_SCK, EPD_MISO, EPD_MOSI, EPD_CS);
  // 電子ペーパー初期化
  Serial.println("電子ペーパー初期化中...");
  display.init(115200, true, 2, false);
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);
  // Preferences初期化
  preferences.begin("wifi-config", false);
  // 保存されたWiFi設定を読み込み
  savedSSID = preferences.getString("ssid", "");
  savedPassword = preferences.getString("password", "");
  if (savedSSID.length() > 0) {
    Serial.println("保存されたWiFi設定でに接続を試行");
    displayMessage("WiFi接続中", savedSSID.c_str());
    // 保存されたWiFi設定で接続試行
    wifiConnected = connectToWiFi(savedSSID, savedPassword);
    if (wifiConnected) {
      Serial.println("WiFi接続成功");
      setupNTP();
      displayMessage("接続完了", WiFi.localIP().toString().c_str());
      delay(2000);
      updateDisplay();
    } else {
      Serial.println("保存されたWiFi設定で接続失敗");
      startConfigMode();
    }
  } else {
    Serial.println("WiFi設定が保存されていません");
    startConfigMode();
  }
}

void loop() {
  if (isConfigMode) {
    server.handleClient();
    delay(10);
  } else if (wifiConnected) {
    // 毎分更新（60秒待機）
    delay(60000);
    updateDisplay();
  } else {
    // WiFi接続失敗状態
    delay(5000);
    // 再接続試行
    if (savedSSID.length() > 0) {
      wifiConnected = connectToWiFi(savedSSID, savedPassword);
      if (wifiConnected) {
        setupNTP();
        isConfigMode = false;
      }
    }
  }
}

bool connectToWiFi(String ssid, String password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("WiFi接続完了 IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("WiFi接続タイムアウト");
    return false;
  }
}

void startConfigMode() {
  Serial.println("設定モード開始");
  isConfigMode = true;
  // アクセスポイントモードで起動
  WiFi.mode(WIFI_AP);
  WiFi.softAP("TrainDisplay_Setup", "12345678");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("設定用AP開始 IP: ");
  Serial.println(IP);
  displayMessage("設定モード", "TrainDisplay_Setup");
  // Webサーバー設定
  setupWebServer();
  server.begin();
  Serial.println("Webサーバー開始");
}

void setupWebServer() {
  // メイン設定ページ
  server.on("/", handleRoot);
  // WiFi設定保存
  server.on("/save", HTTP_POST, handleSave);
  // WiFiスキャン
  server.on("/scan", handleScan);
  // リセット
  server.on("/reset", handleReset);
  server.onNotFound(handleRoot);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>発車時刻表示 WiFi設定</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
  html += ".container{max-width:400px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
  html += "h1{color:#333;text-align:center;margin-bottom:20px}";
  html += "input,select,button{width:100%;padding:12px;margin:8px 0;border:1px solid #ddd;border-radius:4px;box-sizing:border-box}";
  html += "button{background:#4CAF50;color:white;cursor:pointer;font-size:16px}";
  html += "button:hover{background:#45a049}";
  html += ".scan-btn{background:#2196F3}";
  html += ".scan-btn:hover{background:#1976D2}";
  html += ".reset-btn{background:#f44336}";
  html += ".reset-btn:hover{background:#d32f2f}";
  html += ".network{padding:8px;margin:4px 0;background:#f9f9f9;border-radius:4px;cursor:pointer}";
  html += ".network:hover{background:#e9e9e9}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>🚃 発車時刻表示<br>WiFi設定</h1>";
  html += "<form method='POST' action='/save'>";
  html += "<label>WiFiネットワーク:</label>";
  html += "<select name='ssid' id='ssid'>";
  html += "<option value=''>ネットワークを選択</option>";
  html += "</select>";
  html += "<input type='text' name='custom_ssid' placeholder='または手動入力' id='custom_ssid'>";
  html += "<input type='password' name='password' placeholder='パスワード' required>";
  html += "<button type='submit'>保存して接続</button>";
  html += "</form>";
  html += "<button onclick='scanNetworks()' class='scan-btn'>WiFiスキャン</button>";
  html += "<button onclick='resetConfig()' class='reset-btn'>設定リセット</button>";
  html += "<div id='networks'></div>";
  html += "</div>";
  html += "<script>";
  html += "function scanNetworks(){";
  html += "document.getElementById('networks').innerHTML='<p>スキャン中...</p>';";
  html += "fetch('/scan').then(r=>r.text()).then(data=>{";
  html += "document.getElementById('networks').innerHTML=data;";
  html += "});}";
  html += "function selectNetwork(ssid){";
  html += "document.getElementById('ssid').value=ssid;";
  html += "document.getElementById('custom_ssid').value='';}";
  html += "function resetConfig(){";
  html += "if(confirm('設定をリセットしますか？')){";
  html += "fetch('/reset').then(()=>location.reload());}}";
  html += "</script></body></html>";
  server.send(200, "text/html", html);
}

void handleScan() {
  Serial.println("WiFiスキャン開始");
  int n = WiFi.scanNetworks();
  String networks = "";
  if (n == 0) {
    networks = "<p>ネットワークが見つかりません</p>";
  } else {
    networks = "<h3>検出されたネットワーク:</h3>";
    for (int i = 0; i < n; i++) {
      String ssid = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      String security = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "🔓" : "🔒";
      networks += "<div class='network' onclick='selectNetwork(\"" + ssid + "\")'>";
      networks += security + " " + ssid + " (" + String(rssi) + "dBm)";
      networks += "</div>";
    }
  }
  server.send(200, "text/html", networks);
}

void handleSave() {
  String ssid = server.arg("ssid");
  String customSSID = server.arg("custom_ssid");
  String password = server.arg("password");
  // カスタム入力がある場合はそちらを優先
  if (customSSID.length() > 0) {
    ssid = customSSID;
  }
  Serial.println("WiFi設定保存: " + ssid);
  // 設定を保存
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='5;url=/'>";
  html += "</head><body>";
  html += "<div style='text-align:center;margin:50px;'>";
  html += "<h2>設定を保存しました</h2>";
  html += "<p>" + ssid + "に接続中...</p>";
  html += "<p>5秒後に自動的にリダイレクトします</p>";
  html += "</div></body></html>";
  server.send(200, "text/html", html);
  delay(1000);
  // WiFi接続試行
  savedSSID = ssid;
  savedPassword = password;
  if (connectToWiFi(ssid, password)) {
    wifiConnected = true;
    isConfigMode = false;
    setupNTP();
    displayMessage("接続完了", WiFi.localIP().toString().c_str());
    delay(2000);
    updateDisplay();
  } else {
    displayMessage("接続失敗", "設定を確認してください");
  }
}

void handleReset() {
  Serial.println("設定リセット");
  preferences.clear();
  savedSSID = "";
  savedPassword = "";
  server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body><h2>設定をリセットしました</h2><a href='/'>戻る</a></body></html>");
}

void setupNTP() {
  Serial.println("NTP時刻同期中...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  int attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts < 10) {
    delay(1000);
    attempts++;
  }
  if (attempts < 10) {
    Serial.println("時刻同期完了");
  } else {
    Serial.println("時刻同期失敗");
  }
}

void displayMessage(const char* title, const char* message) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(5, 30);
    display.print(title);
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 60);
    display.print(message);
    if (isConfigMode) {
      display.setCursor(5, 90);
      display.print("SSID:");
      display.setCursor(5, 110);
      display.print("TrainDisplay_Setup");
      display.setCursor(5, 130);
      display.print("Pass: 12345678");
      display.setCursor(5, 150);
      display.print("IP: 192.168.4.1");
    }
  } while (display.nextPage());
}

void updateDisplay() {
  if (!wifiConnected) return;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("時刻取得エラー");
    return;
  }

  // 曜日を判定してダイヤを選択
  selectSchedule(timeinfo);
  int currentHour = timeinfo.tm_hour;
  int currentMinute = timeinfo.tm_min;
  // 次の発車時刻を検索
  TrainSchedule nextTrain = findNextTrain(currentHour, currentMinute);
  // 現在時刻までの分数を計算
  int minutesUntilNext = calculateMinutesUntil(currentHour, currentMinute, nextTrain.hour, nextTrain.minute);
  // 画面クリア
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    // 現在時刻表示（小さめフォント）
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 18);
    char currentTimeStr[20];
    sprintf(currentTimeStr, "%02d/%02d (%s) %02d:%02d",
            timeinfo.tm_mon + 1, timeinfo.tm_mday,
            getWeekdayString(timeinfo.tm_wday),
            currentHour, currentMinute);
    display.print(currentTimeStr);
    // 線を描画
    display.drawLine(0, 25, display.width(), 25, GxEPD_BLACK);
    // 次の電車情報（メイン表示）
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(5, 50);
    display.print("次の電車");
    // 発車時刻を大きく表示
    display.setFont(&FreeMonoBold18pt7b);
    display.setCursor(5, 80);
    char nextTrainStr[20];
    sprintf(nextTrainStr, "%02d:%02d", nextTrain.hour, nextTrain.minute);
    display.print(nextTrainStr);
    // 行き先と種別（小さめフォント）
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 100);
    String destInfo = nextTrain.trainType + " " + nextTrain.destination;
    // 文字数制限（1.54inchは幅が狭いため）
    if (destInfo.length() > 12) {
      destInfo = destInfo.substring(0, 12) + "..";
    }
    display.print(destInfo);
    // 残り時間表示（強調）
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(5, 130);
    if (minutesUntilNext == 0) {
      display.print("まもなく");
    } else if (minutesUntilNext == 1) {
      display.print("あと1分");
    } else if (minutesUntilNext < 10) {
      char minutesStr[15];
      sprintf(minutesStr, "あと%d分", minutesUntilNext);
      display.print(minutesStr);
    } else {
      char minutesStr[15];
      sprintf(minutesStr, "%d分後", minutesUntilNext);
      display.print(minutesStr);
    }
    // 線を描画
    display.drawLine(0, 140, display.width(), 140, GxEPD_BLACK);
    // その次の電車情報（簡潔に）
    TrainSchedule nextNextTrain = findNextTrain(nextTrain.hour, nextTrain.minute + 1);
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 160);
    display.print("その次");
    display.setCursor(5, 180);
    char nextNextStr[25];
    sprintf(nextNextStr, "%02d:%02d %s",
            nextNextTrain.hour, nextNextTrain.minute,
            nextNextTrain.destination.c_str());
    // 文字数制限
    String nextNextDisplay = String(nextNextStr);
    if (nextNextDisplay.length() > 15) {
      nextNextDisplay = nextNextDisplay.substring(0, 15) + "..";
    }
    display.print(nextNextDisplay);
  } while (display.nextPage());
  Serial.printf("表示更新: %02d:%02d, 次の電車: %02d:%02d (%d分後)\n",
                currentHour, currentMinute, nextTrain.hour, nextTrain.minute, minutesUntilNext);
}

// 曜日によってダイヤを切り替える関数（土日のみ休日ダイヤ）
void selectSchedule(struct tm timeinfo) {
  // tm_wday: 日曜日=0, 月曜日=1, ..., 土曜日=6
  if (timeinfo.tm_wday == 0 || timeinfo.tm_wday == 6) {  // 日曜日または土曜日
    currentSchedule = holidaySchedule;
    currentScheduleSize = holidayScheduleSize;
    Serial.println("休日ダイヤを適用");
  } else {
    currentSchedule = weekdaySchedule;
    currentScheduleSize = weekdayScheduleSize;
    Serial.println("平日ダイヤを適用");
  }
}

// 曜日文字列取得
const char* getWeekdayString(int wday) {
  switch (wday) {
    case 0: return "Sun";
    case 1: return "Mon";
    case 2: return "Tue";
    case 3: return "Wed";
    case 4: return "Thu";
    case 5: return "Fri";
    case 6: return "Sat";
    default: return "";
  }
}

TrainSchedule findNextTrain(int currentHour, int currentMinute) {
  int currentTotalMinutes = currentHour * 60 + currentMinute;
  // 今日の残りの電車を検索
  for (int i = 0; i < currentScheduleSize; i++) {
    int trainTotalMinutes = currentSchedule[i].hour * 60 + currentSchedule[i].minute;
    if (trainTotalMinutes > currentTotalMinutes) {
      return currentSchedule[i];
    }
  }
  // 今日の電車がない場合は翌日の最初の電車
  return currentSchedule[0];
}

int calculateMinutesUntil(int currentHour, int currentMinute, int targetHour, int targetMinute) {
  int currentTotalMinutes = currentHour * 60 + currentMinute;
  int targetTotalMinutes = targetHour * 60 + targetMinute;
  if (targetTotalMinutes > currentTotalMinutes) {
    return targetTotalMinutes - currentTotalMinutes;
  } else {
    // 翌日の場合
    return (24 * 60) - currentTotalMinutes + targetTotalMinutes;
  }
}
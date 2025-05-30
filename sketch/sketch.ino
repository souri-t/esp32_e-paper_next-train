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
  String destination; // 日本語から英語表記に変更
  String trainType;   // 日本語から英語表記に変更
};

// 最寄駅の発車時刻データ（例：平日ダイヤ）
TrainSchedule weekdaySchedule[] = {
  { 5, 7, "Yoyogi Uehara", "Local" },
  { 5, 27, "Yoyogi Uehara", "Local" },
  { 5, 40, "Yoyogi Uehara", "Local" },
  { 5, 46, "Yoyogi Uehara", "Local" },
  { 6, 0, "Yoyogi Uehara", "Local" },
  { 6, 7, "Ayase", "Local" },
  { 6, 14, "Yoyogi Uehara", "Local" },
  { 6, 20, "Kasumigaseki", "Local" },
  { 6, 26, "Ayase", "Local" },
  { 6, 33, "Yoyogi Uehara", "Local" },
  { 6, 39, "Yoyogi Uehara", "Local" },
  { 6, 44, "Ayase", "Local" },
  { 6, 51, "Yoyogi Uehara", "Local" },
  { 6, 56, "Ayase", "Local" },
  { 7, 3, "Seijo Gakuen-mae", "Local" },
  { 7, 14, "Sagami-Ono", "Local" },
  { 7, 20, "Yoyogi Uehara", "Local" },
  { 7, 27, "Ayase", "Local" },
  { 7, 37, "Yoyogi Uehara", "Local" },
  { 7, 45, "Ayase", "Local" },
  { 7, 54, "Mukogaoka-Yuen", "Local" },
  { 8, 1, "Ayase", "Local" },
  { 8, 12, "Seijo Gakuen-mae", "Local" },
  { 8, 19, "Ayase", "Local" },
  { 8, 29, "Yoyogi Uehara", "Local" },
  { 8, 34, "Ayase", "Local" },
  { 8, 47, "Ayase", "Local" },
  { 8, 57, "Yoyogi Uehara", "Local" },
  { 9, 4, "Ayase", "Local" },
  { 9, 16, "Yoyogi Uehara", "Local" },
  { 9, 25, "Ayase", "Local" },
  { 9, 37, "Ayase", "Local" },
  { 9, 53, "Ayase", "Local" },
  { 10, 5, "Yoyogi Uehara", "Local" },
  { 10, 17, "Ayase", "Local" },
  { 10, 27, "Yoyogi Uehara", "Local" },
  { 10, 43, "Ayase", "Local" },
  { 10, 58, "Yoyogi Uehara", "Local" },
  { 11, 6, "Ayase", "Local" },
  { 11, 17, "Yoyogi Uehara", "Local" },
  { 11, 26, "Ayase", "Local" },
  { 11, 37, "Yoyogi Uehara", "Local" },
  { 11, 47, "Yoyogi Uehara", "Local" },
  { 11, 56, "Ayase", "Local" },
  { 12, 7, "Yoyogi Uehara", "Local" },
  { 12, 17, "Yoyogi Uehara", "Local" },
  { 12, 26, "Ayase", "Local" },
  { 12, 37, "Yoyogi Uehara", "Local" },
  { 12, 46, "Ayase", "Local" },
  { 12, 56, "Ayase", "Local" },
  { 13, 7, "Yoyogi Uehara", "Local" },
  { 13, 17, "Yoyogi Uehara", "Local" },
  { 13, 26, "Ayase", "Local" },
  { 13, 37, "Yoyogi Uehara", "Local" },
  { 13, 46, "Ayase", "Local" },
  { 13, 57, "Yoyogi Uehara", "Local" },
  { 14, 6, "Ayase", "Local" },
  { 14, 17, "Yoyogi Uehara", "Local" },
  { 14, 26, "Ayase", "Local" },
  { 14, 37, "Yoyogi Uehara", "Local" },
  { 14, 46, "Ayase", "Local" },
  { 14, 57, "Yoyogi Uehara", "Local" },
  { 15, 6, "Ayase", "Local" },
  { 15, 17, "Yoyogi Uehara", "Local" },
  { 15, 26, "Ayase", "Local" },
  { 15, 37, "Yoyogi Uehara", "Local" },
  { 15, 46, "Ayase", "Local" },
  { 15, 57, "Ayase", "Local" },
  { 16, 7, "Yoyogi Uehara", "Local" },
  { 16, 17, "Ayase", "Local" },
  { 16, 27, "Yoyogi Uehara", "Local" },
  { 16, 38, "Ayase", "Local" },
  { 16, 47, "Yoyogi Uehara", "Local" },
  { 16, 57, "Yoyogi Uehara", "Local" },
  { 17, 6, "Yoyogi Uehara", "Local" },
  { 17, 16, "Ayase", "Local" },
  { 17, 25, "Yoyogi Uehara", "Local" },
  { 17, 32, "Mukogaoka-Yuen", "Local" },
  { 17, 44, "Isehara", "Local" },
  { 17, 56, "Yoyogi Uehara", "Local" },
  { 18, 2, "Ayase", "Local" },
  { 18, 10, "Mukogaoka-Yuen (Semi-Exp)", "Local" },
  { 18, 15, "Yoyogi Uehara", "Local" },
  { 18, 24, "Isehara (Exp)", "Local" },
  { 18, 31, "Mukogaoka-Yuen (Semi-Exp)", "Local" },
  { 18, 37, "Ayase", "Local" },
  { 18, 45, "Isehara (Exp)", "Local" },
  { 18, 53, "Ayase", "Local" },
  { 18, 57, "Ayase", "Local" },
  { 19, 5, "Yoyogi Uehara", "Local" },
  { 19, 10, "Ayase", "Local" },
  { 19, 16, "Yoyogi Uehara", "Local" },
  { 19, 24, "Ayase", "Local" },
  { 19, 30, "Yoyogi Uehara", "Local" },
  { 19, 37, "Ayase", "Local" },
  { 19, 44, "Ayase", "Local" },
  { 19, 54, "Ayase", "Local" },
  { 20, 7, "Ayase", "Local" },
  { 20, 19, "Ayase", "Local" },
  { 20, 26, "Ayase", "Local" },
  { 20, 34, "Ayase", "Local" },
  { 20, 46, "Ayase", "Local" },
  { 20, 58, "Ayase", "Local" },
  { 21, 6, "Yoyogi Uehara", "Local" },
  { 21, 15, "Ayase", "Local" },
  { 21, 28, "Ayase", "Local" },
  { 21, 35, "Yoyogi Uehara", "Local" },
  { 21, 44, "Ayase", "Local" },
  { 21, 55, "Yoyogi Uehara", "Local" },
  { 22, 6, "Ayase", "Local" },
  { 22, 22, "Yoyogi Uehara", "Local" },
  { 22, 30, "Ayase", "Local" },
  { 22, 43, "Ayase", "Local" },
  { 22, 55, "Ayase", "Local" },
  { 23, 7, "Ayase", "Local" },
  { 23, 22, "Yoyogi Uehara", "Local" },
  { 23, 36, "Ayase", "Local" },
  { 23, 50, "Yoyogi Uehara", "Local" },
  { 24, 8, "Ayase", "Local" }
};
const int weekdayScheduleSize = sizeof(weekdaySchedule) / sizeof(TrainSchedule);

// 最寄駅の発車時刻データ（例：休日ダイヤ）
TrainSchedule holidaySchedule[] = {
  { 5, 7, "Yoyogi Uehara", "Local" },
  { 5, 27, "Yoyogi Uehara", "Local" },
  { 5, 40, "Yoyogi Uehara", "Local" },
  { 5, 46, "Yoyogi Uehara", "Local" },
  { 6, 0, "Yoyogi Uehara", "Local" },
  { 6, 7, "Ayase", "Local" },
  { 6, 14, "Yoyogi Uehara", "Local" },
  { 6, 20, "Kasumigaseki", "Local" },
  { 6, 26, "Ayase", "Local" },
  { 6, 33, "Yoyogi Uehara", "Local" },
  { 6, 39, "Yoyogi Uehara", "Local" },
  { 6, 44, "Ayase", "Local" },
  { 6, 51, "Yoyogi Uehara", "Local" },
  { 6, 56, "Ayase", "Local" },
  { 7, 3, "Seijo Gakuen-mae", "Local" },
  { 7, 14, "Sagami-Ono", "Local" },
  { 7, 20, "Yoyogi Uehara", "Local" },
  { 7, 27, "Ayase", "Local" },
  { 7, 37, "Yoyogi Uehara", "Local" },
  { 7, 45, "Ayase", "Local" },
  { 7, 54, "Mukogaoka-Yuen", "Local" },
  { 8, 1, "Ayase", "Local" },
  { 8, 12, "Seijo Gakuen-mae", "Local" },
  { 8, 19, "Ayase", "Local" },
  { 8, 29, "Yoyogi Uehara", "Local" },
  { 8, 34, "Ayase", "Local" },
  { 8, 47, "Ayase", "Local" },
  { 8, 57, "Yoyogi Uehara", "Local" },
  { 9, 4, "Ayase", "Local" },
  { 9, 16, "Yoyogi Uehara", "Local" },
  { 9, 25, "Ayase", "Local" },
  { 9, 37, "Ayase", "Local" },
  { 9, 53, "Ayase", "Local" },
  { 10, 5, "Yoyogi Uehara", "Local" },
  { 10, 17, "Ayase", "Local" },
  { 10, 27, "Yoyogi Uehara", "Local" },
  { 10, 43, "Ayase", "Local" },
  { 10, 58, "Yoyogi Uehara", "Local" },
  { 11, 6, "Ayase", "Local" },
  { 11, 17, "Yoyogi Uehara", "Local" },
  { 11, 26, "Ayase", "Local" },
  { 11, 37, "Yoyogi Uehara", "Local" },
  { 11, 47, "Yoyogi Uehara", "Local" },
  { 11, 56, "Ayase", "Local" },
  { 12, 7, "Yoyogi Uehara", "Local" },
  { 12, 17, "Yoyogi Uehara", "Local" },
  { 12, 26, "Ayase", "Local" },
  { 12, 37, "Yoyogi Uehara", "Local" },
  { 12, 46, "Ayase", "Local" },
  { 12, 56, "Ayase", "Local" },
  { 13, 7, "Yoyogi Uehara", "Local" },
  { 13, 17, "Yoyogi Uehara", "Local" },
  { 13, 26, "Ayase", "Local" },
  { 13, 37, "Yoyogi Uehara", "Local" },
  { 13, 46, "Ayase", "Local" },
  { 13, 57, "Yoyogi Uehara", "Local" },
  { 14, 6, "Ayase", "Local" },
  { 14, 17, "Yoyogi Uehara", "Local" },
  { 14, 26, "Ayase", "Local" },
  { 14, 37, "Yoyogi Uehara", "Local" },
  { 14, 46, "Ayase", "Local" },
  { 14, 57, "Yoyogi Uehara", "Local" },
  { 15, 6, "Ayase", "Local" },
  { 15, 17, "Yoyogi Uehara", "Local" },
  { 15, 26, "Ayase", "Local" },
  { 15, 37, "Yoyogi Uehara", "Local" },
  { 15, 46, "Ayase", "Local" },
  { 15, 57, "Ayase", "Local" },
  { 16, 7, "Yoyogi Uehara", "Local" },
  { 16, 17, "Ayase", "Local" },
  { 16, 27, "Yoyogi Uehara", "Local" },
  { 16, 38, "Ayase", "Local" },
  { 16, 47, "Yoyogi Uehara", "Local" },
  { 16, 57, "Yoyogi Uehara", "Local" },
  { 17, 6, "Yoyogi Uehara", "Local" },
  { 17, 16, "Ayase", "Local" },
  { 17, 25, "Yoyogi Uehara", "Local" },
  { 17, 32, "Mukogaoka-Yuen", "Local" },
  { 17, 44, "Isehara", "Local" },
  { 17, 56, "Yoyogi Uehara", "Local" },
  { 18, 2, "Ayase", "Local" },
  { 18, 10, "Mukogaoka-Yuen (Semi-Exp)", "Local" },
  { 18, 15, "Yoyogi Uehara", "Local" },
  { 18, 24, "Isehara (Exp)", "Local" },
  { 18, 31, "Mukogaoka-Yuen (Semi-Exp)", "Local" },
  { 18, 37, "Ayase", "Local" },
  { 18, 45, "Isehara (Exp)", "Local" },
  { 18, 53, "Ayase", "Local" },
  { 18, 57, "Ayase", "Local" },
  { 19, 5, "Yoyogi Uehara", "Local" },
  { 19, 10, "Ayase", "Local" },
  { 19, 16, "Yoyogi Uehara", "Local" },
  { 19, 24, "Ayase", "Local" },
  { 19, 30, "Yoyogi Uehara", "Local" },
  { 19, 37, "Ayase", "Local" },
  { 19, 44, "Ayase", "Local" },
  { 19, 54, "Ayase", "Local" },
  { 20, 7, "Ayase", "Local" },
  { 20, 19, "Ayase", "Local" },
  { 20, 26, "Ayase", "Local" },
  { 20, 34, "Ayase", "Local" },
  { 20, 46, "Ayase", "Local" },
  { 20, 58, "Ayase", "Local" },
  { 21, 6, "Yoyogi Uehara", "Local" },
  { 21, 15, "Ayase", "Local" },
  { 21, 28, "Ayase", "Local" },
  { 21, 35, "Yoyogi Uehara", "Local" },
  { 21, 44, "Ayase", "Local" },
  { 21, 55, "Yoyogi Uehara", "Local" },
  { 22, 6, "Ayase", "Local" },
  { 22, 22, "Yoyogi Uehara", "Local" },
  { 22, 30, "Ayase", "Local" },
  { 22, 43, "Ayase", "Local" },
  { 22, 55, "Ayase", "Local" },
  { 23, 7, "Ayase", "Local" },
  { 23, 22, "Yoyogi Uehara", "Local" },
  { 23, 36, "Ayase", "Local" },
  { 23, 50, "Yoyogi Uehara", "Local" },
  { 24, 8, "Ayase", "Local" }
};
const int holidayScheduleSize = sizeof(holidaySchedule) / sizeof(TrainSchedule);

// 現在のダイヤ種別
TrainSchedule* currentSchedule;
int currentScheduleSize;

void setup() {
  Serial.begin(115200);
  Serial.println("Train Schedule Display System Start"); // メッセージを英語に変更
  // SPI設定（Driver Board用）
  SPI.begin(EPD_SCK, EPD_MISO, EPD_MOSI, EPD_CS);
  // 電子ペーパー初期化
  Serial.println("Initializing e-Paper display..."); // メッセージを英語に変更
  display.init(115200, true, 2, false);
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);
  // Preferences初期化
  preferences.begin("wifi-config", false);
  // 保存されたWiFi設定を読み込み
  savedSSID = preferences.getString("ssid", "");
  savedPassword = preferences.getString("password", "");
  if (savedSSID.length() > 0) {
    Serial.println("Attempting to connect with saved WiFi credentials"); // メッセージを英語に変更
    displayMessage("Connecting to WiFi", savedSSID.c_str()); // 表示メッセージを英語に変更
    // 保存されたWiFi設定で接続試行
    wifiConnected = connectToWiFi(savedSSID, savedPassword);
    if (wifiConnected) {
      Serial.println("WiFi connection successful"); // メッセージを英語に変更
      setupNTP();
      displayMessage("Connected", WiFi.localIP().toString().c_str()); // 表示メッセージを英語に変更
      delay(2000);
      updateDisplay();
    } else {
      Serial.println("Failed to connect with saved WiFi credentials"); // メッセージを英語に変更
      startConfigMode();
    }
  } else {
    Serial.println("No WiFi configuration saved"); // メッセージを英語に変更
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
    Serial.print("WiFi Connected. IP: "); // メッセージを英語に変更
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("WiFi connection timeout"); // メッセージを英語に変更
    return false;
  }
}

void startConfigMode() {
  Serial.println("Starting configuration mode"); // メッセージを英語に変更
  isConfigMode = true;
  // アクセスポイントモードで起動
  WiFi.mode(WIFI_AP);
  WiFi.softAP("TrainDisplay_Setup", "12345678");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Configuration AP started. IP: "); // メッセージを英語に変更
  Serial.println(IP);
  displayMessage("Config Mode", "TrainDisplay_Setup"); // 表示メッセージを英語に変更
  // Webサーバー設定
  setupWebServer();
  server.begin();
  Serial.println("Web server started"); // メッセージを英語に変更
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
  html += "<title>Train Display WiFi Setup</title>"; // タイトルを英語に変更
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
  html += "<h1>🚃 Train Display<br>WiFi Setup</h1>"; // タイトルを英語に変更
  html += "<form method='POST' action='/save'>";
  html += "<label>WiFi Network:</label>"; // ラベルを英語に変更
  html += "<select name='ssid' id='ssid'>";
  html += "<option value=''>Select Network</option>"; // オプションを英語に変更
  html += "</select>";
  html += "<input type='text' name='custom_ssid' placeholder='Or enter manually' id='custom_ssid'>"; // プレースホルダーを英語に変更
  html += "<input type='password' name='password' placeholder='Password' required>"; // プレースホルダーを英語に変更
  html += "<button type='submit'>Save & Connect</button>"; // ボタンを英語に変更
  html += "</form>";
  html += "<button onclick='scanNetworks()' class='scan-btn'>Scan WiFi</button>"; // ボタンを英語に変更
  html += "<button onclick='resetConfig()' class='reset-btn'>Reset Settings</button>"; // ボタンを英語に変更
  html += "<div id='networks'></div>";
  html += "</div>";
  html += "<script>";
  html += "function scanNetworks(){";
  html += "document.getElementById('networks').innerHTML='<p>Scanning...</p>';"; // メッセージを英語に変更
  html += "fetch('/scan').then(r=>r.text()).then(data=>{";
  html += "document.getElementById('networks').innerHTML=data;";
  html += "});}";
  html += "function selectNetwork(ssid){";
  html += "document.getElementById('ssid').value=ssid;";
  html += "document.getElementById('custom_ssid').value='';}";
  html += "function resetConfig(){";
  html += "if(confirm('Are you sure you want to reset settings?')){"; // 確認メッセージを英語に変更
  html += "fetch('/reset').then(()=>location.reload());}}";
  html += "</script></body></html>";
  server.send(200, "text/html", html);
}

void handleScan() {
  Serial.println("Starting WiFi scan"); // メッセージを英語に変更
  int n = WiFi.scanNetworks();
  String networks = "";
  if (n == 0) {
    networks = "<p>No networks found</p>"; // メッセージを英語に変更
  } else {
    networks = "<h3>Detected Networks:</h3>"; // メッセージを英語に変更
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
  Serial.println("Saving WiFi settings: " + ssid); // メッセージを英語に変更
  // 設定を保存
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='5;url=/'>";
  html += "</head><body>";
  html += "<div style='text-align:center;margin:50px;'>";
  html += "<h2>Settings saved</h2>"; // メッセージを英語に変更
  html += "<p>Connecting to " + ssid + "...</p>"; // メッセージを英語に変更
  html += "<p>Redirecting in 5 seconds</p>"; // メッセージを英語に変更
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
    displayMessage("Connected", WiFi.localIP().toString().c_str()); // 表示メッセージを英語に変更
    delay(2000);
    updateDisplay();
  } else {
    displayMessage("Connection Failed", "Check settings"); // 表示メッセージを英語に変更
  }
}

void handleReset() {
  Serial.println("Resetting settings"); // メッセージを英語に変更
  preferences.clear();
  savedSSID = "";
  savedPassword = "";
  server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body><h2>Settings have been reset</h2><a href='/'>Back</a></body></html>"); // メッセージを英語に変更
}

void setupNTP() {
  Serial.println("Synchronizing time with NTP..."); // メッセージを英語に変更
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  int attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts < 10) {
    delay(1000);
    attempts++;
  }
  if (attempts < 10) {
    Serial.println("Time synchronization complete"); // メッセージを英語に変更
  } else {
    Serial.println("Time synchronization failed"); // メッセージを英語に変更
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
    Serial.println("Error getting time"); // メッセージを英語に変更
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
    display.print("Next Train"); // "次の電車" を英語に変更
    // 発車時刻を大きく表示
    display.setFont(&FreeMonoBold18pt7b);
    display.setCursor(5, 80);
    char nextTrainStr[20];
    sprintf(nextTrainStr, "%02d:%02d", nextTrain.hour, nextTrain.minute);
    display.print(nextTrainStr);
    // 行き先と種別（小さめフォント）
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 100);
    String destInfo = nextTrain.destination;
    // 文字数制限（1.54inchは幅が狭いため）
    if (destInfo.length() > 17) {
      destInfo = destInfo.substring(0, 17) + "..";
    }
    display.print(destInfo);
    // 残り時間表示（強調）
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(5, 130);
    if (minutesUntilNext == 0) {
      display.print("Departing Soon"); // "まもなく" を英語に変更
    } else if (minutesUntilNext == 1) {
      display.print("1 min Left"); // "あと1分" を英語に変更
    } else {
      char minutesStr[15];
      sprintf(minutesStr, "%d min Left", minutesUntilNext); // "あと%d分" または "%d分後" を英語に変更
      display.print(minutesStr);
    }
    // 線を描画
    display.drawLine(0, 140, display.width(), 140, GxEPD_BLACK);
    // その次の電車情報（簡潔に）
    TrainSchedule nextNextTrain = findNextTrain(nextTrain.hour, nextTrain.minute + 1);
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 160);
    display.print("Next after"); // "その次" を英語に変更
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
  Serial.printf("Display updated: %02d:%02d, Next train: %02d:%02d (%d min left)\n", // メッセージを英語に変更
                currentHour, currentMinute, nextTrain.hour, nextTrain.minute, minutesUntilNext);
}

// 曜日によってダイヤを切り替える関数（土日のみ休日ダイヤ）
void selectSchedule(struct tm timeinfo) {
  // tm_wday: 日曜日=0, 月曜日=1, ..., 土曜日=6
  if (timeinfo.tm_wday == 0 || timeinfo.tm_wday == 6) {  // 日曜日または土曜日
    currentSchedule = holidaySchedule;
    currentScheduleSize = holidayScheduleSize;
    Serial.println("Applying holiday schedule"); // メッセージを英語に変更
  } else {
    currentSchedule = weekdaySchedule;
    currentScheduleSize = weekdayScheduleSize;
    Serial.println("Applying weekday schedule"); // メッセージを英語に変更
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
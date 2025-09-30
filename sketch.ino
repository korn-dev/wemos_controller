#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

ESP8266WebServer server(80);

#define ONE_WIRE_BUS D1
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define MONITOR_PIN_1 D5
#define MONITOR_PIN_2 D6
#define MONITOR_PIN_3 D7

struct Settings {
  bool configured;
  char ssid[32];
  char password[64];
};

Settings settings;

String getChipId() {
  String chipId = String(ESP.getChipId(), HEX);
  chipId.toUpperCase();
  return chipId;
}

const char* mainPage = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Wemos D1 Mini Control</title>
    <style>
        :root {
            --md-sys-color-primary: #6750A4;
            --md-sys-color-surface: #FEF7FF;
            --md-sys-color-on-surface: #000000;
            --md-sys-color-surface-container: #F3EDF7;
            --md-sys-color-error: #BA1A1A;
            --md-sys-color-success: #0D8B47;
            --md-sys-color-warning: #FF6D00;
        }
        
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Roboto', Arial, sans-serif;
            background: linear-gradient(135deg, #a8edea 0%, #fed6e3 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
            color: #000000;
        }
        
        .glass-card {
            background: rgba(255, 255, 255, 0.25);
            backdrop-filter: blur(20px);
            -webkit-backdrop-filter: blur(20px);
            border-radius: 24px;
            border: 1px solid rgba(255, 255, 255, 0.18);
            padding: 32px;
            box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.37);
            max-width: 500px;
            width: 100%;
        }
        
        .header {
            text-align: center;
            margin-bottom: 32px;
        }
        
        .header h1 {
            color: #000000;
            font-size: 28px;
            font-weight: 500;
            margin-bottom: 8px;
        }
        
        .header p {
            color: #000000;
            font-size: 16px;
            opacity: 0.8;
        }
        
        .chip-id {
            background: rgba(103, 80, 164, 0.2);
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 14px;
            font-weight: 600;
            color: var(--md-sys-color-primary);
            border: 1px solid rgba(255, 255, 255, 0.5);
            margin-top: 10px;
            display: inline-block;
        }
        
        .temperature-value {
            font-size: 18px;
            font-weight: 700;
        }
        
        .temp-normal {
            color: var(--md-sys-color-success);
        }
        
        .temp-warning {
            color: var(--md-sys-color-warning);
        }
        
        .temp-critical {
            color: var(--md-sys-color-error);
        }
        
        .wifi-strength {
            color: var(--md-sys-color-success);
        }
        
        .wifi-weak {
            color: var(--md-sys-color-warning);
        }
        
        .wifi-poor {
            color: var(--md-sys-color-error);
        }
        
        .button-group {
            display: flex;
            gap: 12px;
            margin-top: 24px;
        }
        
        .md-button {
            flex: 1;
            padding: 16px 24px;
            border: none;
            border-radius: 20px;
            font-size: 16px;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.3s ease;
            text-align: center;
            text-decoration: none;
            display: inline-block;
            color: #000000;
        }
        
        .md-button.primary {
            background: var(--md-sys-color-primary);
            color: white;
        }
        
        .md-button.outlined {
            background: transparent;
            color: var(--md-sys-color-primary);
            border: 2px solid var(--md-sys-color-primary);
        }
        
        .md-button:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
        }
        
        .temperature-card {
            background: rgba(255, 255, 255, 0.4);
            border-radius: 16px;
            padding: 20px;
            margin-bottom: 15px;
            text-align: center;
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
        }
        
        .temp-icon {
            font-size: 48px;
            margin-bottom: 10px;
        }
        
        .temp-reading {
            font-size: 32px;
            font-weight: 700;
            margin: 10px 0;
        }
        
        .temp-unit {
            font-size: 16px;
            opacity: 0.8;
        }
        
        .monitor-cards {
            display: flex;
            gap: 10px;
            margin-bottom: 20px;
        }
        
        .monitor-card {
            flex: 1;
            background: rgba(255, 255, 255, 0.3);
            border-radius: 12px;
            padding: 15px;
            text-align: center;
            box-shadow: 0 2px 6px rgba(0, 0, 0, 0.1);
            transition: all 0.3s ease;
        }
        
        .monitor-card.active {
            background: rgba(13, 139, 71, 0.2);
            border: 2px solid var(--md-sys-color-success);
        }
        
        .monitor-card.alarm {
            background: rgba(186, 26, 26, 0.2);
            border: 2px solid var(--md-sys-color-error);
            animation: pulse 1s infinite;
        }
        
        @keyframes pulse {
            0% { transform: scale(1); opacity: 1; }
            50% { transform: scale(1.05); opacity: 0.8; }
            100% { transform: scale(1); opacity: 1; }
        }
        
        .monitor-icon {
            font-size: 24px;
            margin-bottom: 8px;
        }
        
        .monitor-label {
            font-size: 12px;
            font-weight: 600;
            color: #000000;
            opacity: 0.8;
        }
        
        .monitor-status {
            font-size: 10px;
            font-weight: 500;
            margin-top: 5px;
        }
        
        .status-normal {
            color: var(--md-sys-color-success);
        }
        
        .status-alarm {
            color: var(--md-sys-color-error);
        }
        
        .wifi-info {
            background: rgba(255, 255, 255, 0.3);
            border-radius: 12px;
            padding: 12px;
            margin-bottom: 15px;
            text-align: center;
        }
        
        .wifi-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin: 5px 0;
        }
        
        .wifi-label {
            font-size: 12px;
            font-weight: 500;
            color: #000000;
            opacity: 0.8;
        }
        
        .wifi-value {
            font-size: 12px;
            font-weight: 600;
            color: #000000;
        }
        
        .last-update {
            text-align: center;
            margin-top: 16px;
            font-size: 12px;
            color: #000000;
            opacity: 0.7;
        }
        
        .notification {
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 12px 20px;
            border-radius: 8px;
            color: white;
            font-weight: 500;
            transform: translateX(400px);
            transition: transform 0.3s ease;
            z-index: 1000;
        }
        
        .notification.show {
            transform: translateX(0);
        }
        
        .notification.success {
            background: var(--md-sys-color-success);
        }
        
        .notification.error {
            background: var(--md-sys-color-error);
        }
    </style>
</head>
<body>
    <div id="notification" class="notification"></div>
    
    <div class="glass-card">
        <div class="header">
            <h1>🌡️ Wemos D1 Mini</h1>
            <p>Мониторинг температуры и состояния выходов</p>
            <div class="chip-id">ID устройства: <span id="chipId">%CHIP_ID%</span></div>
        </div>
        
        <div class="wifi-info">
            <div class="wifi-item">
                <span class="wifi-label">WiFi сеть:</span>
                <span class="wifi-value" id="wifiSsid">Загрузка...</span>
            </div>
            <div class="wifi-item">
                <span class="wifi-label">Сигнал WiFi:</span>
                <span class="wifi-value">
                    <span id="wifiStrength">Загрузка...</span>
                    <span id="wifiIcon">📶</span>
                </span>
            </div>
            <div class="wifi-item">
                <span class="wifi-label">IP адрес:</span>
                <span class="wifi-value" id="ipAddress">Загрузка...</span>
            </div>
        </div>
        
        <div class="temperature-card">
            <div class="temp-icon">🌡️</div>
            <div class="temp-reading">
                <span id="temperatureValue">--.--</span>
                <span class="temp-unit">°C</span>
            </div>
            <div id="tempStatus" class="status-value">Загрузка...</div>
        </div>
        
        <div class="monitor-cards">
            <div class="monitor-card" id="monitorD5">
                <div class="monitor-icon" id="iconD5">🔴</div>
                <div class="monitor-label">Выход D5</div>
                <div class="monitor-status" id="statusD5">Загрузка...</div>
            </div>
            <div class="monitor-card" id="monitorD6">
                <div class="monitor-icon" id="iconD6">🔴</div>
                <div class="monitor-label">Выход D6</div>
                <div class="monitor-status" id="statusD6">Загрузка...</div>
            </div>
            <div class="monitor-card" id="monitorD7">
                <div class="monitor-icon" id="iconD7">🔴</div>
                <div class="monitor-label">Выход D7</div>
                <div class="monitor-status" id="statusD7">Загрузка...</div>
            </div>
        </div>
        
        <div class="button-group">
            <button class="md-button outlined" id="refreshBtn" onclick="refreshData()">
                <span id="refreshText">Обновить</span>
                <span id="refreshSpinner" style="display: none;">⟳</span>
            </button>
            <button class="md-button primary" onclick="rebootDevice()">Перезагрузить</button>
        </div>
        
        <div class="last-update">
            Последнее обновление: <span id="lastUpdate">--:--:--</span>
        </div>
    </div>

    <script>
        function showNotification(message, type = 'success') {
            const notification = document.getElementById('notification');
            notification.textContent = message;
            notification.className = `notification ${type} show`;
            
            setTimeout(() => {
                notification.classList.remove('show');
            }, 3000);
        }
        
        function updateLastUpdateTime() {
            const now = new Date();
            const timeString = now.toLocaleTimeString();
            document.getElementById('lastUpdate').textContent = timeString;
        }
        
        async function fetchStatus() {
            try {
                const response = await fetch('/status?t=' + new Date().getTime());
                if (!response.ok) {
                    throw new Error(`HTTP error! status: ${response.status}`);
                }
                const data = await response.json();
                return data;
            } catch (error) {
                throw error;
            }
        }
        
        function getTemperatureClass(temp) {
            if (temp === null || temp === undefined) return 'temp-normal';
            if (temp > 30) return 'temp-critical';
            if (temp > 25) return 'temp-warning';
            return 'temp-normal';
        }
        
        function getTemperatureStatus(temp) {
            if (temp === null || temp === undefined) return 'Нет данных';
            if (temp > 30) return 'Критическая температура';
            if (temp > 25) return 'Повышенная температура';
            return 'Нормальная температура';
        }
        
        function getWifiStrengthClass(rssi) {
            if (rssi >= -55) return 'wifi-strength';
            if (rssi >= -70) return 'wifi-weak';
            return 'wifi-poor';
        }
        
        function getWifiIcon(rssi) {
            if (rssi >= -55) return '📶';
            if (rssi >= -70) return '📶';
            if (rssi >= -80) return '📶';
            return '📶';
        }
        
        function updateMonitorCards(monitorData) {
            const pins = ['D5', 'D6', 'D7'];
            
            pins.forEach(pin => {
                const card = document.getElementById(`monitor${pin}`);
                const icon = document.getElementById(`icon${pin}`);
                const status = document.getElementById(`status${pin}`);
                
                if (monitorData[`pin${pin}`]) {
                    card.className = 'monitor-card alarm';
                    icon.textContent = '🔔';
                    status.textContent = 'ТРЕВОГА';
                    status.className = 'monitor-status status-alarm';
                } else {
                    card.className = 'monitor-card active';
                    icon.textContent = '✅';
                    status.textContent = 'НОРМА';
                    status.className = 'monitor-status status-normal';
                }
            });
        }
        
        async function refreshData() {
            const refreshBtn = document.getElementById('refreshBtn');
            const refreshText = document.getElementById('refreshText');
            const refreshSpinner = document.getElementById('refreshSpinner');
            
            refreshBtn.disabled = true;
            refreshText.style.display = 'none';
            refreshSpinner.style.display = 'inline';
            
            try {
                const data = await fetchStatus();
                
                document.getElementById('chipId').textContent = data.chipId;
                document.getElementById('wifiSsid').textContent = data.ssid || 'Неизвестно';
                document.getElementById('ipAddress').textContent = data.ip || 'Не доступен';
                
                const wifiStrength = document.getElementById('wifiStrength');
                const wifiIcon = document.getElementById('wifiIcon');
                if (data.rssi) {
                    wifiStrength.textContent = `${data.rssi} dBm`;
                    wifiStrength.className = getWifiStrengthClass(data.rssi);
                    wifiIcon.textContent = getWifiIcon(data.rssi);
                } else {
                    wifiStrength.textContent = 'Нет данных';
                    wifiIcon.textContent = '📶';
                }
                
                const tempValue = document.getElementById('temperatureValue');
                const tempStatus = document.getElementById('tempStatus');
                
                if (data.temperature !== null && data.temperature !== undefined) {
                    tempValue.textContent = data.temperature.toFixed(1);
                    tempValue.className = `temperature-value ${getTemperatureClass(data.temperature)}`;
                    tempStatus.textContent = getTemperatureStatus(data.temperature);
                    tempStatus.className = `status-value ${getTemperatureClass(data.temperature)}`;
                } else {
                    tempValue.textContent = '--.--';
                    tempValue.className = 'temperature-value temp-normal';
                    tempStatus.textContent = 'Датчик не отвечает';
                    tempStatus.className = 'status-value temp-normal';
                }
                
                if (data.monitorPins) {
                    updateMonitorCards(data.monitorPins);
                }
                
                updateLastUpdateTime();
                showNotification('Данные обновлены', 'success');
                
            } catch (error) {
                showNotification('Ошибка обновления данных', 'error');
            } finally {
                refreshBtn.disabled = false;
                refreshText.style.display = 'inline';
                refreshSpinner.style.display = 'none';
            }
        }
        
        async function rebootDevice() {
            if (confirm('Вы уверены, что хотите перезагрузить устройство?')) {
                try {
                    const response = await fetch('/reboot');
                    if (!response.ok) throw new Error('Network error');
                    
                    showNotification('Устройство перезагружается...', 'success');
                    setTimeout(() => {
                        window.location.reload();
                    }, 5000);
                } catch (error) {
                    showNotification('Ошибка перезагрузки', 'error');
                }
            }
        }
        
        function startAutoRefresh() {
            setInterval(refreshData, 20000);
        }
        
        document.addEventListener('DOMContentLoaded', function() {
            refreshData();
            startAutoRefresh();
        });
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  EEPROM.begin(sizeof(Settings));
  EEPROM.get(0, settings);
  
  sensors.begin();
  
  pinMode(MONITOR_PIN_1, INPUT_PULLUP);
  pinMode(MONITOR_PIN_2, INPUT_PULLUP);
  pinMode(MONITOR_PIN_3, INPUT_PULLUP);
  
  if (!settings.configured) {
    WiFiManager wifiManager;
    if (!wifiManager.startConfigPortal("WemosD1_Config")) {
      delay(3000);
      ESP.restart();
    }
    strncpy(settings.ssid, WiFi.SSID().c_str(), sizeof(settings.ssid));
    strncpy(settings.password, WiFi.psk().c_str(), sizeof(settings.password));
    settings.configured = true;
    EEPROM.put(0, settings);
    EEPROM.commit();
  } else {
    WiFi.begin(settings.ssid, settings.password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
      delay(1000);
      attempts++;
    }
    if (WiFi.status() != WL_CONNECTED) {
      settings.configured = false;
      EEPROM.put(0, settings);
      EEPROM.commit();
      ESP.restart();
    }
  }
  
  setupWebServer();
}

void setupWebServer() {
  server.on("/", HTTP_GET, []() {
    String page = mainPage;
    page.replace("%CHIP_ID%", getChipId());
    server.send(200, "text/html", page);
  });
  
  server.on("/status", HTTP_GET, []() {
    float temperature = readTemperature();
    
    DynamicJsonDocument doc(512);
    
    doc["ip"] = WiFi.localIP().toString();
    doc["ssid"] = WiFi.SSID();
    doc["rssi"] = WiFi.RSSI();
    doc["chipId"] = getChipId();
    
    JsonObject monitorPins = doc.createNestedObject("monitorPins");
    readMonitorPins(monitorPins);
    
    if (isnan(temperature)) {
      doc["temperature"] = nullptr;
    } else {
      doc["temperature"] = temperature;
    }
    
    String response;
    serializeJson(doc, response);
    
    server.send(200, "application/json", response);
  });
  
  server.on("/reboot", HTTP_GET, []() {
    server.send(200, "text/plain", "Rebooting...");
    delay(1000);
    ESP.restart();
  });
  
  server.begin();
}

float readTemperature() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  
  if (tempC == DEVICE_DISCONNECTED_C) {
    return NAN;
  }
  
  return tempC;
}

void readMonitorPins(JsonObject& monitorObj) {
  bool pinD5 = digitalRead(MONITOR_PIN_1) == LOW;
  bool pinD6 = digitalRead(MONITOR_PIN_2) == LOW;
  bool pinD7 = digitalRead(MONITOR_PIN_3) == LOW;
  
  monitorObj["pinD5"] = pinD5;
  monitorObj["pinD6"] = pinD6;
  monitorObj["pinD7"] = pinD7;
}

void loop() {
  server.handleClient();
}
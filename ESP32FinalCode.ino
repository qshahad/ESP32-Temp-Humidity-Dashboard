#include <WiFi.h>        // مكتبة Wi-Fi
#include <DHT.h>         // مكتبة حساس الحرارة والرطوبة

// بيانات الشبكة
#define WIFI_SSID "Tuwaiq's employees"
#define WIFI_PASSWORD "Bootcamp@001"

// حساس DHT11 على GPIO21
#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// أرجل الليدات
const int redLED = 12;
const int greenLED = 14;
const int statusLED = 2;

// سيرفر الويب
WiFiServer server(80);

// وميض ليد أحمر
bool redState = false;
unsigned long lastBlink = 0;

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // وميض حتى يتصل
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(statusLED, !digitalRead(statusLED));
    delay(500);
  }

  digitalWrite(statusLED, HIGH);  // شغل الليد إذا اتصل
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  Serial.println("🔧 Starting ESP32...");
  delay(4000);

  // تهيئة الأرجل
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(statusLED, OUTPUT);

  // بدء الحساس
  dht.begin();

  // الاتصال بالشبكة
  connectToWiFi();

  // بدء السيرفر
  server.begin();
}

void loop() {
  // قراءة البيانات من الحساس
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // تحكم في الليدات حسب الرطوبة
  if (h > 40) {
    // إذا كانت الرطوبة عالية -> شغّل الأحمر واطفئ الأخضر
    digitalWrite(greenLED, LOW);

    if (millis() - lastBlink > 500) {
      redState = !redState;
      digitalWrite(redLED, redState);
      lastBlink = millis();
    }
  } else {
    // إذا كانت الرطوبة أقل من أو تساوي 40 -> شغّل الأخضر واطفئ الأحمر
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);
  }

  // الرد على طلبات المتصفح
  WiFiClient client = server.available();
  if (client) {
    String req = client.readStringUntil('\r');
    client.readStringUntil('\n');

    // JSON API for chart
    if (req.indexOf("GET /data") != -1) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print("{\"temperature\":");
      client.print(t);
      client.print(",\"humidity\":");
      client.print(h);
      client.println("}");
      return;
    }

    // HTML Page
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html\n");
    client.println("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
    client.println("<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>");
    client.println("<style>");
    client.println("body { font-family: Arial; background-color: #f2f2f2; text-align: center; }");
    client.println(".header { background-color: #cceeff; padding: 20px; font-size: 26px; font-weight: bold; }");
    client.println(".logo { position: absolute; right: 20px; height: 40px; top: 20px; }");
    client.println(".charts { display: flex; justify-content: center; margin-top: 30px; gap: 60px; }");
    client.println(".chart-container { width: 250px; height: 250px; position: relative; }");
    client.println(".label { margin-top: 10px; font-weight: bold; font-size: 18px; }");
    client.println(".percentage { position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); font-weight: bold; font-size: 22px; }");
    client.println(".status { margin-top: 30px; padding: 12px 20px; color: white; font-weight: bold; border-radius: 8px; display: inline-block; }");
    client.println(".footer { margin-top: 30px; font-size: 14px; color: gray; }");
    client.println(".links a { margin: 0 15px; color: black; text-decoration: none; font-weight: bold; font-size: 16px; }");
    client.println(".chart-row { margin-top: 40px; width: 80%; margin-left: auto; margin-right: auto; }");
    client.println("</style></head><body>");

    client.println("<div class='header'>ESP32 Temp & Humidity Dashboard<img src='https://cdn.tuwaiq.edu.sa/landing/images/logo/logo-h.png' class='logo'></div>");

    client.println("<div class='charts'>");
    client.println("  <div class='chart-container'>");
    client.println("    <canvas id='tempChart'></canvas>");
    client.println("    <div class='percentage'>" + String(t, 1) + "%</div>");
    client.println("    <div class='label'>Temperature</div>");
    client.println("  </div>");
    client.println("  <div class='chart-container'>");
    client.println("    <canvas id='humChart'></canvas>");
    client.println("    <div class='percentage'>" + String(h, 1) + "%</div>");
    client.println("    <div class='label'>Humidity</div>");
    client.println("  </div>");
    client.println("</div>");

    String statusClass = (h > 40) ? "background-color: red;" : "background-color: green;";
    String statusText = (h > 40) ? "Status: Red LED Blinking" : "Status: Green LED ON";
    client.println("<div class='status' style='" + statusClass + "'>" + statusText + "</div>");

    client.println("<div class='footer'>Designed by Shahad Algadah</div>");
    client.println("<div class='footer links'>");
    client.println("<a href='https://github.com/qshahad' target='_blank'>GitHub</a>");
    client.println("<a href='https://www.linkedin.com/in/shahad-algadah-841509337?utm_source=share&utm_campaign=share_via&utm_content=profile&utm_medium=ios_app' target='_blank'>LinkedIn</a>");
    client.println("</div>");

    client.println("<div class='chart-row'><canvas id='lineChart'></canvas></div>");

    client.println("<script>");
    client.println("const tempValue = " + String(t) + ";");
    client.println("const humValue = " + String(h) + ";");

    client.println("new Chart(document.getElementById('tempChart'), { type: 'doughnut', data: { labels: ['Temp', ''], datasets: [{ data: [tempValue, 100-tempValue], backgroundColor: ['orange', '#eee'], borderWidth: 0 }] }, options: { cutout: '70%', plugins: { tooltip: {enabled: false}, legend: {display: false} } } });");
    client.println("new Chart(document.getElementById('humChart'), { type: 'doughnut', data: { labels: ['Humidity', ''], datasets: [{ data: [humValue, 100-humValue], backgroundColor: ['#007BFF', '#eee'], borderWidth: 0 }] }, options: { cutout: '70%', plugins: { tooltip: {enabled: false}, legend: {display: false} } } });");

    // Real-Time Chart
    client.println("const ctx = document.getElementById('lineChart').getContext('2d');");
    client.println("const lineChart = new Chart(ctx, { type: 'line', data: { labels: [], datasets: [ { label: 'Temperature (°C)', data: [], borderColor: 'orange', fill: false }, { label: 'Humidity (%)', data: [], borderColor: 'blue', fill: false } ] }, options: { responsive: true, animation: false, plugins: { legend: { position: 'bottom' } }, scales: { x: { title: { display: true, text: 'Time' } }, y: { beginAtZero: true } } } });");

    client.println("setInterval(() => { fetch('/data').then(res => res.json()).then(data => { const now = new Date().toLocaleTimeString(); lineChart.data.labels.push(now); lineChart.data.datasets[0].data.push(data.temperature); lineChart.data.datasets[1].data.push(data.humidity); if (lineChart.data.labels.length > 10) { lineChart.data.labels.shift(); lineChart.data.datasets[0].data.shift(); lineChart.data.datasets[1].data.shift(); } lineChart.update(); }); }, 2000);");

    client.println("</script></body></html>");


}



  }

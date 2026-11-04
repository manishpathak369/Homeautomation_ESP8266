/*
  ESP8266 Smart Home Automation System
  Reconstructed from the project's documented requirements.

  Hardware:
    ESP8266 NodeMCU
    PIR motion sensor
    Digital sound sensor module
    1-channel relay module (AC light)

  Assumed NodeMCU pin mapping (verify against your original wiring):
    PIR OUT       -> D5 (GPIO14)
    Sound DO      -> D6 (GPIO12)
    Relay IN      -> D1 (GPIO5)

  Relay logic below assumes an ACTIVE-LOW relay module.
  Disconnect mains power while wiring. Mains voltage is dangerous.
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* AP_SSID = "HomeAutomation";
const char* AP_PASSWORD = "12345678"; // Change before deployment.

const uint8_t PIR_PIN = D5;
const uint8_t SOUND_PIN = D6;
const uint8_t RELAY_PIN = D1;

const bool RELAY_ACTIVE_LOW = true;
const unsigned long SOUND_DEBOUNCE_MS = 350;

ESP8266WebServer server(80);

bool lightOn = false;
bool pirEnabled = true;
bool soundEnabled = true;

int previousPirState = LOW;
int previousSoundState = LOW;
unsigned long lastSoundTrigger = 0;

const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Smart Home Automation</title>
<style>
:root{color-scheme:dark;--bg:#0b1220;--panel:#131f33;--line:#263750;--blue:#4da3ff;--text:#edf4ff;--muted:#9aacc3;--green:#36d399}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font-family:Arial,Helvetica,sans-serif;padding:28px 16px}
main{max-width:760px;margin:auto}
header{margin-bottom:26px}
.eyebrow{color:var(--blue);font-size:12px;letter-spacing:2px;text-transform:uppercase}
h1{font-size:clamp(26px,5vw,38px);margin:9px 0}
.subtitle{color:var(--muted);line-height:1.5}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(230px,1fr));gap:16px}
.card{background:var(--panel);border:1px solid var(--line);border-radius:16px;padding:21px}
.card h2{font-size:17px;margin:0 0 8px}
.card p{color:var(--muted);font-size:13px;line-height:1.5;margin:0 0 18px}
.status{font-size:13px;color:var(--muted);margin:12px 0 0}
.status strong{color:var(--text)}
button{border:0;border-radius:10px;padding:12px 16px;font-weight:700;cursor:pointer;background:var(--blue);color:#061426}
button:active{transform:scale(.98)}
button.secondary{background:#263750;color:var(--text)}
button.off{background:#263750;color:var(--text)}
button:disabled{opacity:.55;cursor:wait}
.row{display:flex;align-items:center;justify-content:space-between;gap:12px}
.light-state{font-size:25px;font-weight:700;margin:8px 0 18px}
.dot{display:inline-block;width:9px;height:9px;border-radius:50%;background:var(--muted);margin-right:7px}
.dot.on{background:var(--green);box-shadow:0 0 12px #36d39988}
footer{color:var(--muted);font-size:12px;text-align:center;margin-top:24px}
</style>
</head>
<body>
<main>
<header>
  <div class="eyebrow">ESP8266 · Local control</div>
  <h1>Smart Home Automation</h1>
  <div class="subtitle">Control your light and manage the onboard motion and sound sensors.</div>
</header>

<section class="grid">
  <article class="card">
    <h2>Light control</h2>
    <div class="light-state" id="lightText">Loading...</div>
    <button id="lightButton" onclick="toggleLight()">Turn light ON</button>
    <div class="status"><span class="dot" id="lightDot"></span><span id="lightStatus">Checking relay</span></div>
  </article>

  <article class="card">
    <h2>PIR motion sensor</h2>
    <p>When enabled, a new motion detection toggles the light.</p>
    <div class="row">
      <span class="status" id="pirStatus">Loading...</span>
      <button class="secondary" id="pirButton" onclick="toggleSensor('pir')">...</button>
    </div>
  </article>

  <article class="card">
    <h2>Sound sensor</h2>
    <p>When enabled, a clap or sound trigger toggles the light.</p>
    <div class="row">
      <span class="status" id="soundStatus">Loading...</span>
      <button class="secondary" id="soundButton" onclick="toggleSensor('sound')">...</button>
    </div>
  </article>
</section>

<footer>Connected directly to the ESP8266 access point · No internet required</footer>
</main>

<script>
let state = {};
async function refreshState(){
  try{
    const response = await fetch('/api/state', {cache:'no-store'});
    state = await response.json();
    document.getElementById('lightText').textContent = state.light ? 'LIGHT ON' : 'LIGHT OFF';
    document.getElementById('lightButton').textContent = state.light ? 'Turn light OFF' : 'Turn light ON';
    document.getElementById('lightButton').className = state.light ? 'off' : '';
    document.getElementById('lightStatus').textContent = state.light ? 'Relay is active' : 'Relay is inactive';
    document.getElementById('lightDot').className = state.light ? 'dot on' : 'dot';

    document.getElementById('pirStatus').textContent = state.pirEnabled ? 'Enabled' : 'Disabled';
    document.getElementById('pirButton').textContent = state.pirEnabled ? 'Disable' : 'Enable';
    document.getElementById('soundStatus').textContent = state.soundEnabled ? 'Enabled' : 'Disabled';
    document.getElementById('soundButton').textContent = state.soundEnabled ? 'Disable' : 'Enable';
  }catch(error){
    document.getElementById('lightStatus').textContent = 'Connection lost';
  }
}
async function toggleLight(){
  if(!state || typeof state.light === 'undefined') return;
  await fetch('/api/light?state=' + (state.light ? 'off' : 'on'), {method:'POST'});
  refreshState();
}
async function toggleSensor(sensor){
  const enabled = sensor === 'pir' ? state.pirEnabled : state.soundEnabled;
  await fetch('/api/' + sensor + '?enabled=' + (enabled ? '0' : '1'), {method:'POST'});
  refreshState();
}
refreshState();
setInterval(refreshState, 1200);
</script>
</body>
</html>
)rawliteral";

void writeRelay() {
  const uint8_t activeLevel = RELAY_ACTIVE_LOW ? LOW : HIGH;
  const uint8_t inactiveLevel = RELAY_ACTIVE_LOW ? HIGH : LOW;
  digitalWrite(RELAY_PIN, lightOn ? activeLevel : inactiveLevel);
}

void setLight(bool on) {
  lightOn = on;
  writeRelay();
}

void sendState() {
  String json = "{";
  json += "\"light\":" + String(lightOn ? "true" : "false") + ",";
  json += "\"pirEnabled\":" + String(pirEnabled ? "true" : "false") + ",";
  json += "\"soundEnabled\":" + String(soundEnabled ? "true" : "false");
  json += "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void handleRoot() {
  server.send_P(200, "text/html", PAGE);
}

void handleLight() {
  if (!server.hasArg("state")) {
    server.send(400, "text/plain", "Missing state");
    return;
  }
  String value = server.arg("state");
  if (value == "on") setLight(true);
  else if (value == "off") setLight(false);
  else {
    server.send(400, "text/plain", "State must be on or off");
    return;
  }
  sendState();
}

void handlePir() {
  if (!server.hasArg("enabled")) {
    server.send(400, "text/plain", "Missing enabled");
    return;
  }
  pirEnabled = (server.arg("enabled") == "1");
  previousPirState = digitalRead(PIR_PIN); // Avoid a false edge after re-enabling.
  sendState();
}

void handleSound() {
  if (!server.hasArg("enabled")) {
    server.send(400, "text/plain", "Missing enabled");
    return;
  }
  soundEnabled = (server.arg("enabled") == "1");
  previousSoundState = digitalRead(SOUND_PIN);
  sendState();
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void readSensors() {
  const int pirState = digitalRead(PIR_PIN);
  if (pirEnabled && pirState == HIGH && previousPirState == LOW) {
    setLight(!lightOn);
  }
  previousPirState = pirState;

  const int soundState = digitalRead(SOUND_PIN);
  const unsigned long now = millis();
  if (soundEnabled && soundState == HIGH && previousSoundState == LOW &&
      (now - lastSoundTrigger >= SOUND_DEBOUNCE_MS)) {
    setLight(!lightOn);
    lastSoundTrigger = now;
  }
  previousSoundState = soundState;
}

void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);
  pinMode(SOUND_PIN, INPUT);

  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
  pinMode(RELAY_PIN, OUTPUT);
  setLight(false);

  previousPirState = digitalRead(PIR_PIN);
  previousSoundState = digitalRead(SOUND_PIN);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  Serial.println();
  Serial.println("Smart Home Automation started");
  Serial.print("Connect to Wi-Fi: ");
  Serial.println(AP_SSID);
  Serial.print("Open: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/light", HTTP_POST, handleLight);
  server.on("/api/pir", HTTP_POST, handlePir);
  server.on("/api/sound", HTTP_POST, handleSound);
  server.onNotFound(handleNotFound);
  server.begin();
}

void loop() {
  server.handleClient();
  readSensors();
}

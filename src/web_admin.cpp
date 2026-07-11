#include "web_admin.h"

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#include "beacon_config.h"

// External references to main.cpp globals and functions.
extern BeaconConfig config;
extern void saveConfig();
extern void loadDefaultConfig();
extern bool forceTransmit;
extern void testPttOnly();
extern float readBatteryVoltage();
extern uint8_t foxNumberFromId(const String &id);
extern uint32_t resolvedStartupDelaySeconds();
extern int beaconStateValue();

static WebServer server(80);
static DNSServer dnsServer;
static bool running = false;
static String apSsid;
static String apIp = "192.168.4.1";

static uint32_t lastActivityMs = 0;   // last time a client was connected or a request was handled
static uint8_t prevStations = 0;      // track station connect/disconnect transitions

static const char *HTML_PAGE = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Fox Beacon Admin</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,sans-serif;background:#1a1a2e;color:#eee;padding:12px;max-width:600px;margin:auto}
h1{font-size:1.4em;color:#0ff;margin-bottom:8px}
h2{font-size:1.1em;color:#0f0;margin:16px 0 8px;border-bottom:1px solid #333;padding-bottom:4px}
.status{background:#16213e;padding:10px;border-radius:8px;margin-bottom:12px;font-size:.9em}
.status span{color:#0ff}
.field{display:flex;justify-content:space-between;align-items:center;padding:6px 0;gap:8px}
.field label{flex:0 0 45%;font-size:.85em;color:#aaa}
.field input,.field select{flex:1;padding:6px;background:#0f0f23;border:1px solid #444;border-radius:4px;color:#eee;font-size:.85em}
.field input:focus,.field select:focus{border-color:#0ff}
.btns{display:flex;gap:8px;flex-wrap:wrap;margin-top:16px}
button{padding:10px 18px;border:none;border-radius:6px;font-size:.9em;cursor:pointer}
.btn-save{background:#0a7;color:#fff}
.btn-test{background:#c80;color:#000}
.btn-ptt{background:#c40;color:#fff}
.btn-defaults{background:#666;color:#fff}
.btn-reboot{background:#c00;color:#fff}
.msg{padding:8px;margin:8px 0;border-radius:4px;display:none}
.msg-ok{background:#0a52}
.msg-err{background:#a30}
a{color:#0ff}
</style>
</head>
<body>
<h1>&#129418; Fox Beacon Admin</h1>
<div class="status" id="status">Loading...</div>
<div id="msg" class="msg"></div>

<h2>Identification</h2>
<div class="field"><label>Callsign</label><input id="call" type="text" maxlength="12"></div>
<div class="field"><label>Fox ID</label><input id="fox" type="text" maxlength="8"></div>
<div class="field"><label>Mode</label><select id="mode"><option value="fox">Fox (scheduled)</option><option value="beacon">Beacon (continuous)</option></select></div>
<div class="field"><label>Fox sync</label><select id="fox_sync"><option value="on">On</option><option value="off">Off</option></select></div>

<h2>Timing (seconds)</h2>
<div class="field"><label>Startup delay</label><input id="startup" type="number" min="0" max="3600"></div>
<div class="field"><label>Transmit time</label><input id="tx" type="number" min="3" max="600"></div>
<div class="field"><label>Idle time</label><input id="idle" type="number" min="5" max="3600"></div>
<div class="field"><label>Beacon ID interval</label><input id="beacon_id" type="number" min="10" max="600"></div>

<h2>CW / Audio</h2>
<div class="field"><label>CW speed (WPM)</label><input id="wpm" type="number" min="5" max="35"></div>
<div class="field"><label>CW tone (Hz)</label><input id="tone" type="number" min="300" max="1800"></div>
<div class="field"><label>Warble</label><select id="warble"><option value="on">On</option><option value="off">Off</option></select></div>
<div class="field"><label>Warble low (Hz)</label><input id="warble_low" type="number" min="300" max="1800"></div>
<div class="field"><label>Warble high (Hz)</label><input id="warble_high" type="number" min="300" max="1800"></div>
<div class="field"><label>Warble step (ms)</label><input id="warble_step" type="number" min="100" max="3000"></div>

<h2>PTT</h2>
<div class="field"><label>PTT polarity</label><select id="ptt"><option value="active_high">Active HIGH</option><option value="active_low">Active LOW</option></select></div>
<div class="field"><label>PTT lead (ms)</label><input id="lead" type="number" min="50" max="3000"></div>
<div class="field"><label>PTT tail (ms)</label><input id="tail" type="number" min="50" max="3000"></div>

<h2>Battery</h2>
<div class="field"><label>Battery monitor</label><select id="battery"><option value="on">On</option><option value="off">Off</option></select></div>
<div class="field"><label>Battery scale</label><input id="battery_scale" type="number" step="0.01" min="1.0" max="10.0"></div>
<div class="field"><label>Low battery (V)</label><input id="low_battery" type="number" step="0.01" min="2.5" max="15.0"></div>

<h2>Display &amp; WiFi</h2>
<div class="field"><label>WiFi AP / Web Admin</label><select id="wifi_ap"><option value="on">On</option><option value="off">Off</option></select></div>
<div class="field"><label>AP auto-off (min)</label><input id="wifi_ap_timeout" type="number" min="0" max="1440"><small style="color:#888;font-size:.7em">0=never</small></div>
<div class="field"><label>Display eco mode</label><select id="eco_mode"><option value="on">On</option><option value="off">Off</option></select></div>

<div class="btns">
<button class="btn-save" onclick="save()">Save</button>
<button class="btn-test" onclick="cmd('test')">Test TX</button>
<button class="btn-ptt" onclick="cmd('ptt_test')">PTT Test</button>
<button class="btn-defaults" onclick="cmd('defaults')">Defaults</button>
<button class="btn-reboot" onclick="cmd('reboot')">Reboot</button>
</div>

<script>
function val(id){return document.getElementById(id).value}
function set(id,v){document.getElementById(id).value=v}
function setSel(id,v){document.getElementById(id).value=v}
function msg(t,ok){var e=document.getElementById('msg');e.textContent=t;e.className='msg '+(ok?'msg-ok':'msg-err');e.style.display='block';setTimeout(function(){e.style.display='none'},4000)}

function loadConfig(){
  fetch('/api/config').then(r=>r.json()).then(c=>{
    set('call',c.call);set('fox',c.fox);
    setSel('mode',c.beaconMode?'beacon':'fox');
    setSel('fox_sync',c.foxSync?'on':'off');
    set('startup',c.startup);set('tx',c.tx);set('idle',c.idle);set('beacon_id',c.beaconId);
    set('wpm',c.wpm);set('tone',c.tone);
    setSel('warble',c.warble?'on':'off');
    set('warble_low',c.warbleLow);set('warble_high',c.warbleHigh);set('warble_step',c.warbleStep);
    setSel('ptt',c.pttActiveLow?'active_low':'active_high');
    set('lead',c.lead);set('tail',c.tail);
    setSel('battery',c.batteryEnabled?'on':'off');
    set('battery_scale',c.batteryScale);set('low_battery',c.lowBattery);
    setSel('wifi_ap',c.wifiAp?'on':'off');
    set('wifi_ap_timeout',c.wifiApTimeout);
    setSel('eco_mode',c.ecoMode?'on':'off');
    var s='<b>State:</b> <span>'+c.state+'</span> | <b>Battery:</b> <span>'+c.battery+' V</span>';
    if(c.foxSync&&!c.beaconMode&&c.foxNum>0)s+=' | <b>Slot:</b> <span>'+c.foxNum+'</span> (start '+c.startupResolved+'s)';
    s+='<br><b>AP:</b> <span>'+c.apSsid+'</span> | <b>IP:</b> <span>'+c.ip+'</span>';
    document.getElementById('status').innerHTML=s;
  }).catch(e=>msg('Load failed: '+e,false));
}

function save(){
  var d=new URLSearchParams();
  d.append('call',val('call'));d.append('fox',val('fox'));
  d.append('mode',val('mode'));d.append('fox_sync',val('fox_sync'));
  d.append('startup',val('startup'));d.append('tx',val('tx'));d.append('idle',val('idle'));
  d.append('beacon_id',val('beacon_id'));
  d.append('wpm',val('wpm'));d.append('tone',val('tone'));d.append('warble',val('warble'));
  d.append('warble_low',val('warble_low'));d.append('warble_high',val('warble_high'));d.append('warble_step',val('warble_step'));
  d.append('ptt',val('ptt'));d.append('lead',val('lead'));d.append('tail',val('tail'));
  d.append('battery',val('battery'));d.append('battery_scale',val('battery_scale'));d.append('low_battery',val('low_battery'));
  d.append('wifi_ap',val('wifi_ap'));d.append('wifi_ap_timeout',val('wifi_ap_timeout'));d.append('eco_mode',val('eco_mode'));
  fetch('/api/config',{method:'POST',body:d}).then(r=>r.text()).then(t=>{msg(t,true);setTimeout(loadConfig,500)}).catch(e=>msg('Save failed: '+e,false));
}

function cmd(c){
  fetch('/api/'+c,{method:'POST'}).then(r=>r.text()).then(t=>msg(t,true)).catch(e=>msg('Failed: '+e,false));
}

loadConfig();
</script>
</body>
</html>
)HTML";

static String stateName(int s) {
  switch (s) {
    case 0: return "StartupDelay";
    case 1: return "Idle";
    case 2: return "Transmitting";
    case 3: return "ContinuousTransmit";
    case 4: return "LowBatteryHalt";
    default: return "Unknown";
  }
}

static void markActivity() {
  lastActivityMs = millis();
}

static void handleRoot() {
  markActivity();
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", HTML_PAGE);
}

static void handleGetConfig() {
  markActivity();
  String json = "{";
  json += "\"call\":\"" + config.callSign + "\",";
  json += "\"fox\":\"" + config.foxId + "\",";
  json += "\"beaconMode\":" + String(config.beaconMode ? "true" : "false") + ",";
  json += "\"foxSync\":" + String(config.foxSyncEnabled ? "true" : "false") + ",";
  json += "\"foxNum\":" + String(foxNumberFromId(config.foxId)) + ",";
  json += "\"startup\":" + String(config.startupDelaySeconds) + ",";
  json += "\"startupResolved\":" + String(resolvedStartupDelaySeconds()) + ",";
  json += "\"tx\":" + String(config.transmitSeconds) + ",";
  json += "\"idle\":" + String(config.idleSeconds) + ",";
  json += "\"beaconId\":" + String(config.beaconIdIntervalSeconds) + ",";
  json += "\"wpm\":" + String(config.cwWpm) + ",";
  json += "\"tone\":" + String(config.cwToneHz) + ",";
  json += "\"warble\":" + String(config.warbleEnabled ? "true" : "false") + ",";
  json += "\"warbleLow\":" + String(config.warbleLowHz) + ",";
  json += "\"warbleHigh\":" + String(config.warbleHighHz) + ",";
  json += "\"warbleStep\":" + String(config.warbleStepMs) + ",";
  json += "\"pttActiveLow\":" + String(config.pttActiveLow ? "true" : "false") + ",";
  json += "\"lead\":" + String(config.pttLeadMs) + ",";
  json += "\"tail\":" + String(config.pttTailMs) + ",";
  json += "\"batteryEnabled\":" + String(config.batteryEnabled ? "true" : "false") + ",";
  json += "\"batteryScale\":" + String(config.batteryScale, 3) + ",";
  json += "\"lowBattery\":" + String(config.lowBatteryVoltage, 2) + ",";
  json += "\"wifiAp\":" + String(config.wifiApEnabled ? "true" : "false") + ",";
  json += "\"wifiApTimeout\":" + String(config.wifiApTimeoutMinutes) + ",";
  json += "\"ecoMode\":" + String(config.displayEcoMode ? "true" : "false") + ",";
  json += "\"state\":\"" + stateName(beaconStateValue()) + "\",";
  if (config.batteryEnabled) {
    json += "\"battery\":\"" + String(readBatteryVoltage(), 2) + "\",";
  } else {
    json += "\"battery\":\"N/A\",";
  }
  json += "\"apSsid\":\"" + apSsid + "\",";
  json += "\"ip\":\"" + apIp + "\"";
  json += "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

static void handlePostConfig() {
  markActivity();
  if (!server.hasArg("call")) {
    server.send(400, "text/plain", "Missing form data");
    return;
  }

  config.callSign = server.arg("call");
  config.foxId = server.arg("fox");

  String mode = server.arg("mode");
  config.beaconMode = (mode == "beacon" || mode == "continuous");

  String foxSync = server.arg("fox_sync");
  config.foxSyncEnabled = (foxSync == "on" || foxSync == "true" || foxSync == "1");

  config.startupDelaySeconds = constrain(server.arg("startup").toInt(), 0UL, 3600UL);
  config.transmitSeconds = constrain(server.arg("tx").toInt(), 3UL, 600UL);
  config.idleSeconds = constrain(server.arg("idle").toInt(), 5UL, 3600UL);
  config.beaconIdIntervalSeconds = constrain(server.arg("beacon_id").toInt(), 10, 600);

  config.cwWpm = constrain(server.arg("wpm").toInt(), 5, 35);
  config.cwToneHz = constrain(server.arg("tone").toInt(), 300, 1800);

  String warble = server.arg("warble");
  config.warbleEnabled = (warble == "on" || warble == "true" || warble == "1");
  config.warbleLowHz = constrain(server.arg("warble_low").toInt(), 300, 1800);
  config.warbleHighHz = constrain(server.arg("warble_high").toInt(), 300, 1800);
  config.warbleStepMs = constrain(server.arg("warble_step").toInt(), 100, 3000);

  String ptt = server.arg("ptt");
  config.pttActiveLow = (ptt == "active_low");
  config.pttLeadMs = constrain(server.arg("lead").toInt(), 50, 3000);
  config.pttTailMs = constrain(server.arg("tail").toInt(), 50, 3000);

  String battery = server.arg("battery");
  config.batteryEnabled = (battery == "on" || battery == "true" || battery == "1");
  config.batteryScale = constrain(server.arg("battery_scale").toFloat(), 1.0f, 10.0f);
  config.lowBatteryVoltage = constrain(server.arg("low_battery").toFloat(), 2.5f, 15.0f);

  String wifiAp = server.arg("wifi_ap");
  config.wifiApEnabled = (wifiAp == "on" || wifiAp == "true" || wifiAp == "1");
  config.wifiApTimeoutMinutes = constrain(server.arg("wifi_ap_timeout").toInt(), 0, 1440);

  String ecoMode = server.arg("eco_mode");
  config.displayEcoMode = (ecoMode == "on" || ecoMode == "true" || ecoMode == "1");

  saveConfig();
  server.send(200, "text/plain", "Saved. Settings will apply on next cycle or reboot.");
}

static void handleTest() {
  markActivity();
  forceTransmit = true;
  server.send(200, "text/plain", "Test transmission queued.");
}

static void handlePttTest() {
  markActivity();
  testPttOnly();
  server.send(200, "text/plain", "PTT test completed.");
}

static void handleDefaults() {
  markActivity();
  loadDefaultConfig();
  saveConfig();
  server.send(200, "text/plain", "Defaults restored and saved. Reboot recommended.");
}

static void handleReboot() {
  markActivity();
  server.send(200, "text/plain", "Rebooting...");
  delay(200);
  ESP.restart();
}

static void handleNotFound() {
  markActivity();
  // Captive portal: redirect all unknown URLs to root.
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}

void webAdminInit(const char *apNamePrefix) {
  // Build SSID from prefix + last 4 hex of MAC.
  uint64_t mac = ESP.getEfuseMac();
  char macSuffix[5];
  snprintf(macSuffix, sizeof(macSuffix), "%02X%02X",
           (uint8_t)(mac >> 8), (uint8_t)mac);
  apSsid = String(apNamePrefix) + "-" + macSuffix;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid.c_str());
  apIp = WiFi.softAPIP().toString();

  // Captive portal DNS: redirect all queries to our IP.
  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handlePostConfig);
  server.on("/api/test", HTTP_POST, handleTest);
  server.on("/api/ptt_test", HTTP_POST, handlePttTest);
  server.on("/api/defaults", HTTP_POST, handleDefaults);
  server.on("/api/reboot", HTTP_POST, handleReboot);
  server.onNotFound(handleNotFound);
  server.begin();

  running = true;
  lastActivityMs = millis();
  prevStations = 0;
  Serial.printf("Web admin AP: %s, IP: %s\n", apSsid.c_str(), apIp.c_str());
}

void webAdminLoop() {
  if (!running) return;

  // Track station connections as activity.
  uint8_t stations = WiFi.softAPgetStationNum();
  if (stations > 0) {
    markActivity();
  }
  prevStations = stations;

  dnsServer.processNextRequest();
  server.handleClient();

  // Auto-off AP after timeout with no activity.
  if (config.wifiApTimeoutMinutes > 0) {
    uint32_t timeoutMs = (uint32_t)config.wifiApTimeoutMinutes * 60UL * 1000UL;
    if (millis() - lastActivityMs >= timeoutMs) {
      Serial.printf("WiFi AP auto-off after %u minutes of inactivity.\n",
                    config.wifiApTimeoutMinutes);
      config.wifiApEnabled = false;
      webAdminStop();
    }
  }
}

void webAdminStop() {
  if (!running) return;
  server.stop();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  running = false;
  Serial.println(F("Web admin AP stopped."));
}

String webAdminGetIp() {
  return apIp;
}

String webAdminGetSsid() {
  return apSsid;
}

bool webAdminIsRunning() {
  return running;
}

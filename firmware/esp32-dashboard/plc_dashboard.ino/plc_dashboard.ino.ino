#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// ============================================================
// WiFi credentials (Your Hotspot)
// ============================================================
const char* ssid     = "DESKTOP-KMKR958 2310";
const char* password = "2428eG7/";

WebServer server(80);

// PLC state variables
int plc_inputs[8]  = {0};
int plc_outputs[4] = {0};
unsigned long plc_scan = 0;
unsigned long last_update = 0;
bool plc_connected = false;

// ============================================================
// ORIGINAL INDUSTRIAL SCADA / HMI DASHBOARD AESTHETIC
// ============================================================
const char* html_page = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
<title>STM32 HMI Panel</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  :root {
    --bg-base: #121215;
    --panel-bg: #1c1c21;
    --panel-border-top: #3a3a45;
    --panel-border-bot: #0a0a0d;
    --text-main: #d4d4d8;
    --text-label: #8b8b99;
    --led-on-color: #00ff66;
    --led-off-color: #27272f;
    --alarm-color: #ff3333;
    --font-mono: ui-monospace, 'Cascadia Code', 'Source Code Pro', Menlo, Consolas, 'Courier New', monospace;
  }
  
  body { 
    font-family: 'Segoe UI', system-ui, sans-serif; 
    background-color: var(--bg-base);
    background-image: radial-gradient(circle at center, #1b1b22 0%, #0d0d10 100%);
    color: var(--text-main); 
    text-align: center; 
    padding: 20px 10px; 
    margin: 0;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
  }

  /* Industrial Top Header */
  .header {
    width: 100%;
    max-width: 650px;
    background: linear-gradient(180deg, #2a2a35 0%, #1e1e24 100%);
    border-top: 2px solid #454555;
    border-bottom: 2px solid #000;
    padding: 15px 20px;
    margin-bottom: 25px;
    box-sizing: border-box;
    display: flex;
    justify-content: space-between;
    align-items: center;
    box-shadow: 0 5px 15px rgba(0,0,0,0.5);
  }
  
  .header h1 { 
    margin: 0; 
    font-size: 1.2rem;
    font-weight: 700;
    letter-spacing: 2px;
    text-transform: uppercase;
    color: #fff;
    text-shadow: 0px 2px 4px rgba(0,0,0,0.8);
  }

  .model-tag {
    font-family: var(--font-mono);
    background: #000;
    color: #aaa;
    padding: 4px 8px;
    font-size: 0.8rem;
    border: 1px inset #444;
  }
  
  .container {
    width: 100%;
    max-width: 650px;
    display: flex;
    flex-direction: column;
    gap: 20px;
  }
  
  /* Rugged Hardware Panels */
  .panel { 
    background: var(--panel-bg); 
    border-radius: 4px; 
    padding: 25px 20px; 
    border-top: 2px solid var(--panel-border-top);
    border-left: 2px solid var(--panel-border-top);
    border-bottom: 2px solid var(--panel-border-bot);
    border-right: 2px solid var(--panel-border-bot);
    box-shadow: 0 8px 20px rgba(0,0,0,0.6);
    position: relative;
  }
  
  .panel-title {
    position: absolute;
    top: -10px;
    left: 20px;
    background: var(--bg-base);
    padding: 0 10px;
    color: var(--text-label);
    font-size: 0.75rem;
    font-weight: 700;
    letter-spacing: 1px;
    text-transform: uppercase;
    border: 1px solid #333;
  }
  
  .row { 
    display: flex; 
    justify-content: space-evenly; 
    gap: 10px; 
    flex-wrap: wrap; 
  }
  
  /* Physical Panel-Mount LED Style */
  .led-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 8px;
  }

  .led-label {
    font-family: var(--font-mono);
    font-size: 0.8rem;
    color: #888;
  }

  .led { 
    width: 44px; 
    height: 44px; 
    border-radius: 50%; 
    border: 4px solid #111; 
    box-shadow: 0 2px 4px rgba(255,255,255,0.1), inset 0 3px 6px rgba(0,0,0,0.8);
    transition: all 0.1s; 
  }
  
  .led-on { 
    background: radial-gradient(circle at 35% 35%, #8affb1, var(--led-on-color)); 
    box-shadow: 0 2px 4px rgba(255,255,255,0.1), inset 0 2px 4px rgba(255,255,255,0.6), 0 0 15px var(--led-on-color); 
  }
  
  .led-off { 
    background: radial-gradient(circle at 35% 35%, #4a4a55, var(--led-off-color)); 
  }
  
  /* Footer Stats & Readouts */
  .footer-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 15px;
    margin-top: 10px;
  }

  .readout-box {
    background: #000;
    border: 2px inset #444;
    padding: 10px;
    display: flex;
    flex-direction: column;
    align-items: flex-start;
  }

  .readout-label {
    color: #666;
    font-size: 0.65rem;
    text-transform: uppercase;
    margin-bottom: 4px;
  }

  .readout-value {
    font-family: var(--font-mono);
    font-size: 1.1rem;
    color: #fff;
  }

  .scan-val { color: #00d4ff; }
  
  .status-indicator {
    display: flex;
    align-items: center;
    gap: 10px;
    font-family: var(--font-mono);
    font-size: 0.9rem;
  }

  .status-ok { color: var(--led-on-color); }
  .status-err { color: var(--alarm-color); }

  .status-light {
    width: 12px;
    height: 12px;
    border-radius: 50%;
    border: 1px solid #000;
  }
  .light-ok { background: var(--led-on-color); box-shadow: 0 0 8px var(--led-on-color); }
  .light-err { background: var(--alarm-color); box-shadow: 0 0 8px var(--alarm-color); animation: blink 1s infinite; }

  @keyframes blink { 50% { opacity: 0.3; } }
</style>
</head>
<body>

  <div class="header">
    <h1>HMI Dashboard</h1>
    <div class="model-tag">STM32-MINI-PLC</div>
  </div>

  <div class="container">
    <div class="panel">
      <div class="panel-title">Digital Inputs (I0 - I7)</div>
      <div class="row" id="inputs"></div>
    </div>

    <div class="panel">
      <div class="panel-title">Relay Outputs (Q0 - Q3)</div>
      <div class="row" id="outputs"></div>
    </div>

    <div class="footer-grid">
      <div class="readout-box">
        <span class="readout-label">System Scan Cycle</span>
        <span class="readout-value scan-val" id="scan">000000</span>
      </div>
      
      <div class="readout-box" style="align-items: flex-end; justify-content: center;">
        <span class="readout-label">Comm Status</span>
        <div class="status-indicator status-err" id="status-container">
          <div class="status-light light-err" id="status-light"></div>
          <span id="status-text">OFFLINE</span>
        </div>
      </div>
    </div>
  </div>

<script>
function padZero(num, size) {
    let s = num + "";
    while (s.length < size) s = "0" + s;
    return s;
}

function update() {
  fetch('/state').then(r => r.json()).then(d => {
    let iH = '';
    for (let i = 0; i < 8; i++) {
      iH += '<div class="led-container"><div class="led ' + (d.I[i] ? 'led-on' : 'led-off') + '"></div><span class="led-label">I' + i + '</span></div>';
    }
    document.getElementById('inputs').innerHTML = iH;
    
    let qH = '';
    for (let i = 0; i < 4; i++) {
      qH += '<div class="led-container"><div class="led ' + (d.Q[i] ? 'led-on' : 'led-off') + '"></div><span class="led-label">Q' + i + '</span></div>';
    }
    document.getElementById('outputs').innerHTML = qH;
    
    document.getElementById('scan').textContent = padZero(d.S, 6);
    
    let sCont = document.getElementById('status-container');
    let sText = document.getElementById('status-text');
    let sLight = document.getElementById('status-light');
    
    if (d.live) {
      sText.textContent = 'ONLINE RUN';
      sCont.className = 'status-indicator status-ok';
      sLight.className = 'status-light light-ok';
    } else {
      sText.textContent = 'LINK FAULT';
      sCont.className = 'status-indicator status-err';
      sLight.className = 'status-light light-err';
    }
  }).catch(() => {
    document.getElementById('status-text').textContent = 'SYS FAULT';
    document.getElementById('status-container').className = 'status-indicator status-err';
    document.getElementById('status-light').className = 'status-light light-err';
  });
}
setInterval(update, 500);
update();
</script>
</body>
</html>
)rawhtml";

// ============================================================
// Send Data to Browser
// ============================================================
void handle_state() {
  String json = "{\"I\":[";
  for (int i=0; i<8; i++) json += String(plc_inputs[i]) + (i<7?",":"");
  json += "],\"Q\":[";
  for (int i=0; i<4; i++) json += String(plc_outputs[i]) + (i<3?",":"");
  json += "],\"S\":" + String(plc_scan) + ",\"live\":" + (plc_connected?"true":"false") + "}";
  server.send(200, "application/json", json);
}

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17); 
  
  // CRITICAL PERFORMANCE FIX: Prevents Serial2 from freezing the web server
  Serial2.setTimeout(10); 

  Serial.println("\n==================================");
  Serial.println("  Starting ESP32 HMI (Hotspot Mode)");
  Serial.println("==================================");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to hotspot");
  
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 30) {
    delay(500); 
    Serial.print("."); 
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n\n--- SUCCESS! ---");
    Serial.print("Your phone assigned this IP: http://"); 
    Serial.println(WiFi.localIP());
    
    if (MDNS.begin("plc")) {
      Serial.println("You can also access it at:   http://plc.local");
    }
  } else {
    Serial.println("\nFailed to connect. Check hotspot password!");
  }

  server.on("/", []() { server.send(200, "text/html", html_page); });
  server.on("/state", handle_state);
  server.begin();
}

// ============================================================
// Main Loop
// ============================================================
void loop() {
  server.handleClient(); // Handles web requests

  // Read data from STM32 smoothly
  if (Serial2.available() > 0) {
    String line = Serial2.readStringUntil('\n');
    line.trim();
    if (line.indexOf("JSON:") != -1) {
      int i_start = line.indexOf("\"I\":[") + 5;
      for (int i=0; i<8; i++) plc_inputs[i] = (line.charAt(i_start + (i*2)) == '1');
      
      int q_start = line.indexOf("\"Q\":[") + 5;
      for (int i=0; i<4; i++) plc_outputs[i] = (line.charAt(q_start + (i*2)) == '1');
      
      int s_start = line.indexOf("\"S\":") + 4;
      plc_scan = line.substring(s_start).toInt();
      
      last_update = millis();
      plc_connected = true;
    }
  }
  
  // Timeout if STM32 stops talking for 3 seconds
  if (millis() - last_update > 3000) {
    plc_connected = false;
  }
}
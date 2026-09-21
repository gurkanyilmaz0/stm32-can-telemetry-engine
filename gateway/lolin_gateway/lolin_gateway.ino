#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "STM32_Telemetri_Station";
const char* password = "12345678password";

ESP8266WebServer server(80);

String lastTelemetry = "RX-> C:0 | T:0 | MCU_T:0 | P:0 | Lat:0.000000 | Lon:0.000000 | Spd:0.0 | Cog:0.0 | Time:000000 | Hdop:9.9 | Alt:0.0 | Mode:NO | Sat:0";

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>STM32 Telemetri - Ultimate Taktiksel GCS</title>
  <style>
    body { background-color: #0b0f19; color: #00ffcc; font-family: 'Courier New', Courier, monospace; margin: 0; padding: 15px; }
    .header { display: flex; justify-content: space-between; align-items: center; border-bottom: 2px solid #1f293d; padding-bottom: 10px; margin-bottom: 15px; }
    .header h2 { margin: 0; color: #ffffff; letter-spacing: 2px; font-size: 18px; }
    .status-box { background: #161b22; border: 1px solid #30363d; padding: 5px 15px; border-radius: 4px; color: #00ffcc; font-weight: bold; }
    
    .grid-container { display: grid; grid-template-columns: repeat(5, 1fr); gap: 12px; margin-bottom: 15px; }
    .panel { background: #111622; border: 1px solid #1f293d; border-radius: 6px; padding: 12px; text-align: center; box-shadow: 0 4px 10px rgba(0,0,0,0.6); display: flex; flex-direction: column; justify-content: center; align-items: center;}
    .panel h4 { margin: 0 0 8px 0; color: #8b949e; font-size: 11px; text-transform: uppercase; letter-spacing: 1px;}
    
    .bar-container { width: 25px; height: 90px; background: #1a2233; border: 1px solid #30363d; margin: 0 auto 8px auto; position: relative; border-radius: 4px; display: flex; align-items: flex-end; }
    .bar-fill { width: 100%; background: #00ffcc; height: 0%; border-radius: 2px; transition: height 0.3s ease; }
    .val-text { font-size: 18px; font-weight: bold; color: #00ffcc; margin-top: 4px;}

    .gauge-panel { background: #111622; border: 1px solid #1f293d; border-radius: 6px; padding: 12px; text-align: center; box-shadow: 0 4px 10px rgba(0,0,0,0.6); }
    .gauge-val { font-size: 32px; font-weight: bold; color: #3fb950; margin: 4px 0; }
    .metrics-box { background: #161b22; border: 1px solid #30363d; padding: 8px; border-radius: 4px; margin-top: 8px; text-align: left; font-size: 12px; width: 92%; }

    .alarm-banner { background: #f85149; color: #fff; padding: 8px; border-radius: 4px; text-align: center; font-weight: bold; margin-bottom: 15px; display: none; animation: blink 1s infinite; }
    @keyframes blink { 50% { opacity: 0.4; } }

    .btn { background: #238636; color: #fff; padding: 8px 12px; border-radius: 4px; text-decoration: none; font-size: 12px; font-weight: bold; border: none; cursor: pointer; transition: background 0.3s; }
    .btn:hover { background: #2ea043; }
  </style>
  <script>
    let lastFetchedLat = 0;
    let lastFetchedLon = 0;
    let startTime = Date.now();
    let expectedCounter = null;
    let totalPackets = 0;
    let lostPackets = 0;

    let peakMaxSpeed = 0;
    let peakMaxTemp = 0;
    let peakMaxMcuTemp = 0;
    let peakMaxAlt = 0;
    
    let csvLogData = "Zaman,Sayac,Sicaklik,MCU_Sicaklik,Gaz,Enlem,Boylam,Hiz_Kmh,Irtifa,HDOP\n";

    function updateData() {
      // Kronometre Guncellemesi
      let elapsedSec = Math.floor((Date.now() - startTime) / 1000);
      let mins = Math.floor(elapsedSec / 60).toString().padStart(2, '0');
      let secs = (elapsedSec % 60).toString().padStart(2, '0');
      document.getElementById('mission-timer').innerText = `Uptime: ${mins}:${secs}`;

      fetch('/data')
        .then(response => response.text())
        .then(raw => {
          const matchC = raw.match(/C:(\d+)/);
          const matchT = raw.match(/T:(\d+)/);
          const matchMcuT = raw.match(/MCU_T:(\d+)/);
          const matchP = raw.match(/P:(\d+)/);
          const matchLat = raw.match(/Lat:([\-\d\.]+)/);
          const matchLon = raw.match(/Lon:([\-\d\.]+)/);
          const matchSpd = raw.match(/Spd:([\-\d\.]+)/);
          const matchCog = raw.match(/Cog:([\-\d\.]+)/);
          const matchTime = raw.match(/Time:([^\s\|]+)/);
          const matchHdop = raw.match(/Hdop:([\-\d\.]+)/);
          const matchAlt = raw.match(/Alt:([\-\d\.]+)/);
          const matchMode = raw.match(/Mode:([^\s\|]+)/);
          const matchSat = raw.match(/Sat:(\d+)/);

          if (matchC && matchT && matchP) {
            let c = parseInt(matchC[1]);
            let t = parseInt(matchT[1]);
            let mcuT = matchMcuT ? parseInt(matchMcuT[1]) : 0;
            let p = parseInt(matchP[1]);

            totalPackets++;
            if (expectedCounter !== null) {
              let diff = c - expectedCounter;
              if (diff > 0) { lostPackets += diff; }
              else if (diff < 0) { lostPackets += (256 + diff); } // 8-bit sayac devri
            }
            expectedCounter = (c + 1) % 256;

            let lossPercent = (totalPackets + lostPackets) > 0 ? ((lostPackets / (totalPackets + lostPackets)) * 100).toFixed(1) : 0;
            let linkQuality = (100 - lossPercent).toFixed(1);
            document.getElementById('link-quality').innerText = `Link: %${linkQuality}`;

            document.getElementById('counter').innerText = c;
            document.getElementById('temp-val').innerText = t + " °C";
            document.getElementById('mcu-temp-val').innerText = mcuT + " °C";
            document.getElementById('pot-val').innerText = p;

            // Zirve Deger Guncellemeleri
            if (t > peakMaxTemp) peakMaxTemp = t;
            if (mcuT > peakMaxMcuTemp) peakMaxMcuTemp = mcuT;
            document.getElementById('peak-temp').innerText = `${peakMaxTemp} °C / MCU: ${peakMaxMcuTemp} °C`;

            // Esik Alarm Kontrolu
            let alarmBox = document.getElementById('alarm-banner');
            if (t > 45 || mcuT > 65) {
              alarmBox.style.display = "block";
              alarmBox.innerText = `KRITIK SICAKLIK UYARISI! Harici: ${t}°C, MCU: ${mcuT}°C`;
            } else {
              alarmBox.style.display = "none";
            }

            document.getElementById('temp-bar').style.height = Math.min(100, (t / 80) * 100) + "%";
            document.getElementById('mcu-temp-bar').style.height = Math.min(100, (mcuT / 85) * 100) + "%";
            document.getElementById('pot-bar-horizontal').style.width = Math.min(100, (p / 255) * 100) + "%";
          }
          
          let lat = 0, lon = 0, kmh = 0, alt = 0, hdop = 9.9;
          if (matchLat && matchLon) {
            lat = parseFloat(matchLat[1]).toFixed(6);
            lon = parseFloat(matchLon[1]).toFixed(6);
            document.getElementById('gps-lat').innerText = "Enlem: " + lat;
            document.getElementById('gps-lon').innerText = "Boylam: " + lon;
            
            // FIX: compare as numbers, not strings, so small real movement
            // is detected correctly and a fixed 0,0 doesn't get re-geocoded.
            let latNum = parseFloat(lat);
            let lonNum = parseFloat(lon);
            if (latNum !== 0 && (Math.abs(latNum - lastFetchedLat) > 0.0001 || Math.abs(lonNum - lastFetchedLon) > 0.0001)) {
                lastFetchedLat = latNum;
                lastFetchedLon = lonNum;
                fetchAddress(lat, lon);
                let mapBtn = document.getElementById('map-link');
                mapBtn.href = "https://www.google.com/maps?q=" + lat + "," + lon;
                mapBtn.style.background = "#238636";
                mapBtn.innerText = "HARITADA AC";
            }
          }

          if (matchSpd) {
             kmh = (parseFloat(matchSpd[1]) * 1.852).toFixed(1);
             document.getElementById('speed-val').innerText = kmh + " km/s";
             if (parseFloat(kmh) > peakMaxSpeed) {
                 peakMaxSpeed = kmh;
                 document.getElementById('peak-speed').innerText = peakMaxSpeed + " km/s";
             }
          }

          if (matchCog) {
             document.getElementById('cog-val') && (document.getElementById('cog-val').innerText = parseFloat(matchCog[1]).toFixed(1) + "°");
          }

          if (matchAlt) {
             alt = parseFloat(matchAlt[1]).toFixed(1);
             document.getElementById('alt-val').innerText = alt + " m";
             if (parseFloat(alt) > peakMaxAlt) {
                 peakMaxAlt = alt;
                 document.getElementById('peak-alt').innerText = peakMaxAlt + " m";
             }
          }

          if (matchMode && matchSat) {
             let sat = matchSat[1];
             let mode = matchMode[1];
             document.getElementById('fix-mode-val').innerText = `${mode} Fix (${sat} Uydu)`;
          }

          if (matchTime) {
             let tStr = matchTime[1];
             if(tStr.length >= 6) {
                let utcHours = parseInt(tStr.substring(0,2));
                let localHours = (utcHours + 3) % 24; // Turkiye UTC+3 Saat Kaydirmasi
                let hh = localHours.toString().padStart(2, '0');
                let mm = tStr.substring(2,4);
                let ss = tStr.substring(4,6);
                document.getElementById('utc-clock').innerText = `${hh}:${mm}:${ss} TR (UTC+3)`;
             }
          }

          if (matchHdop) {
             hdop = parseFloat(matchHdop[1]);
             let qualityText = "Mukemmel";
             let color = "#3fb950";
             if (hdop > 2.0 && hdop <= 5.0) { qualityText = "Iyi"; color = "#d29922"; }
             else if (hdop > 5.0) { qualityText = "Zayif"; color = "#f85149"; }
             document.getElementById('hdop-val').innerText = `${hdop} (${qualityText})`;
             document.getElementById('hdop-val').style.color = color;
          }

          // CSV Log Havuzuna Ekle
          let timestampIso = new Date().toISOString();
          csvLogData += `${timestampIso},${document.getElementById('counter').innerText},${document.getElementById('temp-val').innerText},${document.getElementById('mcu-temp-val').innerText},${document.getElementById('pot-val').innerText},${lat},${lon},${kmh},${alt},${hdop}\n`;
        })
        .catch(err => { /* FIX: swallow transient fetch errors instead of an unhandled rejection spamming the console */ });
    }

    function downloadCSV() {
      let blob = new Blob([csvLogData], { type: 'text/csv;charset=utf-8;' });
      let url = URL.createObjectURL(blob);
      let a = document.createElement('a');
      a.href = url;
      a.download = "telemetri_ucus_logu.csv";
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url);
    }

    function fetchAddress(lat, lon) {
      const url = `https://nominatim.openstreetmap.org/reverse?format=json&lat=${lat}&lon=${lon}&accept-language=tr`;
      fetch(url)
        .then(res => res.json())
        .then(data => {
          if (data && data.address) {
            let addr = data.address;
            let mahalle = addr.neighbourhood || addr.suburb || addr.quarter || "";
            let sokak = addr.road || "";
            let ilce = addr.county || addr.town || "";
            let il = addr.province || addr.city || "";
            document.getElementById('address-text').innerText = `${mahalle} ${sokak}, ${ilce} / ${il}`.trim();
          }
        }).catch(err => {});
    }

    setInterval(updateData, 1000);
  </script>
</head>
<body>

  <div class="alarm-banner" id="alarm-banner">SISTEM UYARI PANELI AKTIF</div>

  <div class="header">
    <div id="utc-clock" style="font-size: 13px; color: #8b949e;">--:--:-- TR</div>
    <h2>STM32 TELEMETRY GCS - ULTIMATE TACTICAL</h2>
    <div style="display: flex; gap: 10px;">
      <div class="status-box" id="mission-timer">Uptime: 00:00</div>
      <div class="status-box" id="link-quality" style="color: #3fb950;">Link: %100</div>
    </div>
  </div>

  <div class="grid-container">
    <div class="panel">
      <h4>HARICI SICAKLIK (LM35)</h4>
      <div class="bar-container"><div class="bar-fill" id="temp-bar"></div></div>
      <div class="val-text" id="temp-val">0 °C</div>
    </div>

    <div class="panel">
      <h4>MCU IC SICAKLIK</h4>
      <div class="bar-container"><div class="bar-fill" id="mcu-temp-bar" style="background: #f0883e;"></div></div>
      <div class="val-text" id="mcu-temp-val" style="color: #f0883e;">0 °C</div>
    </div>

    <div class="panel">
      <h4>YER HIZI</h4>
      <div style="font-size: 22px; margin: 10px 0;">🚀</div>
      <div class="val-text" id="speed-val" style="color: #58a6ff;">0.0 km/s</div>
    </div>

    <div class="panel">
      <h4>IRTIFA (RAKIM)</h4>
      <div style="font-size: 22px; margin: 10px 0;">🏔️</div>
      <div class="val-text" id="alt-val" style="color: #d29922;">0.0 m</div>
    </div>

    <div class="panel">
      <h4>SIN./HDOP & FIX</h4>
      <div style="font-size: 18px; margin: 5px 0;" id="fix-mode-val">NO Fix</div>
      <div class="val-text" id="hdop-val" style="font-size: 13px; color: #3fb950;">Bekleniyor</div>
    </div>
  </div>

  <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-bottom: 12px;">
     <div class="gauge-panel">
       <h4>GAZ / POT & SAYAC</h4>
       <div class="gauge-val" id="pot-val">0</div>
       <div style="width: 100%; height: 10px; background: #1a2233; border: 1px solid #30363d; border-radius: 3px; overflow: hidden; margin-bottom: 8px;">
         <div id="pot-bar-horizontal" style="height: 100%; width: 0%; background: #3fb950;"></div>
       </div>
       <div class="metrics-box" style="margin: 0 auto;">
         <div>PAKET SAYACI (C): <span id="counter" style="color:#fff; float:right;">0</span></div>
       </div>
     </div>

     <div class="gauge-panel" style="text-align: left; padding: 15px;">
       <h4 style="text-align: center; margin-bottom: 10px;">PIK (ZIRVE) DEGERLER</h4>
       <div style="font-size: 13px; margin-bottom: 6px;">MAX HIZ: <span id="peak-speed" style="color:#58a6ff; float:right; font-weight:bold;">0.0 km/s</span></div>
       <div style="font-size: 13px; margin-bottom: 6px;">MAX IRTIFA: <span id="peak-alt" style="color:#d29922; float:right; font-weight:bold;">0.0 m</span></div>
       <div style="font-size: 13px; margin-bottom: 10px;">MAX SICAKLIK: <span id="peak-temp" style="color:#f85149; float:right; font-weight:bold;">0 °C</span></div>
       <div style="text-align: center; margin-top: 15px;">
         <button class="btn" onclick="downloadCSV()">CSV LOG DOSYASINI INDIR</button>
       </div>
     </div>
  </div>

  <div style="display: grid; grid-template-columns: 2fr 1fr; gap: 12px;">
    <div class="panel" style="align-items: stretch; text-align: left; padding: 12px;">
      <h4>GPS KOORDINAT KILITLENMESI & ADRES</h4>
      <div id="gps-lat" style="font-size: 13px; color:#fff; margin-bottom:3px;">Enlem: 0.000000</div>
      <div id="gps-lon" style="font-size: 13px; color:#fff; margin-bottom:5px;">Boylam: 0.000000</div>
      <div id="address-text" style="font-size: 12px; color: #00ffcc; margin-bottom: 8px;">Konum cozumleniyor...</div>
      <div><a id="map-link" href="#" target="_blank" style="background:#484f58; color:#fff; padding:5px 10px; border-radius:4px; text-decoration:none; font-size:11px; font-weight:bold;">HARITADA AC</a></div>
    </div>
    
    <div class="panel" style="align-items: stretch; text-align: left; padding: 12px;">
      <h4>SISTEM BILGISI</h4>
      <div style="font-size: 12px; color: #fff; margin-top: 5px;">Baglanti: ESP8266 Wi-Fi Bridge</div>
      <div style="font-size: 12px; color: #fff; margin-top: 3px;">Protokol: NMEA / CAN Bus</div>
    </div>
  </div>

</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", MAIN_page);
}

void handleData() {
  server.send(200, "text/plain", lastTelemetry);
}

void setup() {
  // FIX: bump the UART RX ring buffer well above one telemetry line
  // (~220 bytes). The default 256-byte buffer plus WiFi/HTTP servicing
  // jitter in loop() was a plausible cause of the dropped/garbled lines
  // seen as low "Link %" in the dashboard.
  Serial.setRxBufferSize(512);
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();

  // FIX: bound how long we spend draining Serial per loop() iteration so a
  // stuck/endless stream can never starve server.handleClient(), and only
  // accept lines that actually look like a complete telemetry frame.
  uint32_t guard = 0;
  while (Serial.available() > 0 && guard < 20) {
    String incoming = Serial.readStringUntil('\n');
    incoming.trim();
    if (incoming.length() > 0 && incoming.startsWith("RX->")) {
      lastTelemetry = incoming;
    }
    guard++;
  }
}

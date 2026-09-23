#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>

const char* ssid = "STM32_Telemetri_Station";
const char* password = "12345678password";

ESP8266WebServer server(80);

String lastTelemetry = "RX-> C:0 | T:0 | MCU_T:0 | P:0 | Dist:0 | Lat:0.000000 | Lon:0.000000 | Spd:0.0 | Cog:0.0 | Time:000000 | Hdop:9.9 | Alt:0.0 | Mode:NO | Sat:0";

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

    .grid-container { display: grid; grid-template-columns: repeat(6, 1fr); gap: 10px; margin-bottom: 15px; }
    .panel { background: #111622; border: 1px solid #1f293d; border-radius: 6px; padding: 10px; text-align: center; box-shadow: 0 4px 10px rgba(0,0,0,0.6); display: flex; flex-direction: column; justify-content: center; align-items: center;}
    .panel h4 { margin: 0 0 6px 0; color: #8b949e; font-size: 10px; text-transform: uppercase; letter-spacing: 1px;}

    .bar-container { width: 22px; height: 80px; background: #1a2233; border: 1px solid #30363d; margin: 0 auto 6px auto; position: relative; border-radius: 4px; display: flex; align-items: flex-end; }
    .bar-fill { width: 100%; background: #00ffcc; height: 0%; border-radius: 2px; transition: height 0.3s ease; }
    .val-text { font-size: 16px; font-weight: bold; color: #00ffcc; margin-top: 4px;}

    .gauge-panel { background: #111622; border: 1px solid #1f293d; border-radius: 6px; padding: 12px; text-align: center; box-shadow: 0 4px 10px rgba(0,0,0,0.6); }
    .gauge-val { font-size: 32px; font-weight: bold; color: #3fb950; margin: 4px 0; }
    .metrics-box { background: #161b22; border: 1px solid #30363d; padding: 8px; border-radius: 4px; margin-top: 8px; text-align: left; font-size: 12px; width: 92%; }

    .alarm-banner { background: #f85149; color: #fff; padding: 8px; border-radius: 4px; text-align: center; font-weight: bold; margin-bottom: 15px; display: none; animation: blink 1s infinite; }
    @keyframes blink { 50% { opacity: 0.4; } }

    .btn { background: #238636; color: #fff; padding: 8px 12px; border-radius: 4px; text-decoration: none; font-size: 12px; font-weight: bold; border: none; cursor: pointer; transition: background 0.3s; }
    .btn:hover { background: #2ea043; }

    .chart-container { background: #111622; border: 1px solid #1f293d; border-radius: 6px; padding: 12px; margin-bottom: 15px; box-shadow: 0 4px 10px rgba(0,0,0,0.6); }
    .legend-item { display: inline-flex; align-items: center; margin-right: 14px; font-size: 11px; color: #c9d1d9; }
    .legend-dot { width: 10px; height: 10px; border-radius: 50%; display: inline-block; margin-right: 5px; }
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

    let csvLogData = "Zaman,Sayac,Sicaklik,MCU_Sicaklik,Gaz,Mesafe_Cm,Enlem,Boylam,Hiz_Kmh,Irtifa,HDOP\n";

    // ---- Canvas tabanlı grafik (Chart.js YERINE) ----
    const MAX_POINTS = 30;
    let chartLabels = [];
    let chartTempData = [];
    let chartMcuTempData = [];
    let chartSpeedData = [];

    function pushChartPoint(label, t, mcuT, spd) {
      if (chartLabels.length >= MAX_POINTS) {
        chartLabels.shift();
        chartTempData.shift();
        chartMcuTempData.shift();
        chartSpeedData.shift();
      }
      chartLabels.push(label);
      chartTempData.push(t);
      chartMcuTempData.push(mcuT);
      chartSpeedData.push(spd);
      drawChart();
    }

    function drawChart() {
      const canvas = document.getElementById('telemetryChart');
      if (!canvas) return;
      const dpr = window.devicePixelRatio || 1;
      const cssW = canvas.clientWidth;
      const cssH = canvas.clientHeight;
      canvas.width = cssW * dpr;
      canvas.height = cssH * dpr;
      const ctx = canvas.getContext('2d');
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      ctx.clearRect(0, 0, cssW, cssH);

      const padL = 34, padR = 10, padT = 10, padB = 22;
      const w = cssW - padL - padR;
      const h = cssH - padT - padB;

      if (chartLabels.length < 2) {
        ctx.fillStyle = '#8b949e';
        ctx.font = '11px Courier New';
        ctx.fillText('Veri bekleniyor...', padL, padT + h / 2);
        return;
      }

      // Y ekseni: iki farklı skala kullanacagiz (sicaklik 0-80, hiz 0-max ama gorsel olarak ayni eksene sigdiriyoruz)
      let allVals = chartTempData.concat(chartMcuTempData).concat(chartSpeedData);
      let maxVal = Math.max(80, ...allVals);
      let minVal = 0;

      // Izgara cizgileri
      ctx.strokeStyle = '#1f293d';
      ctx.lineWidth = 1;
      ctx.fillStyle = '#8b949e';
      ctx.font = '10px Courier New';
      const gridLines = 4;
      for (let i = 0; i <= gridLines; i++) {
        let y = padT + (h * i / gridLines);
        ctx.beginPath();
        ctx.moveTo(padL, y);
        ctx.lineTo(padL + w, y);
        ctx.stroke();
        let val = maxVal - ((maxVal - minVal) * i / gridLines);
        ctx.fillText(val.toFixed(0), 2, y + 3);
      }

      // X ekseni etiketleri (ilk, orta, son)
      ctx.fillText(chartLabels[0], padL, cssH - 6);
      if (chartLabels.length > 2) {
        let midIdx = Math.floor(chartLabels.length / 2);
        let midX = padL + (w * midIdx / (chartLabels.length - 1));
        ctx.fillText(chartLabels[midIdx], midX - 14, cssH - 6);
      }
      let lastX = padL + w;
      ctx.fillText(chartLabels[chartLabels.length - 1], lastX - 30, cssH - 6);

      function drawSeries(data, color) {
        ctx.beginPath();
        ctx.strokeStyle = color;
        ctx.lineWidth = 2;
        data.forEach((val, i) => {
          let x = padL + (w * i / (chartLabels.length - 1));
          let y = padT + h - ((val - minVal) / (maxVal - minVal || 1)) * h;
          if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
        });
        ctx.stroke();

        // Noktalar
        ctx.fillStyle = color;
        data.forEach((val, i) => {
          let x = padL + (w * i / (chartLabels.length - 1));
          let y = padT + h - ((val - minVal) / (maxVal - minVal || 1)) * h;
          ctx.beginPath();
          ctx.arc(x, y, 2, 0, Math.PI * 2);
          ctx.fill();
        });
      }

      drawSeries(chartTempData, '#00ffcc');
      drawSeries(chartMcuTempData, '#f0883e');
      drawSeries(chartSpeedData, '#58a6ff');
    }

    window.addEventListener('resize', drawChart);
    window.onload = function() {
      drawChart();
    };
    // ---- Canvas grafik sonu ----

    function updateData() {
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
          const matchDist = raw.match(/Dist:(\d+)/);
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
            let dist = matchDist ? parseInt(matchDist[1]) : 0;

            totalPackets++;
            if (expectedCounter !== null) {
              let diff = c - expectedCounter;
              if (diff > 0) { lostPackets += diff; }
              else if (diff < 0) { lostPackets += (256 + diff); }
            }
            expectedCounter = (c + 1) % 256;

            let lossPercent = (totalPackets + lostPackets) > 0 ? ((lostPackets / (totalPackets + lostPackets)) * 100).toFixed(1) : 0;
            let linkQuality = (100 - lossPercent).toFixed(1);
            document.getElementById('link-quality').innerText = `Link: %${linkQuality}`;

            document.getElementById('counter').innerText = c;
            document.getElementById('temp-val').innerText = t + " °C";
            document.getElementById('mcu-temp-val').innerText = mcuT + " °C";
            document.getElementById('pot-val').innerText = p;
            document.getElementById('dist-val').innerText = dist + " cm";

            if (t > peakMaxTemp) peakMaxTemp = t;
            if (mcuT > peakMaxMcuTemp) peakMaxMcuTemp = mcuT;
            document.getElementById('peak-temp').innerText = `${peakMaxTemp} °C / MCU: ${peakMaxMcuTemp} °C`;

            let alarmBox = document.getElementById('alarm-banner');
            if (t > 35 || mcuT > 35 || (dist > 0 && dist < 30)) {
              alarmBox.style.display = "block";
              alarmBox.innerText = `KRITIK UYARI! Sicaklik > 35°C veya Yakinlik Alarmi! (T:${t}°C, MCU:${mcuT}°C, Dist:${dist}cm)`;
            } else {
              alarmBox.style.display = "none";
            }

            document.getElementById('temp-bar').style.height = Math.min(100, (t / 80) * 100) + "%";
            document.getElementById('mcu-temp-bar').style.height = Math.min(100, (mcuT / 85) * 100) + "%";
            document.getElementById('dist-bar').style.height = Math.min(100, (dist / 200) * 100) + "%";
            document.getElementById('pot-bar-horizontal').style.width = Math.min(100, (p / 255) * 100) + "%";

            let spdForChart = parseFloat(document.getElementById('speed-val').innerText) || 0;
            pushChartPoint(`${mins}:${secs}`, t, mcuT, spdForChart);
          }

          let lat = 0, lon = 0, kmh = 0, alt = 0, hdop = 9.9;
          if (matchLat && matchLon) {
            lat = parseFloat(matchLat[1]).toFixed(6);
            lon = parseFloat(matchLon[1]).toFixed(6);
            document.getElementById('gps-lat').innerText = "Enlem: " + lat;
            document.getElementById('gps-lon').innerText = "Boylam: " + lon;

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
                let localHours = (utcHours + 3) % 24;
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

          let timestampIso = new Date().toISOString();
          csvLogData += `${timestampIso},${document.getElementById('counter').innerText},${document.getElementById('temp-val').innerText},${document.getElementById('mcu-temp-val').innerText},${document.getElementById('pot-val').innerText},${document.getElementById('dist-val').innerText},${lat},${lon},${kmh},${alt},${hdop}\n`;
        })
        .catch(err => {});
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
      // NOT: Bu ozellik internet gerektirir, AP modunda calismaz. Sessizce basarisiz olur.
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
      <h4>HARICI SICAKLIK</h4>
      <div class="bar-container"><div class="bar-fill" id="temp-bar"></div></div>
      <div class="val-text" id="temp-val">0 °C</div>
    </div>

    <div class="panel">
      <h4>MCU IC SICAKLIK</h4>
      <div class="bar-container"><div class="bar-fill" id="mcu-temp-bar" style="background: #f0883e;"></div></div>
      <div class="val-text" id="mcu-temp-val" style="color: #f0883e;">0 °C</div>
    </div>

    <div class="panel">
      <h4>MESAFE (HC-SR04)</h4>
      <div class="bar-container"><div class="bar-fill" id="dist-bar" style="background: #3fb950;"></div></div>
      <div class="val-text" id="dist-val" style="color: #3fb950;">0 cm</div>
    </div>

    <div class="panel">
      <h4>YER HIZI</h4>
      <div style="font-size: 20px; margin: 12px 0;">🚀</div>
      <div class="val-text" id="speed-val" style="color: #58a6ff; font-size:14px;">0.0 km/s</div>
    </div>

    <div class="panel">
      <h4>IRTIFA (RAKIM)</h4>
      <div style="font-size: 20px; margin: 12px 0;">🏔️</div>
      <div class="val-text" id="alt-val" style="color: #d29922; font-size:14px;">0.0 m</div>
    </div>

    <div class="panel">
      <h4>SIN./HDOP & FIX</h4>
      <div style="font-size: 14px; margin: 4px 0;" id="fix-mode-val">NO Fix</div>
      <div class="val-text" id="hdop-val" style="font-size: 11px; color: #3fb950;">Bekleniyor</div>
    </div>
  </div>

  <div class="chart-container">
    <h4 style="margin: 0 0 6px 0; color: #8b949e; font-size: 11px; text-transform: uppercase; letter-spacing: 1px;">CANLI TELEMETRI GRAFIGI</h4>
    <div style="margin-bottom: 8px;">
      <span class="legend-item"><span class="legend-dot" style="background:#00ffcc;"></span>Harici Sicaklik (°C)</span>
      <span class="legend-item"><span class="legend-dot" style="background:#f0883e;"></span>MCU Sicaklik (°C)</span>
      <span class="legend-item"><span class="legend-dot" style="background:#58a6ff;"></span>Hiz (km/s)</span>
    </div>
    <div style="position: relative; height: 180px; width: 100%;">
      <canvas id="telemetryChart" style="width:100%; height:100%; display:block;"></canvas>
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
      <div style="font-size: 12px; color: #fff; margin-top: 5px;">Baglanti: ESP8266 Wi-Fi Bridge + OTA</div>
      <div style="font-size: 12px; color: #fff; margin-top: 3px;">Donanim: HC-SR04 / Buzzer / LED</div>
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
  Serial.setRxBufferSize(512);
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  ArduinoOTA.setHostname("STM32-GCS-OTA");
  ArduinoOTA.begin();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();

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

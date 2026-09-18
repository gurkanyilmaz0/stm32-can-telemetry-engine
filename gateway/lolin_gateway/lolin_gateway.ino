#include <ESP8266WiFi.h>

#include <ESP8266WebServer.h>



const char* ssid = "STM32_Telemetri_Station";

const char* password = "12345678password";



ESP8266WebServer server(80);



String lastTelemetry = "RX-> C:0 | T:0 | P:0 | CRC:0 | Err:0";



const char MAIN_page[] PROGMEM = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

  <meta charset="utf-8">

  <title>STM32 Telemetri - Yer Istasyonu (GCS)</title>

  <style>

    body { background-color: #0b0f19; color: #00ffcc; font-family: 'Courier New', Courier, monospace; margin: 0; padding: 15px; }

    .header { display: flex; justify-content: space-between; align-items: center; border-bottom: 2px solid #1f293d; padding-bottom: 10px; margin-bottom: 20px; }

    .header h2 { margin: 0; color: #ffffff; letter-spacing: 2px; font-size: 20px; }

    .status-box { background: #161b22; border: 1px solid #30363d; padding: 5px 15px; border-radius: 4px; color: #00ffcc; font-weight: bold; }

   

    .grid-container { display: grid; grid-template-columns: 1fr 2fr 1fr; gap: 20px; align-items: center; justify-items: center; }

   

    .panel { background: #111622; border: 1px solid #1f293d; border-radius: 6px; padding: 15px; width: 100%; text-align: center; box-shadow: 0 4px 10px rgba(0,0,0,0.6); }

    .panel h4 { margin: 0 0 10px 0; color: #8b949e; font-size: 12px; text-transform: uppercase; }

   

    /* Dikey Bar Göstergeleri */

    .bar-container { width: 30px; height: 150px; background: #1a2233; border: 1px solid #30363d; margin: 0 auto 10px auto; position: relative; border-radius: 3px; display: flex; align-items: flex-end; }

    .bar-fill { width: 100%; background: #00ffcc; height: 0%; border-radius: 2px; transition: height 0.3s ease; }

    .val-text { font-size: 18px; font-weight: bold; color: #00ffcc; }



    /* Orta Panel */

    .gauge-panel { background: #111622; border: 1px solid #1f293d; border-radius: 6px; padding: 20px; text-align: center; width: 320px; box-shadow: 0 4px 10px rgba(0,0,0,0.6); }

    .gauge-val { font-size: 42px; font-weight: bold; color: #00ffcc; margin: 10px 0; }

   

    .metrics-box { background: #161b22; border: 1px solid #30363d; padding: 10px; border-radius: 4px; margin-top: 10px; text-align: left; font-size: 12px; }

  </style>

  <script>

    function updateData() {

      fetch('/data')

        .then(response => response.text())

        .then(raw => {

          const matchC = raw.match(/C:(\d+)/);

          const matchT = raw.match(/T:(\d+)/);

          const matchP = raw.match(/P:(\d+)/);

          const matchErr = raw.match(/Err:(\d+)/);



          if (matchC && matchT && matchP) {

            let c = parseInt(matchC[1]);

            let t = parseInt(matchT[1]);

            let p = parseInt(matchP[1]);

            let err = matchErr ? parseInt(matchErr[1]) : 0;



            document.getElementById('counter').innerText = c;

            document.getElementById('temp-val').innerText = t + " °C";

            document.getElementById('pot-val').innerText = p;

            document.getElementById('err-val').innerText = err;



            let tempPercent = Math.min(100, (t / 80) * 100);

            let potPercent = Math.min(100, (p / 255) * 100);

           

            document.getElementById('temp-bar').style.height = tempPercent + "%";

            document.getElementById('pot-bar').style.height = potPercent + "%";



            let statEl = document.getElementById('sys-status');

            if(err === 0) {

              statEl.innerText = "SİSTEM HAZIR";

              statEl.style.color = "#00ffcc";

            } else {

              statEl.innerText = "HATA KODU: " + err;

              statEl.style.color = "#ff4d4d";

            }

          }

        });

    }

    setInterval(updateData, 500);



    function updateClock() {

      const now = new Date();

      document.getElementById('clock').innerText = now.toLocaleDateString() + " | " + now.toLocaleTimeString();

    }

    setInterval(updateClock, 1000);

  </script>

</head>

<body>



  <div class="header">

    <div id="clock" style="font-size: 14px; color: #8b949e;">--/--/---- | --:--:--</div>

    <h2>STM32 & CAN TELEMETRY GCS</h2>

    <div class="status-box" id="sys-status">SİSTEM BAĞLANIYOR...</div>

  </div>



  <div class="grid-container">

    <!-- Sol Panel: Sıcaklık Barı -->

    <div class="panel">

      <h4>LM35 Sıcaklık</h4>

      <div class="bar-container">

        <div class="bar-fill" id="temp-bar"></div>

      </div>

      <div class="val-text" id="temp-val">0 °C</div>

    </div>



    <!-- Orta Panel: Potansiyometre ve Sayaç -->

    <div class="gauge-panel">

      <h4>POTANSİYOMETRE GİRİŞİ</h4>

      <div class="gauge-val" id="pot-val">0</div>

      <div style="color: #8b949e; font-size: 12px;">HAM VERİ (0 - 255)</div>

     

      <div class="metrics-box">

        <div>PAKET SAYACI (C): <span id="counter" style="color:#fff; float:right;">0</span></div>

        <div>KRİTİK HATA (ERR): <span id="err-val" style="color:#00ffcc; float:right;">0</span></div>

      </div>

    </div>



    <!-- Sağ Panel: Potansiyometre Barı -->

    <div class="panel">

      <h4>Pot Seviyesi</h4>

      <div class="bar-container">

        <div class="bar-fill" id="pot-bar" style="background: #3fb950;"></div>

      </div>

      <div class="val-text" id="pot-val-bar">Seviye</div>

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

  Serial.begin(115200);

  WiFi.softAP(ssid, password);

 

  server.on("/", handleRoot);

  server.on("/data", handleData);

  server.begin();

}



void loop() {

  server.handleClient();



  while (Serial.available() > 0) {

    String incoming = Serial.readStringUntil('\n');

    incoming.trim();

    if (incoming.length() > 0 && incoming.startsWith("RX->")) {

      lastTelemetry = incoming;

    }

  }

} 
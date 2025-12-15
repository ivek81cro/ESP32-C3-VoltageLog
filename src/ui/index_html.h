#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char indexHtml[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="hr">
<head>
  <meta charset="UTF-8">
  <title>ESP32 WiFi Config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style id="styles"></style>
</head>
<body>
  <div class="container">
    <h1>ESP32 WiFi konfiguracija</h1>
    <p class="subtitle">
      Spoji se na ovu stranicu, odaberi mrežu i upiši lozinku. Uređaj će se
      zatim restartati i pokušati spojiti na tvoj WiFi.
    </p>

    <form id="wifiForm" method="POST" action="/config">
      <label for="ssid">SSID (naziv mreže)</label>
      <input type="text" id="ssid" name="ssid" required>

      <label for="password">Lozinka</label>
      <input type="password" id="password" name="password" required>

      <div class="actions">
        <button type="submit" class="btn-primary">Spremi i restartaj</button>
        <button type="button" class="btn-secondary" id="scanBtn">Skeniraj mreže</button>
      </div>
    </form>

    <div class="status" id="statusText">
      Status: spremno.
    </div>

    <div class="networks">
      <h2>Dostupne mreže</h2>
      <ul class="network-list" id="networkList"></ul>
      <p class="small">Dodirni mrežu da automatski upiše SSID u polje iznad.</p>
    </div>
  </div>

  <script id="script"></script>
</body>
</html>
)HTML";

#endif // INDEX_HTML_H

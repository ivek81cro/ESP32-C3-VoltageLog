#ifndef INDEX_HTML_H
#define INDEX_HTML_H

<<<<<<< HEAD
// HTML template with placeholder tags for CSS and JS injection
const char indexHtml[] PROGMEM = R"HTML(<!DOCTYPE html>
=======
const char indexHtml[] PROGMEM = R"HTML(
<!DOCTYPE html>
>>>>>>> 1ce8b9d6805a4a5ba881cb1dc6d351247d385537
<html lang="hr">
<head>
  <meta charset="UTF-8">
  <title>ESP32 WiFi Config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
<<<<<<< HEAD
  <style>
<!--CSS_PLACEHOLDER-->
  </style>
=======
  <style id="styles"></style>
>>>>>>> 1ce8b9d6805a4a5ba881cb1dc6d351247d385537
</head>
<body>
  <div class="container">
    <h1>ESP32 WiFi konfiguracija</h1>
    <p class="subtitle">
      Spoji se na ovu stranicu, odaberi mrežu i upiši lozinku. Uređaj će se
      zatim restartati i pokušati spojiti na tvoj WiFi.
    </p>
<<<<<<< HEAD
    <form id="wifiForm" method="POST" action="/config">
      <label for="ssid">SSID (naziv mreže)</label>
      <input type="text" id="ssid" name="ssid" required>
      <label for="password">Lozinka</label>
      <input type="password" id="password" name="password" required>
=======

    <form id="wifiForm" method="POST" action="/config">
      <label for="ssid">SSID (naziv mreže)</label>
      <input type="text" id="ssid" name="ssid" required>

      <label for="password">Lozinka</label>
      <input type="password" id="password" name="password" required>

>>>>>>> 1ce8b9d6805a4a5ba881cb1dc6d351247d385537
      <div class="actions">
        <button type="submit" class="btn-primary">Spremi i restartaj</button>
        <button type="button" class="btn-secondary" id="scanBtn">Skeniraj mreže</button>
      </div>
    </form>
<<<<<<< HEAD
    <div class="status" id="statusText">Status: spremno.</div>
=======

    <div class="status" id="statusText">
      Status: spremno.
    </div>

>>>>>>> 1ce8b9d6805a4a5ba881cb1dc6d351247d385537
    <div class="networks">
      <h2>Dostupne mreže</h2>
      <ul class="network-list" id="networkList"></ul>
      <p class="small">Dodirni mrežu da automatski upiše SSID u polje iznad.</p>
    </div>
  </div>
<<<<<<< HEAD
  <script>
<!--JS_PLACEHOLDER-->
  </script>
</body>
</html>)HTML";
=======

  <script id="script"></script>
</body>
</html>
)HTML";
>>>>>>> 1ce8b9d6805a4a5ba881cb1dc6d351247d385537

#endif // INDEX_HTML_H

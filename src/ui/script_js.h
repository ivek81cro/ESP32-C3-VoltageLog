#ifndef SCRIPT_JS_H
#define SCRIPT_JS_H

const char scriptJs[] PROGMEM = R"JS(
const scanBtn = document.getElementById("scanBtn");
const listEl = document.getElementById("networkList");
const statusEl = document.getElementById("statusText");
const ssidInput = document.getElementById("ssid");

function setStatus(text) {
  statusEl.textContent = "Status: " + text;
  console.log(text);
}

scanBtn.addEventListener("click", () => {
  setStatus("skeniram WiFi mreže...");
  listEl.innerHTML = "";
  fetch("/scan")
    .then(r => {
      if (!r.ok) throw new Error("HTTP " + r.status);
      return r.json();
    })
    .then(data => {
      if (!Array.isArray(data)) {
        setStatus("neočekivan odgovor sa /scan");
        return;
      }

      if (data.length === 0) {
        setStatus("nema pronađenih mreža.");
        return;
      }

      data.sort((a, b) => b.rssi - a.rssi);
      setStatus("pronađeno " + data.length + " mreža.");

      data.forEach(ap => {
        const li = document.createElement("li");
        li.className = "network-item";

        const left = document.createElement("div");
        left.className = "network-ssid";
        left.textContent = ap.ssid || "(skriveni SSID)";

        const right = document.createElement("div");
        right.className = "network-rssi";
        right.textContent = ap.rssi + " dBm";

        li.appendChild(left);
        li.appendChild(right);

        li.addEventListener("click", () => {
          if (ap.ssid) {
            ssidInput.value = ap.ssid;
            setStatus("odabrana mreža: " + ap.ssid);
          }
        });

        listEl.appendChild(li);
      });
    })
    .catch(err => {
      console.error(err);
      setStatus("greška pri skeniranju: " + err.message);
    });
});
)JS";

#endif // SCRIPT_JS_H

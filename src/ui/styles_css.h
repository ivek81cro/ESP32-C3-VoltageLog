#ifndef STYLES_CSS_H
#define STYLES_CSS_H

const char stylesCss[] PROGMEM = R"CSS(
body {
  font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
  margin: 0;
  padding: 0;
  background: #f3f4f6;
  color: #111827;
}

.container {
  max-width: 480px;
  margin: 32px auto;
  padding: 24px;
  background: #ffffff;
  border-radius: 12px;
  box-shadow: 0 10px 30px rgba(0,0,0,0.08);
}

h1 {
  font-size: 1.4rem;
  margin: 0 0 4px 0;
  color: #111827;
}

.subtitle {
  margin: 0 0 16px 0;
  font-size: 0.9rem;
  color: #6b7280;
}

label {
  display: block;
  font-size: 0.9rem;
  margin: 12px 0 4px;
}

input[type="text"],
input[type="password"] {
  width: 100%;
  padding: 8px 10px;
  box-sizing: border-box;
  border-radius: 8px;
  border: 1px solid #d1d5db;
  font-size: 0.95rem;
}

input[type="text"]:focus,
input[type="password"]:focus {
  outline: none;
  border-color: #2563eb;
  box-shadow: 0 0 0 1px #2563eb22;
}

button {
  cursor: pointer;
  border: none;
  border-radius: 9999px;
  padding: 8px 16px;
  font-size: 0.95rem;
  font-weight: 500;
  display: inline-flex;
  align-items: center;
  gap: 6px;
}

.btn-primary {
  background: #2563eb;
  color: white;
}

.btn-primary:active {
  transform: translateY(1px);
}

.btn-secondary {
  background: #e5e7eb;
  color: #111827;
}

.actions {
  margin-top: 16px;
  display: flex;
  justify-content: space-between;
  gap: 8px;
  flex-wrap: wrap;
}

.status {
  margin-top: 12px;
  font-size: 0.85rem;
  color: #6b7280;
}

.networks {
  margin-top: 20px;
  border-top: 1px solid #e5e7eb;
  padding-top: 16px;
}

.networks h2 {
  font-size: 1rem;
  margin: 0 0 8px;
}

.network-list {
  list-style: none;
  padding: 0;
  margin: 0;
  max-height: 180px;
  overflow-y: auto;
}

.network-item {
  border-radius: 8px;
  padding: 6px 8px;
  margin-bottom: 6px;
  border: 1px solid #e5e7eb;
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 0.9rem;
}

.network-ssid {
  font-weight: 500;
}

.network-rssi {
  font-size: 0.8rem;
  color: #6b7280;
}

.small {
  font-size: 0.8rem;
  color: #9ca3af;
  margin-top: 4px;
}
)CSS";

#endif // STYLES_CSS_H

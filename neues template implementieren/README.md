Gerne, ich erstelle eine **README.md**-Datei für Ihr futuristisches Industrie-Dashboard-Template.

Diese README enthält Anweisungen zur Einrichtung, eine Übersicht über die Funktionen und Hinweise zur Anpassung und Erweiterung, besonders im Hinblick auf die Echtzeitdaten.

---

#🚀 Garten-Kontrollzentrum V1.3: Futuristisches Dashboard##Inhaltsverzeichnis1. [Überblick](https://www.google.com/search?q=%231-%C3%BCberblick)
2. [Technologien](https://www.google.com/search?q=%232-technologien)
3. [Einrichtung](https://www.google.com/search?q=%233-einrichtung)
4. [Funktionen & Module](https://www.google.com/search?q=%234-funktionen--module)
5. [Anpassung & Erweiterung](https://www.google.com/search?q=%235-anpassung--erweiterung)

---

##1. ÜberblickDieses Projekt ist ein spezialisiertes, **futuristisches und industriell gestaltetes HTML/CSS-Dashboard** zur Überwachung von Cannabis-Wachstumsumgebungen (Grow-Monitor).

Es bietet eine sofortige Visualisierung wichtiger Klimadaten wie **VPD**, **Temperatur**, **Luftfeuchtigkeit** und **\text{CO}_2**, kombiniert mit dynamischen Empfehlungen und Statusinformationen zur Pflanze.

##2. Technologien* **HTML5:** Struktur des Dashboards.
* **CSS3:** Styling (Industrial, Neon, Monospace-Ästhetik).
* **JavaScript:** (Optional) Simulation von Daten, Logik für VPD-Empfehlungen.

##3. EinrichtungDie Einrichtung ist extrem einfach, da keine Abhängigkeiten (Frameworks, externe Bibliotheken) benötigt werden.

1. **Dateien erstellen:** Erstellen Sie die folgenden zwei Dateien im selben Verzeichnis:
* `index.html` (HTML-Struktur)
* `style.css` (CSS-Styling)


2. **Dateien befüllen:** Kopieren Sie den jeweiligen Code in die entsprechenden Dateien.
3. **Öffnen:** Öffnen Sie die Datei `index.html` in Ihrem Webbrowser.

##4. Funktionen & ModuleDas Dashboard ist in spezifische Überwachungsbereiche unterteilt:

| Modul | Daten | Zweck |
| --- | --- | --- |
| **Metriken-Grid** | Temp., RLF, VPD, \text{CO}_2 | Anzeige der aktuellen Messwerte. |
| **VPD-Empfehlungen** | Dynamisch | Bietet auf dem aktuellen VPD basierende Handlungsempfehlungen (z.B. "Feuchte erhöhen"). |
| **Pflanzenstatus** | Alter, Phase (VEGETATION/BLÜTE) | Tracking des Wachstumszyklus. |
| **Historische Analyse** | Temperatur, VPD (Platzhalter) | Visualisierung von 24h-Trends (benötigt Integration). |

##5. Anpassung & Erweiterung###5.1. Echtzeitdaten-IntegrationDerzeit werden die angezeigten Werte durch ein einfaches JavaScript-Snippet (im `<script>`-Tag der `index.html`) simuliert.

Um echte Daten zu integrieren, ersetzen Sie die Platzhalter-Werte:

1. Entfernen oder kommentieren Sie die Funktion `updateDataAndRecs()` und deren Aufrufe.
2. Nutzen Sie **AJAX/Fetch API** in JavaScript, um Daten von Ihrer tatsächlichen Sensor-API (z.B. ESP32, Raspberry Pi, Home Assistant) abzurufen.
3. Aktualisieren Sie die HTML-Elemente (`temp-value`, `vpd-value`, etc.) mit den empfangenen Daten.

```javascript
// Beispiel für die Aktualisierung eines einzelnen Werts mit echten Daten:
document.getElementById('temp-value').textContent = receivedData.temperature.toFixed(1);

```

###5.2. Wachstumsphase wechselnUm die Wachstumsphase von **Vegetation** auf **Blüte** (`Flower`) umzustellen, ändern Sie das `data-phase`-Attribut im `index.html`:

```html
<p class="status-line">Phase: <span class="status-data phase-color" data-phase="Flower">BLÜTE</span></p> 

```

###5.3. Diagramme (Charts)Die Diagramme sind derzeit reine CSS-Platzhalter (`chart-placeholder`). Um sie funktionsfähig zu machen, müssen Sie eine Chart-Bibliothek integrieren:

* **Empfehlung:** **Chart.js** oder **ApexCharts** eignen sich gut und können einfach in den futuristischen Stil (dunkler Hintergrund, Neon-Linien) angepasst werden.
* Ersetzen Sie den `div class="chart-placeholder"` durch das `<canvas>`-Element oder den Container, den die gewählte Bibliothek benötigt.

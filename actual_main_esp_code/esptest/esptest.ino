// =====================================================
// Mars Guard Beta
//
// Authors:
// Jacob Luscombe
// Vincent Watson
// Sumvidh Bharadwaj
// Navtej Vishwanath
// =====================================================



// =====================================================
// 1. Libraries
// =====================================================
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>

// =====================================================
// 2. Configuration
// =====================================================
const char* ssid = "ESP NET";   // Local network name
const char* password = "TestESP32"; // Local network pass

#define DHT_PIN 16      // DHT sensor pin
#define DHT_TYPE DHT11  // Type of DHT sensor being used
#define PIR_PIN 4      // PIR sensor pin
#define LED_GREEN 19   // On when no motion
#define LED_RED 18     // On when motion detected
#define BUZZER_PIN 17  // Active buzzer, on when motion is detected

const unsigned long SENSOR_INTERVAL_MS = 2000; 
const unsigned long WIFI_TIMEOUT_MS = 20000;
const unsigned long PIR_WARMUP_MS = 30000;

// =====================================================
// 3. Objects and global state
// =====================================================
DHT dht(DHT_PIN, DHT_TYPE);     // DHT setup
WebServer server(80);   // Web server setup

float temp = NAN;   // tempruature reading object
float hum = NAN;    // humidity reading object
bool motionState = false;   // Has motion been detected

bool lastRawPIR = false;
unsigned long pirChanges = 0;  // how many times the PIR pin has changed state

// =====================================================
// 4. Web page stuff
// =====================================================

// The actual website that users can see
const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>

<head>
    <meta charset="UTF-8">
<link rel="icon" type="image/x-icon" href="https://raw.githubusercontent.com/thecobkid2009-del/Hackathon-2026/main/favicon.ico">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Mars Base</title>

    <!-- =================================================================
         PART 2: STYLING
         ================================================================= -->
    <style>

        /* ---------------------------------------------------------------
            Colours
           --------------------------------------------------------------- */
        :root {
            --accent: #E3701A;       /* Mars orange */
            --sand-dark: #341D14;    /* dark sand / panel colour */
            --text: #f3d9c4;         /* soft sandy white */
        }

        /* ---------------------------------------------------------------
           Basic page setup
           --------------------------------------------------------------- */
        * { box-sizing: border-box; }
        html { scroll-behavior: smooth; }

        body {
            font-family: Arial, sans-serif;
            line-height: 1.6;
            margin: 0;
            color: var(--text);
            background: #0a0614;
        }


        /* ---------------------------------------------------------------
           Navigation bar 
           --------------------------------------------------------------- */
        nav {
            position: fixed;
            top: 0; left: 0; right: 0;
            display: flex;
            justify-content: center;
            gap: 1.5rem;
            padding: 0.9rem 1rem 0.7rem;
            background: rgba(0, 0, 0, 0.35);
            backdrop-filter: blur(6px);
            z-index: 10;
        }
        nav a {
            color: var(--text);
            text-decoration: none;
            font-size: 0.95rem;
            letter-spacing: 0.08em;
            text-transform: uppercase;
            padding-bottom: 2px;
            border-bottom: 2px solid transparent;
            transition: color .2s, border-color .2s;
        }
        nav a:hover,
        nav a.active {
            color: var(--accent);
            border-color: var(--accent);
        }

        /* ---------------------------------------------------------------
           Animated sky
           --------------------------------------------------------------- */
        #sky {
            position: fixed;
            inset: 0;
            z-index: -1;
            overflow: hidden;
            background: linear-gradient(to bottom, #05030c 0%, #0d0820 60%, #1a0f2e 100%);
        }

        /* Sunset colours, fade in as you scroll (opacity set by JavaScript) */
        #sunset {
            position: absolute;
            inset: 0;
            opacity: 0;
            background: linear-gradient(to bottom,
                #1a0b2e 0%,
                #4a1d44 30%,
                #b5502f 68%,
                #f0a05a 92%);
        }

        /* Stars */
        #stars {
            position: absolute;
            inset: 0 0 30% 0;
        }
        .star {
            position: absolute;
            background: #fff;
            border-radius: 50%;
        }

        #sun {
            position: absolute;
            left: 68%;
            bottom: -10%;
            width: 80px;
            height: 80px;
            border-radius: 50%;
            background: #fff4e0;
            box-shadow:
                0 0 40px 20px rgba(150, 190, 255, 0.55),
                0 0 120px 70px rgba(150, 190, 255, 0.25);
            will-change: transform;
        }

        .dune {
            position: absolute;
            left: -10%;
            width: 120%;
            border-radius: 50% 50% 0 0;
            will-change: transform;
        }
        .dune.back  { bottom: -8%;  height: 30%; background: #7a3a24; }
        .dune.front { bottom: -14%; height: 26%; background: var(--sand-dark); }

        /* ---------------------------------------------------------------
           Title
           --------------------------------------------------------------- */
        header {
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            text-align: center;
            padding: 5rem 1rem 3rem;
        }
        header h1 {
            margin: 0;
            font-size: clamp(2.2rem, 8vw, 4.5rem);
            letter-spacing: 0.15em;
            text-transform: uppercase;
            color: #fff;
            text-shadow: 0 0 20px rgba(227, 112, 26, 0.7);
        }
        header p {
            margin: 0.5rem 0 2rem;
            color: var(--accent);
            font-size: 1.2rem;
        }

        /* Orange outlined button */
        .btn {
            display: inline-block;
            padding: 0.8rem 1.8rem;
            border: 2px solid var(--accent);
            border-radius: 999px;
            color: #fff;
            text-decoration: none;
            letter-spacing: 0.08em;
            text-transform: uppercase;
            transition: background .2s, transform .2s, box-shadow .2s;
        }
        .btn:hover {
            background: var(--accent);
            transform: translateY(-3px);
            box-shadow: 0 8px 25px rgba(227, 112, 26, 0.5);
        }

        /* ---------------------------------------------------------------
           Content sections (About, Explore, Sensors, Team)
           --------------------------------------------------------------- */
        main {
            max-width: 900px;
            margin: 0 auto;
            padding: 0 1rem 4rem;
        }

        section {
            background: rgba(52, 29, 20, 0.85);
            border: 1px solid rgba(227, 112, 26, 0.4);
            border-radius: 12px;
            padding: 1.5rem 2rem;
            margin-bottom: 30vh;
            backdrop-filter: blur(4px);

            /* hidden until scrolled into view */
            opacity: 0;
            transform: translateY(40px);
            transition: opacity .8s ease, transform .8s ease;
        }
        section.visible {
            opacity: 1;
            transform: none;
        }
        h2 {
            margin-top: 0;
            color: var(--accent);
        }

        /* ---------------------------------------------------------------
           3D model viewer
           --------------------------------------------------------------- */
        #model-viewer {
            width: 100%;
            height: 480px;
            border-radius: 8px;
            background: radial-gradient(circle at 50% 70%, rgba(227, 112, 26, 0.25), rgba(0, 0, 0, 0.45));
            --poster-color: transparent;
        }


        /* ---------------------------------------------------------------
           Sensor cards
           --------------------------------------------------------------- */
        .sensors {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
            gap: 1rem;
        }
        .sensor {
            background: rgba(0, 0, 0, 0.35);
            border: 1px solid transparent;
            border-radius: 8px;
            padding: 1rem;
            text-align: center;
            cursor: default;
            transition: transform .2s, border-color .2s, box-shadow .2s;
        }
        .sensor:hover {
            transform: scale(1.05);
            border-color: var(--accent);
            box-shadow: 0 6px 20px rgba(227, 112, 26, 0.35);
        }
        .sensor.flash { animation: flash .5s; }
        @keyframes flash {
            50% { background: rgba(227, 112, 26, 0.5); }
        }
        .sensor .value  { font-size: 1.8rem; color: #fff; }
        .sensor .label  { font-size: 0.9rem; color: var(--accent); }
        .sensor .detail { font-size: 0.8rem; opacity: .7; height: 1.2em; }

        /* ---------------------------------------------------------------
           Team cards
           --------------------------------------------------------------- */
        .team {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 1rem;
        }
        .member {
            background: rgba(0, 0, 0, 0.35);
            border-radius: 8px;
            padding: 1rem;
            text-align: center;
            transition: transform .2s;
        }
        .member:hover { transform: scale(1.05); }
        .member .avatar { font-size: 2.5rem; }

        /* ---------------------------------------------------------------
           Footer
           --------------------------------------------------------------- */
        footer {
            text-align: center;
            padding: 1.5rem 1rem;
            background: rgba(0, 0, 0, 0.6);
            color: var(--accent);
        }
        footer p { margin: 0; }

    </style>
</head>


<!-- =====================================================================
     PART 3: PAGE CONTENT
     ===================================================================== -->
<body>

    <!-- Scroll progress bar -->
    <div id="progress"></div>

    <!-- Top navigation menu -->
    <nav>
        <a href="#top"     data-section="top">Home</a>
        <a href="#about"   data-section="about">About</a>
        <a href="#explore" data-section="explore">Explore</a>
        <a href="#sensors" data-section="sensors">Sensors</a>
        <a href="#team"    data-section="team">Team</a>
    </nav>

    <!-- Animated sky background (stars, sun, dunes) -->
    <div id="sky">
        <div id="sunset"></div>
        <div id="stars"></div>
        <div id="sun"></div>
        <div class="dune back"></div>
        <div class="dune front"></div>
    </div>


    <!-- ---------------------------------------------------------------
         title
         --------------------------------------------------------------- -->
    <header id="top">
        <h1>Mars Base</h1>
        <p>Our Place In Space</p>
        <a class="btn" href="#explore">Explore the Base</a>
    </header>

    <main>

        <!-- -----------------------------------------------------------
             About
             ----------------------------------------------------------- -->
        <section id="about">
            <h2>Welcome to the Martian home of the future!</h2>
            <p>
                This is an example of what our place in space, being a rightfully claimed plot of land on Mars, would look like. It includes what systems would be present and how they would work.
            </p>
        </section>


        <!-- -----------------------------------------------------------
             Explore: the 3D model of the base
             3D model loaded from our github repository
             ----------------------------------------------------------- -->
        <section id="explore">
            <h2>Explore the Base</h2>

            <!-- The library that makes <model-viewer> work -->
            <script type="module"
                src="https://cdn.jsdelivr.net/npm/@google/model-viewer@3.5.0/dist/model-viewer.min.js"></script>

            <model-viewer id="model-viewer"
                src="https://raw.githubusercontent.com/thecobkid2009-del/Hackathon-2026/main/website%20(Totally%20not%20vibe%20coded)/marsbase.glb"
                alt="3D model of our Mars base"
                camera-controls
                auto-rotate
                auto-rotate-delay="2000"
                camera-orbit="35deg 40deg 30%"
                min-camera-orbit="auto 5deg 30%"
                shadow-intensity="1.2"
                exposure="1.1"
                interaction-prompt="none">
            </model-viewer>

            <!-- Shows "Loading... 40%" or an error message if the model fails -->
            <p id="model-status" class="hint">Loading model…</p>
            <p class="hint">Drag to rotate · scroll to zoom · click the glowing dots</p>
        </section>


        <!-- -----------------------------------------------------------
             Sensors: temperature, humidity, motion, and PIR diagnostics
             ----------------------------------------------------------- -->
        <section id="sensors">
            <h2>Base Sensors</h2>
            <p>Live readings from the habitat sensors.</p>
            <p id="api-status" class="hint" aria-live="polite">Connecting to ESP32…</p>

            <div class="sensors">
                <div class="sensor" data-sensor="temp">
                    <div class="value" id="temp">waiting</div>
                    <div class="label">Temperature (°C)</div>
                </div>

                <div class="sensor" data-sensor="humidity">
                    <div class="value" id="hum">waiting</div>
                    <div class="label">Humidity (%)</div>
                </div>

                <div class="sensor" data-sensor="motion">
                    <div class="value" id="motion">--</div>
                    <div class="label">Alien</div>
                </div>

            </div>
        </section>


        <!-- -----------------------------------------------------------
             Us
             ----------------------------------------------------------- -->
        <section id="team">
            <h2>The Team</h2>

            <div class="team">
                <div class="member"><div class="avatar">;)</div>Jacob Luscombe</div>
                <div class="member"><div class="avatar">>:|</div>Vincent Watson</div>
                <div class="member"><div class="avatar">:)</div>Sumvidh Bharadwaj</div>
                <div class="member"><div class="avatar">:^)</div>Navtej Vishwanath</div>
            </div>
        </section>

    </main>

    <footer>
        <p>&copy; 404 Planet Not Found</p>
    </footer>


    <!-- =================================================================
         PART 4: BEHAVIOUR (JavaScript)
         ================================================================= -->
    <script>

        /* ---------------------------------------------------------------
           Small helpers
           --------------------------------------------------------------- */
        const $ = (selector) => document.querySelector(selector);
        const clamp = (n, min, max) => Math.min(max, Math.max(min, n));


        /* ---------------------------------------------------------------
           1. Create the stars
           --------------------------------------------------------------- */
        const starsEl = $('#stars');

        for (let i = 0; i < 170; i++) {
            const star = document.createElement('div');
            star.className = 'star';

            const size = Math.random() * 2 + 1;   // 1px to 3px
            star.style.width  = size + 'px';
            star.style.height = size + 'px';
            star.style.left = Math.random() * 100 + '%';
            star.style.top  = Math.random() * 100 + '%';
            star.style.animationDelay    = Math.random() * 3 + 's';
            star.style.animationDuration = (2 + Math.random() * 3) + 's';

            starsEl.appendChild(star);
        }

        /* ---------------------------------------------------------------
           3. Scrolling effects
           --------------------------------------------------------------- */
        const sunset    = $('#sunset');
        const sun       = $('#sun');
        const duneBack  = $('.dune.back');
        const duneFront = $('.dune.front');
        const progress  = $('#progress');

        const onScroll = () => {
            // How far down the page are we? 0 = top, 1 = bottom
            const maxScroll = document.documentElement.scrollHeight - innerHeight;
            const p = maxScroll > 0 ? clamp(scrollY / maxScroll, 0, 1) : 0;

            sunset.style.opacity    = p;
            starsEl.style.opacity   = 1 - p * 0.85;
            sun.style.transform     = `translateY(${-p * innerHeight * 0.45}px)`;
            duneBack.style.transform  = `translateY(${-p * 30}px)`;
            duneFront.style.transform = `translateY(${-p * 60}px)`;
            progress.style.width    = (p * 100) + '%';

            // Work out which section is on screen and highlight its nav link
            let current = 'top';
            document.querySelectorAll('header, section').forEach((el) => {
                if (el.getBoundingClientRect().top < innerHeight * 0.5) {
                    current = el.id;
                }
            });
            document.querySelectorAll('nav a').forEach((link) => {
                link.classList.toggle('active', link.dataset.section === current);
            });
        };

        addEventListener('scroll', onScroll, { passive: true });
        addEventListener('resize', onScroll);
        onScroll();   // run once on load


        /* ---------------------------------------------------------------
           4. Sections fade in when you scroll to them
           --------------------------------------------------------------- */
        const fadeObserver = new IntersectionObserver((entries) => {
            entries.forEach((entry) => {
                if (entry.isIntersecting) entry.target.classList.add('visible');
            });
        }, { threshold: 0.15 });

        document.querySelectorAll('section').forEach((s) => fadeObserver.observe(s));


        /* ---------------------------------------------------------------
           5. 3D model loading status
           --------------------------------------------------------------- */
        const model = $('#model-viewer');
        const modelStatus = $('#model-status');

        model.addEventListener('progress', (event) => {
            const percent = Math.round(event.detail.totalProgress * 100);
            if (percent < 100) {
                modelStatus.textContent = 'Loading model… ' + percent + '%';
            }
        });

        model.addEventListener('load', () => {
            modelStatus.textContent = '';   // hide the message once it works
        });

        model.addEventListener('error', (event) => {
            const reason = event.detail?.sourceError?.message || 'unknown error';
            modelStatus.textContent = 'Model failed to load: ' + reason;
            console.error('Model error:', event);
        });

        // If the library itself never loaded, tell the user why
        setTimeout(() => {
            if (!customElements.get('model-viewer')) {
                modelStatus.textContent =
                    'The 3D viewer library did not load. Check this device has internet access.';
            }
        }, 5000);


        /* ---------------------------------------------------------------
           6. Live ESP32 sensor readings from the /data endpoint
           --------------------------------------------------------------- */
        const apiStatus = $('#api-status');

        const update = () => {
            fetch('/data', { cache: 'no-store' })
                .then((response) => {
                    if (!response.ok) throw new Error('HTTP ' + response.status);
                    return response.json();
                })
                .then((data) => {
                    $('#temp').textContent = data.temp == null ? 'waiting' : Number(data.temp).toFixed(0);
                    $('#hum').textContent = data.hum == null ? 'waiting' : Number(data.hum).toFixed(0);
                    $('#motion').textContent = data.motion ? 'Detected' : 'None';
                    $('#raw').textContent = data.raw ?? '--';
                    $('#changes').textContent = data.changes ?? '--';
                    apiStatus.textContent = 'Connected to ESP32';
                })
                .catch(() => {
                    apiStatus.textContent = 'Waiting for sensor data from the ESP32…';
                });
        }

        update();
        setInterval(update, 500);

        /* ---------------------------------------------------------------
           7. Clicking a glowing dot on the model jumps to its sensor
           --------------------------------------------------------------- */
        document.querySelectorAll('.hotspot').forEach((dot) => {
            dot.addEventListener('click', () => {
                const card = document.querySelector(`.sensor[data-sensor="${dot.dataset.target}"]`);
                if (!card) return;

                card.scrollIntoView({ behavior: 'smooth', block: 'center' });

                // Wait for the scroll, then flash the card
                setTimeout(() => {
                    card.classList.remove('flash');
                    void card.offsetWidth;
                    card.classList.add('flash');
                }, 600);
            });
        });

    </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", PAGE);
}

void handleData() {
  String json = "{";
  json += "\"temp\":" + (isnan(temp) ? String("null") : String(temp, 1)) + ",";
  json += "\"hum\":" + (isnan(hum) ? String("null") : String(hum, 1)) + ",";
  json += "\"motion\":" + String(motionState ? "true" : "false") + ",";
  json += "\"raw\":" + String(digitalRead(PIR_PIN)) + ",";
  json += "\"changes\":" + String(pirChanges);
  json += "}";

  server.send(200, "application/json", json);
}

// =====================================================
// 5. Wi-Fi
// =====================================================
void connectWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_TIMEOUT_MS) {
      Serial.println();
      Serial.println("Wi-Fi timeout, retrying...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      start = millis();
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected. IP address: ");
  Serial.println(WiFi.localIP());
}

// =====================================================
// 6. Sensor, LED and buzzer control
// =====================================================
void readPIR() {
  bool raw = (digitalRead(PIR_PIN) == HIGH);

  if (raw != lastRawPIR) {
    lastRawPIR = raw;
    pirChanges++;
    Serial.println(raw ? "PIR changed: HIGH" : "PIR changed: LOW");
  }

  motionState = raw;

  // Green = no motion, red = motion
  digitalWrite(LED_GREEN, motionState ? LOW : HIGH);
  digitalWrite(LED_RED, motionState ? HIGH : LOW);

  // Buzzer sounds while motion is detected
  digitalWrite(BUZZER_PIN, motionState ? HIGH : LOW);
}

void readDHT() {
  static unsigned long lastRead = 0;
  if (millis() - lastRead < SENSOR_INTERVAL_MS) return;
  lastRead = millis();

  temp = dht.readTemperature();
  hum = dht.readHumidity();

  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(" C | Humidity: ");
  Serial.print(hum);
  Serial.print(" % | Motion: ");
  Serial.println(motionState ? "Yes" : "No");
}

// =====================================================
// 7. Setup
// =====================================================
void setup() {
  Serial.begin(9600);

  pinMode(PIR_PIN, INPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  dht.begin();

  Serial.println("PIR warming up, please wait...");
  delay(PIR_WARMUP_MS);
  Serial.println("PIR ready.");

  connectWiFi();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

// =====================================================
// 8. Main loop
// =====================================================
void loop() {
  server.handleClient();
  readPIR();
  readDHT();
}

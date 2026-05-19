/****************************************************
 *                LIBRARIES
 ****************************************************/
#include <WiFi.h>        // WiFi functions for ESP32
#include <WebServer.h>  // Simple HTTP server
#include <Arduino.h>    // Core Arduino definitions

/****************************************************
 *            WIFI ACCESS POINT SETTINGS
 ****************************************************/
const char* ssid = "WifiESP32";        // Name of the WiFi network created by ESP32
const char* password = "123456789";    // Password for the WiFi network

// Create a web server on port 80 (HTTP)
WebServer server(80);

/****************************************************
 *            MOTOR DRIVER PIN DEFINITIONS
 ****************************************************/

// Motor Driver 1 (left side motors)
const int IN1_1 = 12;   // Direction pin
const int IN2_1 = 14;
const int IN3_1 = 27;
const int IN4_1 = 26;
const int ENA_1 = 19;   // PWM enable pin (speed)
const int ENB_1 = 21;

// Motor Driver 2 (right side motors)
const int IN1_2 = 25;
const int IN2_2 = 33;
const int IN3_2 = 32;
const int IN4_2 = 13;
const int ENA_2 = 22;
const int ENB_2 = 23;

/****************************************************
 *            SPEED VARIABLES
 ****************************************************/
int motorSpeed = 255;   // Default motor speed (0–255)

/*
 * Dead zone function:
 * - Motors often do not move at very low PWM values
 * - This function forces a minimum speed (150)
 */
int applyDeadZone(int speed) {
  if (speed == 0) {
    return 0;           // Complete stop
  } 
  else if (speed < 150) {
    return 150;         // Minimum usable speed
  } 
  else {
    return speed;       // Normal speed
  }
}

/****************************************************
 *                    SETUP
 ****************************************************/
void setup() { 
  Serial.begin(115200); // Start serial communication for debugging
 
  /******** Motor pins configuration ********/
  pinMode(IN1_1, OUTPUT); pinMode(IN2_1, OUTPUT);
  pinMode(IN3_1, OUTPUT); pinMode(IN4_1, OUTPUT);
  pinMode(ENA_1, OUTPUT); pinMode(ENB_1, OUTPUT);

  pinMode(IN1_2, OUTPUT); pinMode(IN2_2, OUTPUT);
  pinMode(IN3_2, OUTPUT); pinMode(IN4_2, OUTPUT);
  pinMode(ENA_2, OUTPUT); pinMode(ENB_2, OUTPUT);

  /******** PWM setup ********/
  // Attach PWM channels to enable pins
  // Frequency = 1000 Hz, Resolution = 8 bits (0–255)
  ledcAttach(ENA_1, 1000, 8);
  ledcAttach(ENB_1, 1000, 8);
  ledcAttach(ENA_2, 1000, 8);
  ledcAttach(ENB_2, 1000, 8);

  // Set initial motor speed
  ledcWrite(ENA_1, motorSpeed);
  ledcWrite(ENB_1, motorSpeed);
  ledcWrite(ENA_2, motorSpeed);
  ledcWrite(ENB_2, motorSpeed);
  
  /******** WiFi Access Point ********/
  WiFi.softAP(ssid, password);  // ESP32 creates its own WiFi network

  Serial.println("Access Point started");
  Serial.print("Name: "); Serial.println(ssid);
  Serial.print("Password: "); Serial.println(password);
  Serial.print("IP address: "); Serial.println(WiFi.softAPIP());

  /******** HTTP ROUTES ********/

  // Root page (HTML interface)
  server.on("/", handleRoot);

  // Movement routes
  server.on("/forward", [](){ forward(); server.send(200, "text/plain", "forward"); });
  server.on("/back",    [](){ back();    server.send(200, "text/plain", "back"); });
  server.on("/left",    [](){ left();    server.send(200, "text/plain", "left"); });
  server.on("/right",   [](){ right();   server.send(200, "text/plain", "right"); });
  server.on("/stop",    [](){ stopMotors(); server.send(200, "text/plain", "stop"); });

  /******** SPEED CONTROL ROUTE ********/
  server.on("/speed", [](){
    if (server.hasArg("value")) {

      // Read slider value from URL
      motorSpeed = server.arg("value").toInt();
      motorSpeed = constrain(motorSpeed, 0, 255);

      // Apply dead zone correction
      int realSpeed = applyDeadZone(motorSpeed);

      // Apply speed to all motors
      ledcWrite(ENA_1, realSpeed);
      ledcWrite(ENB_1, realSpeed);
      ledcWrite(ENA_2, realSpeed);
      ledcWrite(ENB_2, realSpeed);

      // Debug info
      Serial.print("Slider: ");
      Serial.print(motorSpeed);
      Serial.print(" | Real PWM: ");
      Serial.println(realSpeed);

      server.send(200, "text/plain", "Speed updated");
    } else {
      server.send(400, "text/plain", "Missing value");
    }
  });

  server.begin(); 
  Serial.println("HTTP server started");
}

/****************************************************
 *                    LOOP
 ****************************************************/
void loop() { 
  // Continuously listen for incoming HTTP requests
  server.handleClient(); 
}

/****************************************************
 *            MOTOR CONTROL FUNCTIONS
 ****************************************************/

// Move forward
void forward() { 
  Serial.println("Button pressed: FORWARD");

  digitalWrite(IN1_1, HIGH); digitalWrite(IN2_1, LOW);
  digitalWrite(IN3_1, HIGH); digitalWrite(IN4_1, LOW);

  digitalWrite(IN1_2, HIGH); digitalWrite(IN2_2, LOW);
  digitalWrite(IN3_2, HIGH); digitalWrite(IN4_2, LOW);
}

// Move backward
void back() { 
  Serial.println("Button pressed: BACKWARD");

  digitalWrite(IN1_1, LOW); digitalWrite(IN2_1, HIGH);
  digitalWrite(IN3_1, LOW); digitalWrite(IN4_1, HIGH);

  digitalWrite(IN1_2, LOW); digitalWrite(IN2_2, HIGH);
  digitalWrite(IN3_2, LOW); digitalWrite(IN4_2, HIGH);
}

// Turn left (left wheels backward, right wheels forward)
void left() { 
  Serial.println("Button pressed: LEFT");

  digitalWrite(IN1_1, LOW); digitalWrite(IN2_1, HIGH);
  digitalWrite(IN3_1, LOW); digitalWrite(IN4_1, HIGH);

  digitalWrite(IN1_2, HIGH); digitalWrite(IN2_2, LOW);
  digitalWrite(IN3_2, HIGH); digitalWrite(IN4_2, LOW);
}

// Turn right (left wheels forward, right wheels backward)
void right() { 
  Serial.println("Button pressed: RIGHT");

  digitalWrite(IN1_1, HIGH); digitalWrite(IN2_1, LOW);
  digitalWrite(IN3_1, HIGH); digitalWrite(IN4_1, LOW);

  digitalWrite(IN1_2, LOW); digitalWrite(IN2_2, HIGH);
  digitalWrite(IN3_2, LOW); digitalWrite(IN4_2, HIGH);
}

// Stop all motors
void stopMotors() { 
  Serial.println("Button pressed: STOP");

  digitalWrite(IN1_1, LOW); digitalWrite(IN2_1, LOW);
  digitalWrite(IN3_1, LOW); digitalWrite(IN4_1, LOW);

  digitalWrite(IN1_2, LOW); digitalWrite(IN2_2, LOW);
  digitalWrite(IN3_2, LOW); digitalWrite(IN4_2, LOW);
}
 
// Serve the main HTML page with buttons 
void handleRoot() { 
/*
* The entire HTML page is stored in a String.
* R"rawliteral(...)" allows multi-line text without escaping quotes or line breaks.
* This page is sent to the browser when the user accesses: http://ESP32_IP/
*/
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <!-- Page title shown in the browser tab -->
    <title>ESP32 Car Control</title>

    <style>
        /* General page styling */
        body {
            font-family: cursive; /* Text font */
            text-align: center; /* Center all text */
            background: #fcfefd; /* Light background color */
            margin: 0;
            padding: 20px;
        }

        /* Main button style */
        button {
            padding: 2em 3em; /* Button size */
            font-size: 1.2em; /* Text size */
            margin: 0;
            border-radius: 12px; /* Rounded corners */
            border: none;
            background: #27d3a9; /* Button color */
            color: white;
            cursor: pointer; /* Hand cursor on hover */
            transition: background-color 0.3s ease;
            min-width: 80px;
            max-width: 150px;
            box-sizing: border-box;
            flex-shrink: 0; /* Prevent shrinking */
            display: flex;
            align-items: center;
            justify-content: center;
        }

            /* Button color when mouse is over it */
            button:hover {
                background: #058672;
            }

        /* Container for all movement buttons */
        .button-grid {
            display: flex;
            flex-direction: column; /* Stack rows vertically */
            align-items: center;
            gap: 0.5em;
            max-width: 600px;
            margin: 0 auto; /* Center horizontally */
        }

        /* Row for forward button */
        .top-row, .bottom-row {
            display: flex;
            justify-content: center;
        }

        /* Row for left, stop, right buttons */
        .middle-row {
            display: flex;
            flex-direction: row;
            justify-content: center;
            gap: 0.5em;
        }

        /* Speed slider styling */
        .slider {
            width: 80%;
            max-width: 400px;
            accent-color: #058672; /* Slider color */
        }

        /* Speed control section */
        .speed-controls {
            margin: 20px;
        }

            /* Speed buttons (reset) */
            .speed-controls button {
                padding: 0.5em 1em;
                font-size: 1em;
                margin: 0.25em;
            }

            /* Center reset button */
            .speed-controls button {
                display: flex;
                margin: 0.5em auto;
            }

        /* Adjust layout for small screens (phones) */
        @media (max-width: 480px) {
            button {
                font-size: 0.8em;
                padding: 1em 1.5em;
            }

            .middle-row {
                gap: 0.25em;
            }

            .button-grid {
                gap: 0.25em;
            }
        }
    </style>
</head>

<body>
    <!-- Page title -->
    <h1>Control Your Car</h1>

    <!-- Speed control section -->
    <div class="speed-controls">
        <!-- Display current speed value -->
        <h3>Speed: <span id="speedVal">255</span></h3>

        <!-- Slider to choose speed (0–255 for ESP32 PWM) -->
        <input type="range" min="0" max="255" value="255" class="slider"
               id="speedSlider" oninput="updateSpeed(this.value)">

        <br>

        <!-- Reset speed to zero -->
        <button onclick="resetSpeed()">Reset</button>
    </div>

    <br><br>

    <!-- Movement control buttons -->
    <div class="button-grid">

        <!-- Forward -->
        <div class="top-row">
            <button onclick="fetch('/forward')">Forward</button>
        </div>

        <!-- Left, Stop, Right -->
        <div class="middle-row">
            <button onclick="fetch('/left')">Left</button>
            <button onclick="fetch('/stop')">Stop</button>
            <button onclick="fetch('/right')">Right</button>
        </div>

        <!-- Backward -->
        <div class="bottom-row">
            <button onclick="fetch('/back')">Backward</button>
        </div>
    </div>

    <script>
    /*
     * Called whenever the speed slider is moved
     * - Updates the speed text on the page
     * - Sends the speed value to the ESP32
     */
    function updateSpeed(val) {
        document.getElementById('speedVal').textContent = val;
        fetch('/speed?value=' + val);
    }

    /*
     * Resets speed to 0
     * - Slider goes to zero
     * - Speed text updates
     * - ESP32 receives speed = 0
     */
    function resetSpeed() {
        document.getElementById('speedSlider').value = 0;
        document.getElementById('speedVal').textContent = 0;
        fetch('/speed?value=0');
    }
    </script>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", page); 
}


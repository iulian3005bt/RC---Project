#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

#define SS_PIN 21   // Pin pentru CS RFID
#define RST_PIN 4
#define SERVO_PIN 2

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_MOSI 22    // Pin MOSI OLED
#define OLED_CLK 14     // Pin CLK OLED
#define OLED_DC 27      // Pin DC OLED
#define OLED_CS 5       // Pin CS OLED
#define OLED_RESET 33

const char* ssid = "Mateiul";
const char* password = "parola12345";

// Setări de login
const String username = "admin";
const String passwordLogin = "1234";

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Creare obiect RFID
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
Servo myServo;  // Creare obiect Servo
WebServer server(80);

bool isOpen = false;
bool loggedIn = false;

void setup() {
  Serial.begin(115200);
  SPI.begin();      // Inițializare SPI
  mfrc522.PCD_Init();  // Inițializare RFID
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Inițializare OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Asteptare RFID");
  display.display();
  myServo.attach(SERVO_PIN);  // Inițializare Servo

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectare la WiFi...");
  }
  Serial.println("Conectat la WiFi");
  Serial.println(WiFi.localIP());

  // Pagina de login
  server.on("/", HTTP_GET, handleLoginPage);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/open", handleOpen);
  server.on("/close", handleClose);
  server.begin();
}

void loop() {
  server.handleClient();

  // Verificare RFID doar daca utilizatorul este logat
  if (loggedIn) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      String content = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        content.concat(String(mfrc522.uid.uidByte[i], HEX));
      }
      content.toUpperCase();

      // Afișează UID-ul citit în Serial Monitor
      Serial.print("UID Citit: ");
      Serial.println(content);

      if (content.substring(1) == "D3 36 C9 F7") {  // UID-ul cardului valid
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Acces permis!");
        display.display();
        openBarrier();
      } else {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Acces interzis!");
        display.display();
        delay(2000);
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Asteptare RFID");
        display.display();
      }
    }
  }
}

void handleLoginPage() {
  if (loggedIn) {
    String html = "<html><body>";
    html += "<h1>Control Bariera</h1>";
    html += "<a href='/open'><button>Deschide Bariera</button></a><br>";
    html += "<a href='/close'><button>Inchide Bariera</button></a>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  } else {
    String html = "<html><body>";
    html += "<h1>Login</h1>";
    html += "<form action='/login' method='POST'>";
    html += "<label for='username'>Username:</label><br>";
    html += "<input type='text' id='username' name='username'><br><br>";
    html += "<label for='password'>Parola:</label><br>";
    html += "<input type='password' id='password' name='password'><br><br>";
    html += "<input type='submit' value='Login'>";
    html += "</form>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  }
}

void handleLogin() {
  String user = server.arg("username");
  String pass = server.arg("password");

  if (user == username && pass == passwordLogin) {
    loggedIn = true;
    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    server.send(401, "text/html", "<html><body><h1>Login Eșuat! Reîncercați.</h1></body></html>");
  }
}

void handleOpen() {
  if (loggedIn) {
    openBarrier();
    server.send(200, "text/plain", "Bariera deschisa");
  } else {
    server.send(403, "text/plain", "Acces interzis!");
  }
}

void handleClose() {
  if (loggedIn) {
    closeBarrier();
    server.send(200, "text/plain", "Bariera inchisa");
  } else {
    server.send(403, "text/plain", "Acces interzis!");
  }
}

void openBarrier() {
  if (!isOpen) {
    Serial.println("Se deschide bariera...");  // Mesaj de debugging
    myServo.write(90);  // Deschide bariera
    isOpen = true;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Bariera deschisa");
    display.display();
    Serial.println("Bariera deschisa");  // Afișează în monitorul serial că bariera s-a ridicat
    delay(2000);  // Afișează "Bariera deschisa" timp de 2 secunde

    // După 5 secunde închide automat bariera
    delay(5000);  // Așteaptă 5 secunde
    closeBarrier();  // Închide bariera automat
  }
}

void closeBarrier() {
  if (isOpen) {
    Serial.println("Se inchide bariera...");  // Mesaj de debugging
    myServo.write(0);  // Închide bariera
    isOpen = false;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Bariera inchisa");
    display.display();
    Serial.println("Bariera inchisa");  // Afișează în monitorul serial că bariera s-a coborât
    delay(2000);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Asteptare RFID");
    display.display();
  }
}

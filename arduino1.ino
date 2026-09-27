ARDUINO 1:
#include <SoftwareSerial.h>
// Communication Links
SoftwareSeriallink(7, 8); // To Slave (Arm)
SoftwareSerialgsm(12, 13); // To GSM Module (TX, RX)
// ===== MOTOR PINS =====
#define IN1 2
#define IN2 3
#define ENA 6
#define IN3 4
#define IN4 5
#define ENB 11
// ===== ULTRASONIC PINS =====
#define TRIG 9
#define ECHO 10
#define DRY_TRIG A0
#define DRY_ECHO A1
#define WET_TRIG A2
#define WET_ECHO A3
#define STOP_DIST 10 // Distance to stop for trash (cm)
#define BIN_FULL_DIST 5 // Distance for full bin (cm)
bool armBusy = false;
bool smsSent = false; // Prevent SMS spam
// ------------------
int readUS(int trig, int echo) {
digitalWrite(trig, LOW); delayMicroseconds(2);
digitalWrite(trig, HIGH); delayMicroseconds(10);
digitalWrite(trig, LOW);
long t = pulseIn(echo, HIGH, 30000);
if (t == 0) return 999; // Return large value if no reading
return t * 0.034 / 2;
}
void moveForward() {
digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
analogWrite(ENA, 180); analogWrite(ENB, 180);
}
void stopMotors() {
digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
analogWrite(ENA, 0); analogWrite(ENB, 0);
}
void sendSMS(String message) {
gsm.println("AT+CMGF=1"); // Set SMS to text mode
delay(1000);
gsm.println("AT+CMGS=\"+1234567890\""); // REPLACE WITH YOUR NUMBER
delay(1000);
gsm.print(message);
delay(100);
gsm.write(26); // ASCII code for CTRL+Z to send
delay(5000);
}
void setup() {
Serial.begin(9600);
link.begin(9600);
gsm.begin(9600);
pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); pinMode(ENA, OUTPUT);
pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENB, OUTPUT);
pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
pinMode(DRY_TRIG, OUTPUT); pinMode(DRY_ECHO, INPUT);
pinMode(WET_TRIG, OUTPUT); pinMode(WET_ECHO, INPUT);
stopMotors();
Serial.println("MASTER READY");
}
void loop() {
// 1. Check for updates from Arm
if (link.available()) {
String msg = link.readStringUntil('\n');
msg.trim();
if (msg == "ARM_BUSY") armBusy = true;
if (msg == "ARM_DONE") armBusy = false;
}
// 2. Read Sensors
int obs = readUS(TRIG, ECHO);
int dryDist = readUS(DRY_TRIG, DRY_ECHO);
int wetDist = readUS(WET_TRIG, WET_ECHO);
bool dryFull = (dryDist<= BIN_FULL_DIST);
bool wetFull = (wetDist<= BIN_FULL_DIST);
// 3. Handle Bin Full (GSM)
if (dryFull || wetFull) {
stopMotors();
if (!smsSent) {
Serial.println("BIN FULL! Sending SMS...");
sendSMS("Alert: Waste bin is full. Please empty.");
smsSent = true;
}
return;
} else {
smsSent = false; // Reset when emptied
}
// 4. Handle Arm Activity
if (armBusy) {
stopMotors();
return;
}
// 5. Navigation Logic
if (obs<= STOP_DIST) {
stopMotors();
Serial.println("OBSTACLE DETECTED - TRIGGERING ARM");
link.println("PICK"); // Only trigger if something is in front
delay(1000); // Debounce trigger
} else {
moveForward();
}
delay(100);
}
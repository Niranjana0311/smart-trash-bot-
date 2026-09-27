
#include <Servo.h>
#include <SoftwareSerial.h>
SoftwareSeriallink(7, 8); // RX, TX to Master
Servo base, shoulder, elbow, gripper, flipper;
// ===== PINS =====
#define BASE_PIN 5
#define SHOULDER_PIN 6
#define ELBOW_PIN 9
#define GRIPPER_PIN 10
#define FLIPPER_PIN 3
#define WET_SENSOR_PIN 4
// ===== POSITIONS =====
#define BASE_HOME 90
#define FLIP_CENTER 110
#define FLIP_LEFT 60 // Dry
#define FLIP_RIGHT 180 // Wet
#define SHOULDER_HOME 90
#define SHOULDER_PICK 20
#define SHOULDER_DROP 90
#define ELBOW_HOME 100
#define ELBOW_PICK 100
#define ELBOW_DROP 0
#define GRIPPER_OPEN 180
#define GRIPPER_CLOSE 20
// ===== STATE TRACKING =====
int sPos = SHOULDER_HOME;
int ePos = ELBOW_HOME;
int fPos = FLIP_CENTER;
// ===== MOVE ARM FUNCTION =====
void moveBoth(int sTarget, int eTarget, int speedDelay = 15) {
while (sPos != sTarget || ePos != eTarget) {
if (sPos<sTarget) sPos++;
else if (sPos>sTarget) sPos--;
if (ePos<eTarget) ePos++;
else if (ePos>eTarget) ePos--;
shoulder.write(sPos);
elbow.write(ePos);
delay(speedDelay);
}
}
// ===== MOVE FLIPPER FUNCTION =====
void moveFlipper(int target, int speedDelay = 10) {
while (fPos != target) {
if (fPos< target) fPos++;
else if (fPos> target) fPos--;
flipper.write(fPos);
delay(speedDelay);
}
}
void setup() {
Serial.begin(9600);
link.begin(9600); // Communicate with Master
base.attach(BASE_PIN);
shoulder.attach(SHOULDER_PIN);
elbow.attach(ELBOW_PIN);
gripper.attach(GRIPPER_PIN);
flipper.attach(FLIPPER_PIN);
pinMode(WET_SENSOR_PIN, INPUT);
// Initialize positions
base.write(BASE_HOME);
shoulder.write(sPos);
elbow.write(ePos);
gripper.write(GRIPPER_OPEN);
flipper.write(fPos);
Serial.println("ARM SLAVE READY");
}
void loop() {
// Check if Master sent a "PICK" command
if (link.available()) {
String cmd = link.readStringUntil('\n');
cmd.trim();
if (cmd == "PICK") {
// 1. Tell Master to stop moving motors
link.println("ARM_BUSY");
Serial.println("Starting Cycle...");
// 2. MOVE TO PICK
moveBoth(SHOULDER_PICK, ELBOW_PICK);
delay(500);
// 3. CLOSE GRIPPER
gripper.write(GRIPPER_CLOSE);
delay(1000);
// 4. MOVE TO DROP
moveBoth(SHOULDER_DROP, ELBOW_DROP);
delay(500);
// 5. OPEN GRIPPER
gripper.write(GRIPPER_OPEN);
delay(1000);
// 6. SORTING
if (digitalRead(WET_SENSOR_PIN) == HIGH) {
moveFlipper(FLIP_RIGHT); // Wet
} else {
moveFlipper(FLIP_LEFT); // Dry
}
delay(1500);
moveFlipper(FLIP_CENTER);
// 7. RETURN HOME
moveBoth(SHOULDER_HOME, ELBOW_HOME);
// 8. Tell Master it can move again
link.println("ARM_DONE");
Serial.println("Cycle Complete.");
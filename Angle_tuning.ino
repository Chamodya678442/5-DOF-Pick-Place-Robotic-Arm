#include <ESP32Servo.h>

// ---------------- Servo objects ----------------
Servo baseServo;
Servo shoulderServo;
Servo elbowServo;
Servo gripperServo;

// ---------------- Pin assignments ---------------
const int BASE_PIN     = 18;
const int SHOULDER_PIN = 19;
const int ELBOW_PIN    = 21;
const int GRIPPER_PIN  = 23;

// Starting positions
int baseAngle     = 90;
int shoulderAngle = 90;
int elbowAngle    = 90;
int gripperAngle  = 90;

// Currently selected servo:
// b = base, s = shoulder, e = elbow, g = gripper
char selectedServo = 'b';

// Change this to 1 for precise calibration
int stepSize = 5;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Standard servo pulse-width range
  baseServo.setPeriodHertz(50);
  shoulderServo.setPeriodHertz(50);
  elbowServo.setPeriodHertz(50);
  gripperServo.setPeriodHertz(50);

  baseServo.attach(BASE_PIN, 500, 2500);
  shoulderServo.attach(SHOULDER_PIN, 500, 2500);
  elbowServo.attach(ELBOW_PIN, 500, 2500);
  gripperServo.attach(GRIPPER_PIN, 500, 2500);

  // Move to starting positions
  baseServo.write(baseAngle);
  shoulderServo.write(shoulderAngle);
  elbowServo.write(elbowAngle);
  gripperServo.write(gripperAngle);

  delay(1500);

  printInstructions();
  printAllAngles();
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();

    if (command.length() == 0) {
      return;
    }

    // Select a servo
    if (command == "b") {
      selectedServo = 'b';
      Serial.println("Selected: BASE");
    }
    else if (command == "s") {
      selectedServo = 's';
      Serial.println("Selected: SHOULDER");
    }
    else if (command == "e") {
      selectedServo = 'e';
      Serial.println("Selected: ELBOW");
    }
    else if (command == "g") {
      selectedServo = 'g';
      Serial.println("Selected: GRIPPER");
    }

    // Increase selected servo
    else if (command == "+") {
      changeSelectedServo(stepSize);
    }

    // Decrease selected servo
    else if (command == "-") {
      changeSelectedServo(-stepSize);
    }

    // Select movement step
    else if (command == "step1") {
      stepSize = 1;
      Serial.println("Step size: 1 degree");
    }
    else if (command == "step5") {
      stepSize = 5;
      Serial.println("Step size: 5 degrees");
    }
    else if (command == "step10") {
      stepSize = 10;
      Serial.println("Step size: 10 degrees");
    }

    // Direct angle commands: b90, s110, e75 or g30
    else if (command.charAt(0) == 'b' ||
             command.charAt(0) == 's' ||
             command.charAt(0) == 'e' ||
             command.charAt(0) == 'g') {

      char servoName = command.charAt(0);
      int requestedAngle = command.substring(1).toInt();

      setServoAngle(servoName, requestedAngle);
    }

    else if (command == "status") {
      printAllAngles();
    }
    else if (command == "help") {
      printInstructions();
    }
    else {
      Serial.println("Unknown command. Type: help");
    }
  }
}

void changeSelectedServo(int amount) {
  int newAngle;

  switch (selectedServo) {
    case 'b':
      newAngle = baseAngle + amount;
      setServoAngle('b', newAngle);
      break;

    case 's':
      newAngle = shoulderAngle + amount;
      setServoAngle('s', newAngle);
      break;

    case 'e':
      newAngle = elbowAngle + amount;
      setServoAngle('e', newAngle);
      break;

    case 'g':
      newAngle = gripperAngle + amount;
      setServoAngle('g', newAngle);
      break;
  }
}

void setServoAngle(char servoName, int requestedAngle) {
  // Absolute software protection
  int safeAngle = constrain(requestedAngle, 0, 180);

  switch (servoName) {
    case 'b':
      baseAngle = safeAngle;
      baseServo.write(baseAngle);
      selectedServo = 'b';

      Serial.print("BASE = ");
      Serial.println(baseAngle);
      break;

    case 's':
      shoulderAngle = safeAngle;
      shoulderServo.write(shoulderAngle);
      selectedServo = 's';

      Serial.print("SHOULDER = ");
      Serial.println(shoulderAngle);
      break;

    case 'e':
      elbowAngle = safeAngle;
      elbowServo.write(elbowAngle);
      selectedServo = 'e';

      Serial.print("ELBOW = ");
      Serial.println(elbowAngle);
      break;

    case 'g':
      gripperAngle = safeAngle;
      gripperServo.write(gripperAngle);
      selectedServo = 'g';

      Serial.print("GRIPPER = ");
      Serial.println(gripperAngle);
      break;
  }
}

void printAllAngles() {
  Serial.println();
  Serial.println("-------- CURRENT ANGLES --------");

  Serial.print("Base:     ");
  Serial.println(baseAngle);

  Serial.print("Shoulder: ");
  Serial.println(shoulderAngle);

  Serial.print("Elbow:    ");
  Serial.println(elbowAngle);

  Serial.print("Gripper:  ");
  Serial.println(gripperAngle);

  Serial.print("Step size: ");
  Serial.println(stepSize);

  Serial.println("--------------------------------");
}

void printInstructions() {
  Serial.println();
  Serial.println("===== ROBOT ARM CALIBRATION =====");
  Serial.println("Set Serial Monitor to:");
  Serial.println("115200 baud and Newline");
  Serial.println();
  Serial.println("Select servo:");
  Serial.println("b  = Base");
  Serial.println("s  = Shoulder");
  Serial.println("e  = Elbow");
  Serial.println("g  = Gripper");
  Serial.println();
  Serial.println("Move selected servo:");
  Serial.println("+  = Increase angle");
  Serial.println("-  = Decrease angle");
  Serial.println();
  Serial.println("Choose movement amount:");
  Serial.println("step1  = 1 degree");
  Serial.println("step5  = 5 degrees");
  Serial.println("step10 = 10 degrees");
  Serial.println();
  Serial.println("Direct commands:");
  Serial.println("b90  = Base to 90");
  Serial.println("s100 = Shoulder to 100");
  Serial.println("e75  = Elbow to 75");
  Serial.println("g30  = Gripper to 30");
  Serial.println();
  Serial.println("status = Display every angle");
  Serial.println("help   = Display instructions");
  Serial.println("=================================");
}

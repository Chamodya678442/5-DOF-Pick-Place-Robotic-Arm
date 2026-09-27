#include <ESP32Servo.h>

// =====================================================
// Servo objects
// =====================================================
Servo baseServo;
Servo shoulderServo;
Servo elbowServo;
Servo gripperServo;

// =====================================================
// ESP32 pin assignments
// =====================================================
const int BASE_PIN     = 18;
const int SHOULDER_PIN = 19;
const int ELBOW_PIN    = 21;
const int GRIPPER_PIN  = 23;
const int IR_PIN       = 22;

// Most IR modules output LOW when an object is detected
const bool IR_ACTIVE_LOW = true;

// =====================================================
// Movement settings
// =====================================================
const int MOVE_DELAY_MS = 20;
const int IR_DEBOUNCE_MS = 80;

const int GRIPPER_START   = 10;
const int GRIPPER_RELEASE = 20;
const int GRIPPER_HOLD    = 60;

// =====================================================
// Position structure
// =====================================================
struct ArmPosition {
  int base;
  int shoulder;
  int elbow;
};

// =====================================================
// Tested arm positions
// =====================================================

// Start/retracted position
const ArmPosition START_POSITION = {
  90, 90, 90
};

// Picking position
const ArmPosition PICK_POSITION = {
  90, 45, 95
};

// Slots in the required order: 1, 2, 3, 4, 5, 6
const ArmPosition SLOT_POSITIONS[6] = {
  {38, 30,  67},   // Slot 1
  {45, 40,  80},   // Slot 2
  {30, 40,  85},   // Slot 3
  {40, 50, 100},   // Slot 4
  {25, 55, 105},   // Slot 5
  {35, 55, 109}    // Slot 6
};

// =====================================================
// Current servo positions
// =====================================================
int currentBase     = 90;
int currentShoulder = 90;
int currentElbow    = 90;
int currentGripper  = GRIPPER_START;

// Next slot to be filled: 0 means Slot 1
int currentSlot = 0;

// Prevents the same object from being detected repeatedly
bool waitingForSensorClear = false;

// =====================================================
// Setup
// =====================================================
void setup() {
  Serial.begin(115200);

  pinMode(IR_PIN, INPUT);

  baseServo.setPeriodHertz(50);
  shoulderServo.setPeriodHertz(50);
  elbowServo.setPeriodHertz(50);
  gripperServo.setPeriodHertz(50);

  baseServo.attach(BASE_PIN, 500, 2500);
  shoulderServo.attach(SHOULDER_PIN, 500, 2500);
  elbowServo.attach(ELBOW_PIN, 500, 2500);
  gripperServo.attach(GRIPPER_PIN, 500, 2500);

  // Start from the supplied start position
  baseServo.write(currentBase);
  shoulderServo.write(currentShoulder);
  elbowServo.write(currentElbow);
  gripperServo.write(currentGripper);

  delay(2000);

  Serial.println();
  Serial.println("==================================");
  Serial.println("6-SLOT PICK AND PLACE ARM READY");
  Serial.println("Waiting for object at IR sensor...");
  Serial.println("==================================");
}

// =====================================================
// Main loop
// =====================================================
void loop() {
  // Stop permanently when all six slots are filled
  if (currentSlot >= 6) {
    Serial.println("All 6 objects have been placed.");
    Serial.println("Process complete.");

    while (true) {
      delay(1000);
    }
  }

  bool objectDetected = readIRSensor();

  // Wait until the previous object has been removed
  // from the sensor before accepting another detection
  if (waitingForSensorClear) {
    if (!objectDetected) {
      delay(IR_DEBOUNCE_MS);

      if (!readIRSensor()) {
        waitingForSensorClear = false;

        Serial.print("Sensor clear. Waiting for object ");
        Serial.println(currentSlot + 1);
      }
    }

    return;
  }

  // Start the process when an object is detected
  if (objectDetected) {
    delay(IR_DEBOUNCE_MS);

    // Confirm that detection is stable
    if (readIRSensor()) {
      Serial.print("Object detected. Filling Slot ");
      Serial.println(currentSlot + 1);

      pickAndPlace(currentSlot);

      currentSlot++;
      waitingForSensorClear = true;

      if (currentSlot < 6) {
        Serial.print("Slot completed. Next slot: ");
        Serial.println(currentSlot + 1);
      }
      else {
        Serial.println("Slot 6 completed.");
      }
    }
  }
}

// =====================================================
// Complete pick-and-place process
// =====================================================
void pickAndPlace(int slotIndex) {
  Serial.println("1. Opening gripper");

  // Start position uses gripper value 10
  moveGripperSmooth(GRIPPER_START);
  delay(500);

  Serial.println("2. Moving to pick position");

  // Base first while arm is retracted
  moveBaseSmooth(PICK_POSITION.base);

  // Then lower shoulder and elbow to the object
  moveShoulderElbowSmooth(
    PICK_POSITION.shoulder,
    PICK_POSITION.elbow
  );

  delay(700);

  Serial.println("3. Closing gripper");

  // Hold the object using value 60
  moveGripperSmooth(GRIPPER_HOLD);
  delay(1000);

  Serial.println("4. Retracting with object");

  // Raise/retract without rotating the base
  moveShoulderElbowSmooth(
    START_POSITION.shoulder,
    START_POSITION.elbow
  );

  delay(500);

  Serial.print("5. Rotating toward Slot ");
  Serial.println(slotIndex + 1);

  // Rotate only after the arm has retracted
  moveBaseSmooth(SLOT_POSITIONS[slotIndex].base);
  delay(400);

  Serial.print("6. Lowering into Slot ");
  Serial.println(slotIndex + 1);

  moveShoulderElbowSmooth(
    SLOT_POSITIONS[slotIndex].shoulder,
    SLOT_POSITIONS[slotIndex].elbow
  );

  delay(700);

  Serial.println("7. Releasing object");

  // Slot gripper value supplied by user
  moveGripperSmooth(GRIPPER_RELEASE);
  delay(1000);

  Serial.println("8. Retracting from slot");

  // Raise the arm before rotating back
  moveShoulderElbowSmooth(
    START_POSITION.shoulder,
    START_POSITION.elbow
  );

  delay(400);

  Serial.println("9. Returning to start position");

  moveBaseSmooth(START_POSITION.base);
  moveGripperSmooth(GRIPPER_START);

  delay(700);

  Serial.print("Object successfully placed in Slot ");
  Serial.println(slotIndex + 1);
}

// =====================================================
// Read IR sensor
// =====================================================
bool readIRSensor() {
  int sensorValue = digitalRead(IR_PIN);

  if (IR_ACTIVE_LOW) {
    return sensorValue == LOW;
  }

  return sensorValue == HIGH;
}

// =====================================================
// Smooth base movement
// =====================================================
void moveBaseSmooth(int targetBase) {
  targetBase = constrain(targetBase, 0, 180);

  while (currentBase != targetBase) {
    if (currentBase < targetBase) {
      currentBase++;
    }
    else {
      currentBase--;
    }

    baseServo.write(currentBase);
    delay(MOVE_DELAY_MS);
  }
}

// =====================================================
// Smooth shoulder and elbow movement together
// =====================================================
void moveShoulderElbowSmooth(
  int targetShoulder,
  int targetElbow
) {
  targetShoulder = constrain(targetShoulder, 0, 180);
  targetElbow = constrain(targetElbow, 0, 180);

  int startShoulder = currentShoulder;
  int startElbow = currentElbow;

  int shoulderDifference =
    abs(targetShoulder - startShoulder);

  int elbowDifference =
    abs(targetElbow - startElbow);

  int totalSteps = max(
    shoulderDifference,
    elbowDifference
  );

  if (totalSteps == 0) {
    return;
  }

  for (int step = 1; step <= totalSteps; step++) {
    float progress = (float)step / totalSteps;

    currentShoulder =
      round(startShoulder +
            (targetShoulder - startShoulder) * progress);

    currentElbow =
      round(startElbow +
            (targetElbow - startElbow) * progress);

    shoulderServo.write(currentShoulder);
    elbowServo.write(currentElbow);

    delay(MOVE_DELAY_MS);
  }

  currentShoulder = targetShoulder;
  currentElbow = targetElbow;

  shoulderServo.write(currentShoulder);
  elbowServo.write(currentElbow);
}

// =====================================================
// Smooth gripper movement
// =====================================================
void moveGripperSmooth(int targetGripper) {
  targetGripper = constrain(targetGripper, 0, 180);

  while (currentGripper != targetGripper) {
    if (currentGripper < targetGripper) {
      currentGripper++;
    }
    else {
      currentGripper--;
    }

    gripperServo.write(currentGripper);
    delay(MOVE_DELAY_MS);
  }
}
Final code for six slots

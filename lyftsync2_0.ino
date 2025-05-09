BLEUart bleuart;
MPU9250_asukiaaa mpu;

#define MOTOR_PIN 7


const float repSensitivity = 1.0;
const float resetLimit = 0.2;
const unsigned long goodRepTime = 1500;


bool currentRep = false;
unsigned long lastRepTime = 0;
float axb = 0, ayb = 0, azb = 0;
int orientation = 1; 

void setup() {
  Serial.begin(115200);
  Wire.begin();

  Bluefruit.begin();
  Bluefruit.setName("FeatherReps");
  bleuart.begin();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.start();
  Serial.println("✅ BLE UART started");

  mpu.setWire(&Wire);
  mpu.beginAccel();
  mpu.beginGyro();
  delay(1000);
  calibrateAccel();
  Serial.print("✅ MPU Calibrated | azb = ");
  Serial.println(azb);

  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
}

void loop() {
  mpu.accelUpdate();
  unsigned long now = millis();

  float az_raw = mpu.accelZ();
  float az = orientation * (az_raw - azb); 

  if (!currentRep && az < -repSensitivity) {
    currentRep = true;
    unsigned long repTime = now - lastRepTime;

    if (lastRepTime > 0) {
      Serial.print("Rep Time: ");
      Serial.println(repTime);

      if (rep_duration >= goodRepTime) {
        bleuart.print("REP_GOOD\n");
        Serial.println("REP_GOOD");
      } else {
        bleuart.print("REP_FAST\n");
        Serial.println("REP_FAST");
      }
    }

    lastRepTime = now;
  }

  if (currentRep && az > -resetLimit) {
    currentRep = false;
  }


  if (bleuart.available()) {
    String cmd = bleuart.readStringUntil('\n');
    cmd.trim();
    Serial.print("Receiving: ");
    Serial.println(cmd);

    if (cmd == "BUZZ_SHORT") buzzShort();
    else if (cmd == "BUZZ_PULSE") buzzPulse();
    else if (cmd == "BUZZ_DOUBLE_LONG") buzzDoubleLong();
    else if (cmd == "BUZZ_LONG") buzzLong();
  }

  // === Debug Output
  Serial.print("az_raw: ");
  Serial.print(az_raw, 3);
  Serial.print(" | az: ");
  Serial.print(az, 3);
  Serial.print(" | currentRep: ");
  Serial.println(currentRep);

  delay(30); 
}

void buzzShort() {
  digitalWrite(MOTOR_PIN, HIGH);
  delay(200);
  digitalWrite(MOTOR_PIN, LOW);
}

void buzzPulse() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(MOTOR_PIN, HIGH);
    delay(100);
    digitalWrite(MOTOR_PIN, LOW);
    delay(100);
  }
}

void buzzDoubleLong() {
  digitalWrite(MOTOR_PIN, HIGH);
  delay(1000);
  digitalWrite(MOTOR_PIN, LOW);
  delay(400);
  digitalWrite(MOTOR_PIN, HIGH);
  delay(1000);
  digitalWrite(MOTOR_PIN, LOW);
}

void buzzLong() {
  digitalWrite(MOTOR_PIN, HIGH);
  delay(600);
  digitalWrite(MOTOR_PIN, LOW);
}

// Accelerometer Calibration
void calibrateAccel() {
  const int samples = 300;
  axb = ayb = azb = 0;

  for (int i = 0; i < samples; i++) {
    mpu.accelUpdate();
    axb += mpu.accelX();
    ayb += mpu.accelY();
    azb += mpu.accelZ();
    delay(5);
  }

  axb /= samples;
  ayb /= samples;
  azb /= samples;

  if (azb < 0) orientation = -1;
  else orientation = 1;
}


// OBD2 Scanner
#include <Arduino.h>
#include <OBD2UART.h>

// SH1106 OLED Display
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#define OLED_RESET_1 4 // SH1106 OLED Display 1
#define OLED_RESET_2 5 // SH1106 OLED Display 2

COBD obd; // OBD2 Scanner

Adafruit_SH1106 display1(OLED_RESET_1); // SH1106 OLED Display 1
Adafruit_SH1106 display2(OLED_RESET_2); // SH1106 OLED Display 2

int rpm, speed, engineLoad;   // rpm, speed, engindload 값 정의
float engineOilTemp, coolantTemp; // 오일 온도, 수온 값 정의
unsigned long lastTime = 0;   // 마지막으로 속도를 읽은 시간
double totalDistance = 0;     // 총 이동 거리
int currentSpeed = 0;         // 현재 속도

// OBD2 Scanner PIDs
const byte definedPIDs[] = {
    0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x1E, 0x1F,
    0x21, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32,
    0x33, 0x3C, 0x3D, 0x3E, 0x3F, 0x42, 0x43, 0x44,
    0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C,
    0x4D, 0x4E, 0x52, 0x59, 0x5B, 0x5C, 0x5D, 0x5E,
    0x61, 0x62, 0x63
};

void setup() {
  Serial.begin(115200);   // 시리얼 포트
  Serial1.begin(115200);  // 블루투스 포트
  Serial2.begin(115200);  // OBD2 포트

  display1.begin(SH1106_I2C_ADDRESS, OLED_RESET_1); // OLED - 1
  display2.begin(SH1106_I2C_ADDRESS, OLED_RESET_2); // OLED - 2
  display1.clearDisplay();
  display2.clearDisplay();

  connectBluetooth();

  Serial.println("System is starting...");
  Serial.println("Bluetooth Device is Ready to Pair");
  Serial.println("OBD-II Adapter is Ready");
}

void loop() {
  if (Serial1.available()) {
    char cmd = Serial1.read(); // 앱에서 값 읽어오기
    switch (cmd) {
      case '1':
        showDashboard();
        break;
      case '2':
        performMonitoring();
        break;
      case '3':
        performDiagnostics();
        break;
      case '4':
        performDiagnostics();
        break;
      case '5':
        showDrivingRecords();
        break;
      case '6':
        connectBluetooth();
        break;
      default:
        Serial.println("Invalid command");
        updateDistance();
        updateOBDDate();
        updateDisplays();
        break;
    }
    updateDistance();
    updateOBDDate();
    updateDisplays();
  }
}

void updateDistance() { // 실시간 거리 계산
  unsigned long currentTime = millis();
  if (currentTime - lastTime >= 1000) {  // 매 초마다 속도를 읽습니다 (1000ms)
    if (obd.readPID(PID_SPEED, currentSpeed)) {
      double distanceMoved = (currentSpeed * 1000.0 / 3600.0); // m/s로 변환
      totalDistance += distanceMoved * ((currentTime - lastTime) / 1000.0); // 거리 = 속도 * 시간
    }
    lastTime = currentTime;
    Serial.print("Current Speed: "); Serial.println(currentSpeed);
    Serial.print("Total Distance: "); Serial.println(totalDistance, 2); // 소수점 두 자리까지 표시
  }
}

double getTotalDistance() { // 실시간 거리 계산 총합
  return totalDistance / 1000.0;
}

void updateOBDData() {  // Read OBD-II PIDs
  obd.readPID(PID_RPM, rpm);
  obd.readPID(PID_SPEED, speed);
  obd.readPID(PID_ENGINE_LOAD, engineLoad);
  obd.readPID(PID_ENGINE_OIL_TEMP, engineOilTemp);
  obd.readPID(PID_COOLANT_TEMP, coolantTemp);
}

void updateDisplays() { // Update first OLED display
  display1.clearDisplay();
  display1.setTextSize(1);
  display1.setTextColor(WHITE);
  
  display1.setCursor(0, 0);
  display1.print("RPM: "); display1.println(rpm);
  display1.print("Speed: "); display1.println(speed);
  display1.print("Load: "); display1.println(engineLoad);
  
  display1.display();
  
  // Update second OLED display
  display2.clearDisplay();
  display2.setTextSize(1);
  display2.setTextColor(WHITE);
  
  display2.setCursor(0, 0);
  display2.print("Load: "); display2.println(engineLoad);
  display2.print("Oil Temp: "); display2.println(engineOilTemp, 1);
  display2.print("Coolant Temp: "); display2.println(coolantTemp, 1);
  
  display2.display();
}

void showDashboard() { // 대시보드
  Serial.println("Dashboard view activated");
  int speed, rpm, coolantTemp, intakeAirTemp, throttlePos, fuelRate, mafRate, fuelLevel;

  if (obd.readPID(PID_SPEED, speed)) {
    Serial.print("Speed: "); Serial.println(speed);
  }
  if (obd.readPID(PID_RPM, rpm)) {
    Serial.print("RPM: "); Serial.println(rpm);
  }
  if (obd.readPID(PID_COOLANT_TEMP, coolantTemp)) {
    Serial.print("Coolant Temp: "); Serial.println(coolantTemp);
  }
  if (obd.readPID(PID_INTAKE_TEMP, intakeAirTemp)) {
    Serial.print("Intake Air Temp: "); Serial.println(intakeAirTemp);
  }
  if (obd.readPID(PID_THROTTLE, throttlePos)) {
    Serial.print("Throttle Position: "); Serial.println(throttlePos);
  }
  if (obd.readPID(PID_ENGINE_FUEL_RATE, fuelRate)) {
    Serial.print("Fuel Consumption Rate: "); Serial.println(fuelRate);
  }
  if (obd.readPID(PID_MAF_FLOW, mafRate)) {
    Serial.print("MAF Rate: "); Serial.println(mafRate);
  }
  if (obd.readPID(PID_FUEL_LEVEL, fuelLevel)) {
    Serial.print("Fuel Level: "); Serial.println(fuelLevel);
  }
}

void performMonitoring() { //모니터링
  Serial.println("Monitoring started");
  int result;
  for (byte i = 0; i < sizeof(definedPIDs) / sizeof(definedPIDs[0]); i++) {
    if (obd.readPID(definedPIDs[i], result)) {
      Serial.print("PID ");
      Serial.print(definedPIDs[i], HEX);
      Serial.print(": ");
      Serial.println(result);
    }
  }
}

void performDiagnostics() { // 차량 진단
    uint16_t codes[10];
    byte count = obd.readDTC(codes, 10);
    if (count > 0) {
        Serial.println("Diagnostic Trouble Codes (DTCs):");
        for (byte i = 0; i < count; i++) {
            Serial.print("DTC ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.println(codes[i], HEX);
        }
    } else {
        Serial.println("No Diagnostic Trouble Codes (DTCs) found.");
    }
}

void performDiagnostics_clear() { // 진단 코드 삭제
    obd.clearDTC();
    Serial.println("Diagnostic Trouble Codes (DTC) cleared.");
}

void showDrivingRecords() { //주행 시간 및 주행 거리
  Serial.println("Driving records shown");
  int runtime, distance;
  if (obd.readPID(PID_RUNTIME, runtime)) {
    Serial.print("Engine Runtime: "); Serial.println(runtime);
  }
   Serial.print("Total Distance (km): "); Serial.println(getTotalDistance(), 2); // km 단위로 반환
}

void connectBluetooth() { //블루투스 연결
  Serial.println("Checking Bluetooth connection...");

  // Send an AT command to check connection status
  Serial.println("AT");
  delay(100); // Give some time for the module to respond

  String response = "";
  while (Serial1.available()) {
    char c = Serial1.read();  // Read the response from the Bluetooth module
    response += c;
  }

  if (response.indexOf("OK") != -1) {
    // Response "OK" means the Bluetooth module is responsive, check if connected
    Serial.println("AT+CONNL"); // This is a hypothetical command; replace with your module's actual command to check connection status
    delay(100);

    response = "";
    while (Serial1.available()) {
      char c = Serial1.read();
      response += c;
    }

    if (response.indexOf("1") != -1) { // Assuming "1" means connected
      Serial.println("Bluetooth is already connected.");
    } else {
      Serial.println("Bluetooth is not connected. Attempting to connect...");

      // Attempt to connect; replace "AT+PAIR" with your actual command
      Serial.println("AT+PAIR");
      delay(2000); // Wait for the pairing to complete

      // Check if the pairing was successful
      Serial.println("AT+CONNL"); // Check connection status again
      delay(100);

      response = "";
      while (Serial1.available()) {
        char c = Serial1.read();
        response += c;
      }

      if (response.indexOf("1") != -1) {
        Serial.println("Bluetooth connection established.");
      } else {
        Serial.println("Failed to connect via Bluetooth.");
      }
    }
  } else {
    Serial.println("Failed to communicate with Bluetooth module.");
  }
}
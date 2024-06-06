#include <OBD2UART.h> // OBD-II UART 헤더파일
#include "U8glib.h" // SH1106 OLED Display 헤더파일
COBD obd;
bool hasMEMS;

#define OBDSerial Serial1 // Serial1 -> OBDSerial 재정의
#define BTSerial  Serial2 // Serial2 -> BTSerial 재정의
U8GLIB_SH1106_128X64 u8g1(25, 23, 27, 24);	// SCL SDA CS DC 핀 번호
U8GLIB_SH1106_128X64 u8g2(25, 23, 26, 24);  // SCL SDA CS DC 핀 번호

static const byte pids1[] = {PID_ENGINE_LOAD,PID_COOLANT_TEMP,PID_SHORT_TERM_FUEL_TRIM_1,PID_LONG_TERM_FUEL_TRIM_1, // PID 값 8개
                            PID_SHORT_TERM_FUEL_TRIM_2,PID_LONG_TERM_FUEL_TRIM_2,PID_INTAKE_MAP,PID_RPM};

static const byte pids2[] = {PID_SPEED,PID_TIMING_ADVANCE,PID_INTAKE_TEMP,PID_THROTTLE, // PID 값 8개
                            PID_RUNTIME,PID_DISTANCE_WITH_MIL,PID_COMMANDED_EVAPORATIVE_PURGE,PID_FUEL_LEVEL};

static const byte pids3[] = {PID_WARMS_UPS,PID_DISTANCE,PID_EVAP_SYS_VAPOR_PRESSURE,PID_BAROMETRIC, // PID 값 8개
                            PID_CATALYST_TEMP_B1S1,PID_CATALYST_TEMP_B2S1,PID_CONTROL_MODULE_VOLTAGE,PID_ABSOLUTE_ENGINE_LOAD};

static const byte pids4[] = {PID_AIR_FUEL_EQUIV_RATIO,PID_RELATIVE_THROTTLE_POS,PID_AMBIENT_TEMP,PID_ABSOLUTE_THROTTLE_POS_B,
                            PID_ACC_PEDAL_POS_D,PID_ACC_PEDAL_POS_E,PID_COMMANDED_THROTTLE_ACTUATOR};  // PID 값 7개

const byte* pids[] = {pids1, pids2, pids3, pids4}; // 2차원 배열 생성
const int sizes[] = {sizeof(pids1), sizeof(pids2), sizeof(pids3), sizeof(pids4)}; // 배열 크기의 배열 생성
unsigned long lastTime = 0;   // 마지막으로 속도를 읽은 시간
int distance, runtime, averageSpeed = 0; // 거리, 런타임, 평균속도 초기화
int numScreens = 4; // 화면 갯수
bool screenActive[4] = {false, false, false, false}; // 화면 지정 초기화

void testATcommands() { // OBD-II to Vehicle Test AT Commands
  static const char cmds[][6] = {"ATZ\r", "ATI\r", "ATH0\r", "ATRV\r", "0100\r", "010C\r", "0902\r"};
  char buf[128];

  for (byte i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
    const char *cmd = cmds[i];
    Serial.print("Sending ");
    Serial.println(cmd);
    if (obd.sendCommand(cmd, buf, sizeof(buf))) {
      char *p = strstr(buf, cmd);
      if (p)
        p += strlen(cmd);
      else {
        p = buf;
      }
      while (*p == '\r') p++;
      while (*p) {
        Serial.write(*p);
        if (*p == '\r' && *(p + 1) != '\r')
          Serial.write('\n');
        p++;
      }
      Serial.println();
    }
    else {
      Serial.println("Timeout");
    }
    delay(1000);
  }
  Serial.println();
}

void Display_1() { // OLED Display_1 [RPM, 속도, 이동거리, 런타임]
  static const byte pids[] = {PID_RPM, PID_SPEED, PID_DISTANCE, PID_RUNTIME};
  int values[sizeof(pids)];
  if (obd.readPID(pids, sizeof(pids), values) == sizeof(pids)) {
    u8g1.firstPage();
    do {
      u8g1.setPrintPos(0, 15); u8g1.print("Rpm: "); u8g1.print(values[0]); u8g1.print("'C");
      u8g1.setPrintPos(0, 30); u8g1.print("Speed: "); u8g1.print(values[1]); u8g1.print("'C");
      u8g1.setPrintPos(0, 45); u8g1.print("Distance: "); u8g1.print(values[2]); u8g1.print("%");
      int h = values[3] / 3600;
      int m = (values[3] % 3600) / 60;
      int s = (values[3] % 3600) % 60;
      u8g1.setPrintPos(0, 60); u8g1.print("Runtime: "); u8g1.print(h); u8g1.print(":"); u8g1.print(m); u8g1.print(":"); u8g1.print(s);
    } while (u8g1.nextPage());
  }
}

void Display_2() { // OLED Display_2 [냉각수 온도, 흡기 온도, 엔진 로드율, 스로틀 개도량]
  static const byte pids[] = {PID_COOLANT_TEMP, PID_INTAKE_TEMP, PID_ENGINE_LOAD, PID_THROTTLE};
  int values[sizeof(pids)];
  if (obd.readPID(pids, sizeof(pids), values) == sizeof(pids)) {
    u8g2.firstPage();
    do {
      u8g2.setPrintPos(0, 15); u8g2.print("Coolant: "); u8g2.print(values[0]); u8g2.print("'C");
      u8g2.setPrintPos(0, 30); u8g2.print("Intake: "); u8g2.print(values[1]); u8g2.print("'C");
      u8g2.setPrintPos(0, 45); u8g2.print("Load: "); u8g2.print(values[2]); u8g2.print("%");
      u8g2.setPrintPos(0, 60); u8g2.print("Throttle: "); u8g2.print(values[3]); u8g2.print("%");
    } while (u8g2.nextPage());
  }
}

void showDashboard() { // App 모니터링 관련 8가지 데이터
  static const byte pids[] = {PID_RPM, PID_SPEED, PID_COOLANT_TEMP, PID_INTAKE_TEMP, PID_ENGINE_LOAD, PID_THROTTLE, PID_INTAKE_MAP, PID_AIR_FUEL_EQUIV_RATIO};
    int values[sizeof(pids)];
    if (obd.readPID(pids, sizeof(pids), values) == sizeof(pids)) {
      Serial.print('[');
      Serial.print(millis());
      Serial.print(']');
      for (byte i = 0; i < sizeof(pids) ; i++) {
        Serial.print((int)pids[i] | 0x100, HEX);
        Serial.print('=');
        Serial.print(values[i]);
        Serial.print(' ');
       }
       Serial.println();
    }

}

void performMonitoring() { // App 통합 모니터링 관련 모든 데이터
  for (int i = 0; i < 4; i++) {
    int numPids = sizes[i] / sizeof(pids[i][0]); // 배열의 실제 요소 수를 계산
    int values[numPids];
    
    if (obd.readPID(pids[i], numPids, values) == numPids) {
      Serial.print('[');
      Serial.print(millis());
      Serial.print(']');
      for (int j = 0; j < numPids; j++) {
        Serial.print((int)pids[i][j] | 0x100, HEX);
        Serial.print('=');
        Serial.print(values[j]);
        Serial.print(' ');
      }
      Serial.println();
    }
  }
}

void readBatteryVoltage() { // 배터리 전압 (사용 안하는 중)
  Serial.print('[');
  Serial.print(millis());
  Serial.print(']');
  Serial.print("Battery:");
  Serial.print(obd.getVoltage(), 1);
  Serial.println('V');
}

void connectBluetooth() { //블루투스 연결 (앱 개발 완료 후 수정)
  Serial.println("Checking Bluetooth connection...");

  // Send an AT command to check connection status
  Serial.println("AT");
  delay(100); // Give some time for the module to respond

  String response = "";
  while (BTSerial.available()) {
    char c = BTSerial.read();  // Read the response from the Bluetooth module
    response += c;
  }

  if (response.indexOf("OK") != -1) {
    // Response "OK" means the Bluetooth module is responsive, check if connected
    Serial.println("AT+CONNL"); // This is a hypothetical command; replace with your module's actual command to check connection status
    delay(100);

    response = "";
    while (BTSerial.available()) {
      char c = BTSerial.read();
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
      while (BTSerial.available()) {
        char c = BTSerial.read();
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

void showDrivingRecords() { // 평균속도, 이동거리 데이터
  float averageSpeed = obd.getAverageSpeed();
  float drivingScore = obd.getDrivingScore();
  int distance;
  obd.readPID(PID_DISTANCE, distance);
  Serial.print("Current Speed: "); Serial.print(averageSpeed, 1); Serial.println("km/h");// 평균 속도
  Serial.print("Total Distance: "); Serial.print(distance); Serial.println("km");// 이동거리
  Serial.print("DrivingScore: "); Serial.print(drivingScore, 0);
  
}

void performDiagnostics() { // 차량 진단
  int speed, rpm;
  obd.readPID(PID_SPEED, speed);
  obd.readPID(PID_RPM, rpm);
  if (speed == 0 && rpm <= 200) { // 차량이 시동이 걸려있지 않는 상황 (예시)
    u8g1.firstPage();
    do {
      u8g1.setPrintPos(0, 15); u8g1.print("Diagnositc");
      u8g1.setPrintPos(0, 30); u8g1.print("Trouble Codes");
      u8g1.setPrintPos(0, 45); u8g1.print("Checking...");
      u8g1.setPrintPos(0, 60); u8g1.print("Loading...");
    } while(u8g1.nextPage());
    u8g2.firstPage();
    do {
      u8g2.setPrintPos(0, 15); u8g2.print("DTC");
      u8g2.setPrintPos(0, 30); u8g2.print("Freematics");
      u8g2.setPrintPos(0, 45); u8g2.print("OBD-II");
      u8g2.setPrintPos(0, 60); u8g2.print("Adapter");
    } while(u8g2.nextPage());
    uint16_t codes[6];
    byte dtcCount = obd.readDTC(codes, 6);
    if (dtcCount == 0) {
      Serial.println("No Diagnostic Trouble Codes (DTCs) found.");
      u8g1.firstPage();
      do {
        u8g1.setPrintPos(0, 15); u8g1.print("Diagnositc");
        u8g1.setPrintPos(0, 30); u8g1.print("Trouble Codes");
        u8g1.setPrintPos(0, 45); u8g1.print("0 code found");
        u8g1.setPrintPos(0, 60); u8g1.print("Complete");
      } while(u8g1.nextPage());
      u8g2.firstPage();
      do {
        u8g2.setPrintPos(0, 15); u8g2.print("No");
        u8g2.setPrintPos(0, 30); u8g2.print("Diagnostic");
        u8g2.setPrintPos(0, 45); u8g2.print("Trouble Codes");
        u8g2.setPrintPos(0, 60); u8g2.print("found");
      } while(u8g2.nextPage());
    }
    else {
      Serial.print(dtcCount); 
      Serial.print(" DTC:");
      u8g1.firstPage();
      do {
        u8g1.setPrintPos(0, 15); u8g1.print("Diagnositc");
        u8g1.setPrintPos(0, 30); u8g1.print("Trouble Codes");
        u8g1.setPrintPos(0, 45); u8g1.print(dtcCount); u8g1.print(" code found");
        u8g1.setPrintPos(0, 60); u8g1.print("Complete");
      } while(u8g1.nextPage());
      for (byte n = 0; n < dtcCount; n++) {
        Serial.print(' ');
        Serial.print(codes[n], HEX);
      }
      Serial.println();
    }
    delay(10000); // Pause for 10 sec after successful fetch of DTC codes.
  }
  else {
    Serial.println("Vehicle is Running!!! Can't Running DTC");
    u8g1.firstPage();
    do {
      u8g1.setPrintPos(0, 15); u8g1.print("Vehicle");
      u8g1.setPrintPos(0, 30); u8g1.print("is Running!");
      u8g1.setPrintPos(0, 45); u8g1.print("Can't Run");
      u8g1.setPrintPos(0, 60); u8g1.print("DTC");
    } while(u8g1.nextPage());
    u8g2.firstPage();
    do {
      u8g2.setPrintPos(0, 15); u8g2.print("Turn off");
      u8g2.setPrintPos(0, 30); u8g2.print("Vehicle");
      u8g2.setPrintPos(0, 45); u8g2.print("Only");
      u8g2.setPrintPos(0, 60); u8g2.print("ACC");
    } while(u8g2.nextPage());
  }
}

void performDiagnostics_clear() { // 진단 코드 삭제
    obd.clearDTC();
    Serial.println("Diagnostic Trouble Codes (DTC) cleared.");
    u8g1.firstPage();
    do {
      u8g1.setPrintPos(0, 15); u8g1.print("Diagnositc");
      u8g1.setPrintPos(0, 30); u8g1.print("Trouble Codes");
      u8g1.setPrintPos(0, 45); u8g1.print("Clearing...");
      u8g1.setPrintPos(0, 60); u8g1.print("Loading...");
    } while(u8g1.nextPage());
    u8g2.firstPage();
    do {
      u8g2.setPrintPos(0, 15); u8g2.print("DTC");
      u8g2.setPrintPos(0, 30); u8g2.print("TC");
      u8g2.setPrintPos(0, 45); u8g2.print("CLN");
      u8g2.setPrintPos(0, 60); u8g2.print("LOAD");
    } while(u8g2.nextPage());
    delay(10000); // Pause for 10 sec after successful fetch of DTC codes.
}

void select_menu() { // App 버튼 별 데이터 출력 전환
  for (int i = 0; i < numScreens; i++) {
    if (screenActive[i]) {
      switch (i + 1) {
        case 1:
          showDashboard();
          break;
        case 2:
          performMonitoring();
          break;
        case 3:
          showDrivingRecords();
          break;
        case 4:
          connectBluetooth();
          break;
        default:
          break;
      }
    }
  }
}

void setup() {
  u8g1.setFont(u8g_font_unifont); // Choose a suitable font
  u8g2.setFont(u8g_font_unifont); // Choose a suitable font
  
  u8g1.firstPage();
  do {
    u8g1.setPrintPos(0, 15); u8g1.print("SH1106");
    u8g1.setPrintPos(0, 30); u8g1.print("128 x 64");
    u8g1.setPrintPos(0, 45); u8g1.print("OLED");
    u8g1.setPrintPos(0, 60); u8g1.print("Display");
  } while(u8g1.nextPage());
  u8g2.firstPage();
  do {
    u8g2.setPrintPos(0, 15); u8g2.print("Arduino");
    u8g2.setPrintPos(0, 30); u8g2.print("Freematics");
    u8g2.setPrintPos(0, 45); u8g2.print("OBD-II");
    u8g2.setPrintPos(0, 60); u8g2.print("Adapter");
  } while(u8g2.nextPage());
  delay(5000);

  Serial.begin(115200);
  OBDSerial.begin(115200);
  BTSerial.begin(115200);
  while (!Serial);
  
  for (;;) {
    delay(1000);
    byte version = obd.begin();
    Serial.print("Freematics OBD-II Adapter ");
    if (version > 0) {
      Serial.println("detected");
      Serial.print("OBD firmware version ");
      Serial.print(version / 10);
      Serial.print('.');
      Serial.println(version % 10);
      u8g1.firstPage();
      do {
        u8g1.setPrintPos(0, 20); u8g1.print("Freematics");
        u8g1.setPrintPos(0, 40); u8g1.print("OBD-II Adapter");
      } while(u8g1.nextPage());
      u8g2.firstPage();
      do {
        u8g2.setPrintPos(0, 20); u8g2.print("OBD-II Firmware");
        u8g2.setPrintPos(0, 40); u8g2.print("Version");
        u8g2.setPrintPos(0, 60); u8g2.print(version / 10); u8g2.print("."); u8g2.print(version % 10);
      } while(u8g2.nextPage());
      break;
    }
    else {
      Serial.println("not detected");
      u8g1.firstPage();
      do {
        u8g1.setPrintPos(0, 20); u8g1.print("Freematics");
        u8g1.setPrintPos(0, 40); u8g1.print("OBD-II Adapter");
        u8g1.setPrintPos(0, 60); u8g1.print("NOT DETECTED");
      } while(u8g1.nextPage());
    }
  }
  // send some commands for testing and show response for debugging purpose
  testATcommands();

  // initialize OBD-II adapter
  do {
    Serial.println("Connecting...");
    u8g1.firstPage();
    do {
      u8g1.setPrintPos(0, 20); u8g1.print("Freematics");
      u8g1.setPrintPos(0, 40); u8g1.print("OBD-II Adapter");
      u8g1.setPrintPos(0, 60); u8g1.print("Connecting...");
    } while(u8g1.nextPage());
  } while (!obd.init());
  Serial.println("OBD connected!");
  obd.startMonitoring();
  u8g1.firstPage();
  do {
    u8g1.setPrintPos(0, 20); u8g1.print("Freematics");
    u8g1.setPrintPos(0, 40); u8g1.print("OBD-II Adapter");
    u8g1.setPrintPos(0, 60); u8g1.print("CONNECTED!!!");
  } while(u8g1.nextPage());

  char buf[64];
  if (obd.getVIN(buf, sizeof(buf))) {
    Serial.print("VIN:");
    Serial.println(buf);
    u8g2.firstPage();
    do {
      u8g2.setPrintPos(0, 15); u8g2.print("Vehicle");
      u8g2.setPrintPos(0, 30); u8g2.print("Identification");
      u8g2.setPrintPos(0, 45); u8g2.print("Number");
      u8g2.setPrintPos(0, 60); u8g2.print(buf);
    } while(u8g2.nextPage());
  }
  delay(10000);
}

void loop() {
  Display_1(); // SH1106 OLED Display_1
  Display_2(); // SH1106 OLED Display_2
  if (BTSerial.available()) {       // Bluetooth 데이터 값 유무 확인
    char received = BTSerial.read(); // 데이터 없는 경우 -1 할당
    int screenId = received - '0'; // 전송받은 문자를 숫자로 변환

    if (screenId >= 1 && screenId <= numScreens) {
      screenActive[screenId - 1] = true; // 화면 활성화
      select_menu();
    }
    else {
      switch (screenId) {
        case 5:
          performDiagnostics();
          break;
        case 6:
          performDiagnostics_clear();
          break;
        default:
          break;
      }
    }
  }
  Display_1();
  Display_2();
}


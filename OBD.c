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
int numScreens = 3; // 화면 갯수
int screenId;
int statePin = 33;
bool screenActive[4] = {false, false, false, false}; // 화면 지정 초기화

void testATcommands() { // OBD-II to Vehicle Test AT Commands
  static const char cmds[][6] = {"ATZ\r", "ATI\r", "ATH0\r", "ATRV\r", "0100\r", "010C\r", "0902\r"};
  char buf[128];

  for (byte i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
    const char *cmd = cmds[i];
    Serial.print("Sending ");
    Serial.println(cmd);
    u8g2.firstPage();
    do {
      u8g2.setPrintPos(0, 20); u8g2.print("Sending "); u8g2.print(cmd);
    } while(u8g2.nextPage());
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

void Display_1() { // OLED Display_1 [RPM, 속도, 전압, 런타임]
  static const byte pids[] = {PID_RPM, PID_SPEED, PID_RUNTIME};
  int values[sizeof(pids)];
  if (obd.readPID(pids, sizeof(pids), values) == sizeof(pids)) {
    int h = values[3] / 3600;
    int m = (values[3] % 3600) / 60;
    int s = (values[3] % 3600) % 60;

    u8g1.firstPage();
    do {
      u8g1.setPrintPos(0, 15); u8g1.print("RPM "); u8g1.setPrintPos(64,15); u8g1.print("SPEED");
      u8g1.setPrintPos(0, 30); u8g1.print(values[0]); u8g1.setPrintPos(64,30); u8g1.print(values[1]); u8g1.print("km/h");
      u8g1.setPrintPos(0, 45); u8g1.print("Voltage"); u8g1.setPrintPos(64, 45); u8g1.print("Running");
      u8g1.setPrintPos(0, 60); u8g1.print(obd.getVoltage(), 1); u8g1.print(" V"); u8g1.setPrintPos(64, 60); u8g1.print(values[2]);
    } while (u8g1.nextPage());
  }
}

void Display_2() { // OLED Display_2 [냉각수 온도, 흡기 온도, 엔진 로드율, 스로틀 개도량]
  static const byte pids[] = {PID_COOLANT_TEMP, PID_INTAKE_TEMP, PID_ENGINE_LOAD, PID_THROTTLE};
  int values[sizeof(pids)];
  if (obd.readPID(pids, sizeof(pids), values) == sizeof(pids)) {
    u8g2.firstPage();
    do {
      u8g2.setPrintPos(0, 15); u8g2.print("Coolant"); u8g2.setPrintPos(64,15); u8g2.print("Intake");
      u8g2.setPrintPos(0, 30); u8g2.print(values[0]);u8g2.print("'C"); u8g2.setPrintPos(64,30); u8g2.print(values[1]); u8g2.print("'C");
      u8g2.setPrintPos(0, 45); u8g2.print("Engine"); u8g2.setPrintPos(64,45); u8g2.print("Throttle");
      u8g2.setPrintPos(0, 60); u8g2.print(values[2]); u8g2.print("%"); u8g2.setPrintPos(64,60); u8g2.print(values[3]); u8g2.print("%");
    } while (u8g2.nextPage());
  }
}

void showDashboard() { // App 모니터링 관련 8가지 데이터
  static const byte pids[] = {PID_RPM, PID_SPEED, PID_COOLANT_TEMP, PID_INTAKE_TEMP, PID_ENGINE_LOAD, PID_THROTTLE, PID_INTAKE_MAP, PID_AIR_FUEL_EQUIV_RATIO};
    int values[sizeof(pids)];
    String output = "";
    if (obd.readPID(pids, sizeof(pids), values) == sizeof(pids)) {
      for (int i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        output += String(values[i]);
        if (i < sizeof(values) / sizeof(values[0]) - 1) {
          output += ",";
        }
      }
      BTSerial.println(output);
    }
}

void performMonitoring() { // App 통합 모니터링 관련 모든 데이터
  String output = "";
  for (int i = 0; i < 4; i++) {
    int numPids = sizes[i] / sizeof(pids[i][0]);
    int values[numPids];
    if (obd.readPID(pids[i], numPids, values) == numPids) {
       for (int j = 0; j < numPids; j++) {
        output += String(values[j]);
        if (j < numPids - 1 || i < 3) {
          output += ",";
        }
      }
    }
  }
  output += String(obd.getVoltage(), 1);
  BTSerial.println(output);
}

void showDrivingRecords() { // 평균속도, 이동거리, 주행점수 데이터
  float averageSpeed = obd.getAverageSpeed();
  float drivingScore = obd.getDrivingScore();
  int distance;
  obd.readPID(PID_DISTANCE, distance);
  BTSerial.print(averageSpeed, 1); BTSerial.print(","); BTSerial.print(distance); BTSerial.print(","); BTSerial.println(drivingScore, 0);
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
      BTSerial.print(dtcCount);
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
      BTSerial.print(dtcCount);
      u8g1.firstPage();
      do {
        u8g1.setPrintPos(0, 15); u8g1.print("Diagnositc");
        u8g1.setPrintPos(0, 30); u8g1.print("Trouble Codes");
        u8g1.setPrintPos(0, 45); u8g1.print(dtcCount); u8g1.print(" code found");
        u8g1.setPrintPos(0, 60); u8g1.print("Complete");
      } while(u8g1.nextPage());
      for (byte n = 0; n < dtcCount; n++) {
        BTSerial.print(codes[n], HEX);
      }
    }
    delay(7000); // Pause for 10 sec after successful fetch of DTC codes.
  }
  else {
    BTSerial.print("666"); // Vehicle Running Error Code "666"
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
    delay(5000);
  }
}

void performDiagnostics_clear() { // 진단 코드 삭제
  int speed, rpm;
  obd.readPID(PID_SPEED, speed);
  obd.readPID(PID_RPM, rpm);
  if (speed == 0 && rpm <= 200) { // 차량이 시동이 걸려있지 않는 상황 (예시)
    obd.clearDTC();
    BTSerial.print("777"); // Vehicle DTC Clear Code "777"
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
    delay(7000); // Pause for 10 sec after successful fetch of DTC codes.
  }
  else {
    BTSerial.print("666"); // Vehicle Running Error Code "666"
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
    delay(5000);
  }
}

void boot_display() {
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
}


void setup() {
  u8g1.setFont(u8g_font_unifont); // Choose a suitable font
  u8g2.setFont(u8g_font_unifont); // Choose a suitable font
  boot_display();
  delay(2000);
  Serial.begin(115200);
  OBDSerial.begin(115200);
  BTSerial.begin(115200);
  while (!Serial);
  
  for (;;) {
    delay(1000);
    byte version = obd.begin();
    if (version > 0) {
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
      u8g1.firstPage();
      do {
        u8g1.setPrintPos(0, 20); u8g1.print("Freematics");
        u8g1.setPrintPos(0, 40); u8g1.print("OBD-II Adapter");
        u8g1.setPrintPos(0, 60); u8g1.print("NOT DETECTED");
      } while(u8g1.nextPage());
      u8g2.firstPage();
      do {
        u8g2.setPrintPos(0, 20); u8g2.print("OBD-II Firmware");
        u8g2.setPrintPos(0, 40); u8g2.print("Version");
        u8g2.setPrintPos(0, 60); u8g2.print("NOT DETECTED");
      } while(u8g2.nextPage());
    }
  }
  // send some commands for testing and show response for debugging purpose
  testATcommands();

  // initialize OBD-II adapter
  do {
    u8g1.firstPage();
    do {
      u8g1.setPrintPos(0, 20); u8g1.print("Freematics");
      u8g1.setPrintPos(0, 40); u8g1.print("OBD-II Adapter");
      u8g1.setPrintPos(0, 60); u8g1.print("Connecting...");
    } while(u8g1.nextPage());
    delay(1000);
  } while (!obd.init());
  u8g1.firstPage();
  do {
    u8g1.setPrintPos(0, 20); u8g1.print("Freematics");
    u8g1.setPrintPos(0, 40); u8g1.print("OBD-II Adapter");
    u8g1.setPrintPos(0, 60); u8g1.print("CONNECTED!!!");
  } while(u8g1.nextPage());

  char buf[64];
  if (obd.getVIN(buf, sizeof(buf))) {
    BTSerial.print("VIN:");
    BTSerial.println(buf);
    u8g2.firstPage();
    do {
      u8g2.setPrintPos(0, 15); u8g2.print("Vehicle");
      u8g2.setPrintPos(0, 30); u8g2.print("Identification");
      u8g2.setPrintPos(0, 45); u8g2.print("Number");
      u8g2.setPrintPos(0, 60); u8g2.print(buf);
    } while(u8g2.nextPage());
  }
  delay(3000);
}

void loop() {
  Display_1(); // SH1106 OLED Display_1
  Display_2(); // SH1106 OLED Display_2
  if (BTSerial.available()) {       // Bluetooth 데이터 값 유무 확인
    char received = BTSerial.read(); // 데이터 없는 경우 -1 할당
    if (received >= '1' && received <= '5') {
      screenId = received - '0'; // 전송받은 문자를 숫자로 변환
    }
  }
  if (screenId >= 1 && screenId <= numScreens) {
    screenActive[screenId - 1] = true; // 화면 활성화
    for (int i = 0; i < numScreens; i++) {
      if (screenActive[i]) {
        switch (i + 1) {
          case 1:
            showDashboard();
            screenActive[i] = false;
            break;
          case 2:
            performMonitoring();
            screenActive[i] = false;
            break;
          case 3:
            showDrivingRecords();
            screenActive[i] = false;
            break;
          default:
            break;
        }
      }
    }
  }
  else {
    switch (screenId) {
      case 4:
        performDiagnostics();
        screenId = 0;
        break;
      case 5:
        performDiagnostics_clear();
        screenId = 0;
        break;
      default:
        break;
    }
  }
}

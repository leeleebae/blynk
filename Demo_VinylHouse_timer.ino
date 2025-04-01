

#define BLYNK_FIRMWARE_VERSION "0.1.1"

//azuma
#define BLYNK_TEMPLATE_ID "TMPL61b3dE3pn"
#define BLYNK_TEMPLATE_NAME "비닐하우스 타이머 데모"
#define BLYNK_AUTH_TOKEN "TaSQWNVxPGplRVVEScnJuU31eD7LIPGn"

//hidehiko
// #define BLYNK_TEMPLATE_ID "TMPL69dh1Tbx4"
// #define BLYNK_TEMPLATE_NAME "비닐하우스 타이머 데모"
// #define BLYNK_AUTH_TOKEN "BQI1PMPHzfzvNFGCtjhFJizf8ik-2CFb"

//leeleebae
// #define BLYNK_TEMPLATE_ID "TMPL65fzjqkFz"
// #define BLYNK_TEMPLATE_NAME "비닐하우스 타이머 데모"
// #define BLYNK_AUTH_TOKEN "YKPIppsZmdhzKvsUtAduzuG6n09vLgXm"


//#define BLYNK_DEBUG // Blynk 디버깅 모드 활성화
//#define DEBUG_MODE
#define TEST_MODE
#define BLYNK_MODE

#define BLYNK_HEARTBEAT 30  // 30초마다 서버와 핑 체크,기본은 10초이다.

#define BLYNK_PRINT Serial
#define DHTPIN4 4      //  배전반의 이더넷 카드의 DHT22 센서가 연결 된 핀 번호
#define DHTPINA 7      //  A동 DHT22 센서가 연결 된 핀 번호
#define DHTPINB 8      //  B동 DHT22 센서가 연결 된 핀 번호
#define DHTTYPE DHT22  //  DHT22 센서 사용   (DHT22 센서는 DHT22로 수정, DHT11일경우 DHT11)

#define W5100_RESET_PIN 12  // W5100의 RESET 핀 (D12에 연결)

#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>
#include <DHT.h>
#include <NTPClient.h>
#include "CFMEGA.h"


bool isFirstUserTemp = true;
bool isFirstRunMode = true;

bool isReadPrevModeCheck = true;     // 한번만 실행해서 자동/수동모드인지 확인
bool isReadsetUserOpenTemp = false;  // 전의 사용자 개폐온도 읽기
//메뉴 위젯에서 빈번한,적당한을 제외한 다른 기능을 클릭했을때 작동이 끝난후에는 빈번한,적당한 값을보여준다.
int selectedtMenuWiget = 0;

bool isConnected = false;

//쿨링팬을 Runing 할때 사용되는 온도
int setFanRunTemp = 40;
int setFanMinusTemp = 10;
int setFanNotiTemp = 50;

//측면자동개폐 Runing 할대 사용되는 온도
int setUserOpenTemp;  //사용자가 정하는 측면개폐온도
int setAutoMinusTemp = 10;
int setAutoNotiTemp = 10;

//실제 환경
const unsigned long connectionCheckinterval = (1000L * 1);  //1초

static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 177);  // 사용할 고정 IP (변경 가능)
EthernetUDP ntpUDP;
const long utcOffset = 9 * 3600;  // 한국 시간(UTC+9) 기준
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffset);

#define AUTO_MODE true
#define MANU_MODE false
bool RUN_MODE = AUTO_MODE;

//자동관수 타이머
//timer 위젯의 경우 11시 20분처럼 두자리가 넘어가면 마이너스 값을 주므로 String으로 데이타를 받는다.
String server_time;
String startTimerA1;  // A동 오전 관수 첫번째 타이머  start
String endTimerA1;    // A동 오전 관수 첫번째 타이머 stop
String selectedWeekA1;
String startTimerA2;
String endTimerA2;
String selectedWeekA2;
String startTimerB1;
String endTimerB1;
String selectedWeekB1;
String startTimerB2;
String endTimerB2;
String selectedWeekB2;



CFNET cfnet;

//BlynkLog LogEvent 관련변수
#define LogEventQue_SIZE 50  // 큐의 최대 크기

struct LogEvent {
  String eventName;
  String message;
};

LogEvent LogEventQueue[LogEventQue_SIZE];  // 이벤트를 저장하는 큐 배열
int logeventfront = 0, logeventrear = 0, logeventcount = 0;
BlynkTimer blnklogEventTimer;
const unsigned long blnklogEventTimerinterval = (1000L * 2);  //2초



BlynkTimer connectionTimer;  // 인터넷 연결 체크 타이머
unsigned long lastConnectionTime = 0;
const unsigned long connectionTimeout = 1000 * 5;  // 5초 제한 시간

unsigned long lastNotificationTime = 0;
const unsigned long notificationTimeout = 1000 * 1;  // 10초 제한 시간

BlynkTimer ntpUpdateTimer;                                 //NTP update timer
const unsigned long ntpUpdateinterval = (1000L * 60 * 5);  //5분
// 시간 관리 변수
unsigned long lastUpdateMillis = 0;
unsigned long currentMillis = 0;
unsigned long elapsedSeconds = 0;

DHT dhtE(DHTPIN4, DHTTYPE);
DHT dhtA(DHTPINA, DHTTYPE);
DHT dhtB(DHTPINB, DHTTYPE);

//조명
bool bAHouse_Light = false;
bool bBHouse_Light = false;


//Relay On,OFF,Wiget On, OFF의 변화가 첫번째일때만 보낸다. 같은 중복된 데이타는 보내지 않는다.
bool AHouse_WaterRelay_On = false;

bool AHouse_WaterTimer1_On = false;
bool AHouse_WaterWiget1_On = false;

bool AHouse_WaterTimer2_On = false;
bool AHouse_WaterWiget2_On = false;


bool BHouse_WaterRelay_On = false;

bool BHouse_WaterTimer3_On = false;
bool BHouse_WaterWiget3_On = false;

bool BHouse_WaterTimer4_On = false;
bool BHouse_WaterWiget4_On = false;


#define tempfrequentMode 0
#define tempSometimeMode 1
bool TempCollectionMode = tempfrequentMode;




//------- 온도 센싱하여 보내는 변수 --------

String prevAHouse_temp = "NODATE";
String currentAHouse_temp = "NODATE";

#define TEMP_ERR_CNT 30
int SensorErrorCnt_AHouse = 0;

bool AHouse_WindowOpen_sendOn = false;
bool AHouse_setUserOpenTempOver_Alarmsend = false;


String prevBHouse_temp = "NODATE";
String currentBHouse_temp = "NODATE";
int SensorErrorCnt_BHouse = 0;

bool BHouse_WindowOpen_sendOn = false;
bool BHouse_setUserOpenTempOver_Alarmsend = false;

bool FAN_RUNING = false;  // 배전반의 쿨링팬의 동작상태
int SensorErrorCnt_BaeJoneBan = 0;
bool bBaeJoneBanTempOver_Alarmsend = false;

int touchRebootCount = 0;

bool containsSubstring(String str, String target) {
  return str.indexOf(target) != -1;
}

// 이벤트를 큐에 추가하는 함수
void logEvent_Enqueue(String eventName, String message) {
  if (logeventcount < LogEventQue_SIZE) {
    LogEventQueue[logeventrear].eventName = eventName;
    LogEventQueue[logeventrear].message = message;
    logeventrear = (logeventrear + 1) % LogEventQue_SIZE;
    logeventcount++;
  } else {
#ifdef BLYNK_MODE
    Serial.println("큐가 가득 참! 이벤트 추가 불가.");
#endif
  }
}

// 이벤트를 큐에서 제거하고 실행하는 함수
bool logEvent_Dequeue(LogEvent &logevent) {
  if (logeventcount > 0) {
    logevent = LogEventQueue[logeventfront];
    logeventfront = (logeventfront + 1) % LogEventQue_SIZE;
    logeventcount--;
    return true;
  }
  return false;
}

// 2초마다 실행되서 한개씩만 Blynk서버로 보낸다.
void processEventQueue() {
  bool isSend = false;
  LogEvent logevent;
  if (logEvent_Dequeue(logevent)) {
    Blynk.logEvent(logevent.eventName, logevent.message);  // Blynk 이벤트 실행
#ifdef BLYNK_MODE
    Serial.println("logEvent 실행: " + logevent.eventName + " - " + logevent.message);
#endif
  }
}


/*
  초기값 모드 설정
  1. 모든 릴레이를 OFF한다.
  2. Blynk서버에서 관수타이머,자동개폐온도,,자동/수동모드,온도수집주기 정보를 Blynk 서버와 Sync을 맟춘다.
  3. 주의할것은 자동/수동모드의 BLYNK_WRITE(V127) 함수에서 자기 자신을 Blynk서버와 동기시키면 무한루프가 되므로 자신은 제외
  3. 환경설정 모드는 읽기모드로 세팅, 기타기능에서 그전에 저장된 온도수집주기(0,1)로 설정한다.
*/

void auto_menu_switch_init() {
  Blynk.virtualWrite(V10, LOW);  //A동 측면자동개폐LED
  AHouse_WindowClose();          //A동 창문을닫는다.
  Blynk.virtualWrite(V2, LOW);   //A동 수동개폐스위치

  Blynk.virtualWrite(V28, LOW);  //B동 측면자동개폐LED
  BHouse_WindowClose();          //B동 창문을닫는다.
  Blynk.virtualWrite(V20, LOW);  //B동 수동개폐스위치

  //화면 위젯을 모두 OFF
  Blynk.virtualWrite(V6, LOW);    //A동 관수스위치
  Blynk.virtualWrite(V24, LOW);   //B동 관수스위치
  cfnet.digitalWrite(0, 6, LOW);  //A동관수릴레이
  cfnet.digitalWrite(1, 6, LOW);  //B동관수릴레이

  AHouse_WaterTimer1_On = false;
  AHouse_WaterWiget1_On = false;

  AHouse_WaterTimer2_On = false;
  AHouse_WaterWiget2_On = false;

  BHouse_WaterTimer3_On = false;
  BHouse_WaterWiget3_On = false;

  BHouse_WaterTimer4_On = false;
  BHouse_WaterWiget4_On = false;

  AHouse_WaterRelay_On = false;
  BHouse_WaterRelay_On = false;
}


void setInitialMode() {
  //기본적으로 기타기능 경우 disable 상태이므로 enable시킨다.
  Blynk.setProperty(V126, "isDisabled", false);
  Blynk.syncVirtual(V126);

  if (isReadPrevModeCheck) {
    //전의 모드값을 가져오고 AutoMOde,Menu 모드값만 가져온다.
    Blynk.syncVirtual(V127);
    unsigned long startMillis = millis();  // 시작 시간 기록
    const unsigned long timeout = 5000;    // 5초 제한

    while (isReadPrevModeCheck) {
      Blynk.run();                             // Blynk 이벤트 처리 (BLYNK_WRITE 실행 가능하도록)
      if (millis() - startMillis > timeout) {  // 타임아웃 방지
        Serial.println("Blynk sync timeout!");
        break;
      }
    }
  }


  if (!isReadsetUserOpenTemp) {
    Blynk.syncVirtual(V30);
    unsigned long startMillis = millis();  // 시작 시간 기록
    const unsigned long timeout = 5000;    // 5초 제한

    while (!isReadsetUserOpenTemp) {
      Blynk.run();                             // Blynk 이벤트 처리 (BLYNK_WRITE 실행 가능하도록)
      if (millis() - startMillis > timeout) {  // 타임아웃 방지
        Serial.println("Blynk sync timeout!");
        break;
      }
    }
  }

  Serial.println("전의 사용자 설정온도: " + String(setUserOpenTemp) + "°C");

  //전의 모드가 수동모드라면
  if (RUN_MODE == MANU_MODE) {
    //수동개패스위치,관수스위를 싱크을한다.
    Blynk.syncVirtual(V2, V6, V20, V24);
    Blynk.run();

#ifdef BLYNK_MODE
    Serial.println("수동모드이므로 A동,B동 개폐스위치,관수스위치를 싱크했습니다.");
#endif

  } else {  //자동모드라면

    //2개의 채널 릴레이를 모두 OFF
    cfnet.digitalWrite(0, LOW);
    cfnet.digitalWrite(1, LOW);
    Blynk.virtualWrite(V32, LOW);  //A,B동 공통 조명 LED

    //자동,수동에 따라 연관된 위젯을 enable/disable시킴, 특히 관수타이머도 싱크한다.
    Blynk.syncVirtual(V127);
  }

  //자동,수동에 따라 연관된 위젯을  enable/disable시킴
  //여기서 하면 auto_menu_switch_init()가 작동되서 수동제어스위치가 작동되지 않는다.
  //Blynk.syncVirtual(V127);

  // 환경설정 세그먼트 스위치를 읽기 모드로 설정, 개폐온도,운전모드 disable
  Blynk.virtualWrite(V31, HIGH);
  Blynk.setProperty(V30, "isDisabled", true);   //개폐온도 disable
  Blynk.setProperty(V127, "isDisabled", true);  //운전모드 disable
}

/*
  A동 배전반의 온도 상태에 따라 냉각팬을 ON/OFF, 타이머는 Setup함수 정함
  1.40이상은 냉각팬 ON
  2.냉각팬이 ON 상태에서 30~40도인경우 계속 식히기 위해서 그대로 냉각팬을 ON
  3.냉각팬이 OFF 상태에서 30~40도인경우 계속 식히기 위해서 그대로 냉각팬waring을 OFF
  3.온도가 30도 미만인경우 냉각펜 OFF
  4.온도가 45도 넘으면 스마트폰으로 통지를 보낸다.
*/

void sensorBaeJoneBan() {

  int t = static_cast<int>(dhtE.readTemperature());  // 섭씨 온도,, DHT11경우 int, 만약 DHT22센서 사용시 float형으로 바꿀 것
  int h = static_cast<int>(dhtE.readHumidity());


  if (isnan(t) || isnan(h)) {
    SensorErrorCnt_BaeJoneBan++;
    return;
  }

  if (SensorErrorCnt_BaeJoneBan == TEMP_ERR_CNT)  //온도 센서 에러가 10번 이상 나오면 경보 알람을 보낸다.
  {
    logEvent_Enqueue("waringalert", "배전반의 온도습도센서의 데이타가 올바르지 않습니다. 온도습도센서를 확인해주세요. 기타기능 > 시스탬리부팅을 3번 클릭 해보시길 바랍니다.");
    SensorErrorCnt_BaeJoneBan = 0;
    return;
  }
#ifdef DEBUG_MODE
  Serial.println("현재 배전반의 온도:" + String(t));
  Serial.println("현재 배전반의 습도:" + String(h));
#endif



  //40도 이상일대 냉각팬을 켜고 다시 30도 이하로 떨어질때 냉각팬을 끈다.
  if (t >= (setFanRunTemp - setFanMinusTemp) && t < setFanRunTemp) {
    //아무것도 안한다.
  } else if (t >= setFanRunTemp) {
    if (!FAN_RUNING) {  //처음 경보 Send 했다면
      // 온도 경보 알람을 보낸다.
      FAN_RUNING = true;
      cfnet.digitalWrite(0, 0, HIGH);  //냉각팬을 켠다.
      logEvent_Enqueue("infoalert", "배전판의 냉각팬을 작동시킵니다. 현재 온도: " + String(t) + "°C, 사용자 설정온도: " + String(setFanRunTemp) + "°C");
    }
  } else {  //설정한 값, 설정한값 -10 가 아닌 온도이고 결론 냉각팬을 끄는 온도이다.

    if (FAN_RUNING) {                 //현재 알람이 ON상태라면 알람해제 되었습니다.
      cfnet.digitalWrite(0, 0, LOW);  //냉각팬을 끈다.

      logEvent_Enqueue("infoalert", "배전반의 냉각팬을 중지했습니다. 현재 온도: " + String(t) + "°C, 사용자 설정온도: " + String(setFanRunTemp) + "°C");
      FAN_RUNING = false;
    }
  }

  //배전반의 온도가 60도 넘으면 스마트폰에 알람을 보낸다.
  if (t >= setFanNotiTemp) {
    if (!bBaeJoneBanTempOver_Alarmsend) {  //처음 경보 Send 했다면
      // 온도 경보 알람을 보낸다.
      bBaeJoneBanTempOver_Alarmsend = true;
      logEvent_Enqueue("waringalert", "배전반의 온도가 높습니다. 확인해주세요." + String(t) + "°C");
    }
  } else {
    if (bBaeJoneBanTempOver_Alarmsend) {  //현재 알람이 ON상태라면 알람해제 되었습니다.
      logEvent_Enqueue("infoalert", "배전반의 온도가 정상범위로 돌아왔습니다." + String(t) + "°C");
      bBaeJoneBanTempOver_Alarmsend = false;
    }
  }
}


//A동 개패스위치을 On/Off했을때, 좌측,우측 측면 모터를 한번에 On,Off한다.
BLYNK_WRITE(V2) {

  int pinValue = param.asInt();

  if (pinValue) {  //열림
    AHouse_WindowOpen();

  } else {  //닫힘
    AHouse_WindowClose();
  }
}

//A동의 창을 개폐을 한다.
void AHouse_WindowOpen() {
  cfnet.digitalWrite(0, 1, HIGH);  //촤측개패모터열림
  cfnet.digitalWrite(0, 2, LOW);   //좌측개폐모터닫힘
  cfnet.digitalWrite(0, 3, HIGH);  //우측개패모터열림
  cfnet.digitalWrite(0, 4, LOW);   //우측개폐모터닫힘
}
void AHouse_WindowClose() {
  cfnet.digitalWrite(0, 1, LOW);   //촤측개패모터열림
  cfnet.digitalWrite(0, 2, HIGH);  //좌측개폐모터닫힘
  cfnet.digitalWrite(0, 3, LOW);   //우측개패모터열림
  cfnet.digitalWrite(0, 4, HIGH);  //우측개폐모터닫힘
}

//A동 하우스 온도 습도 데이타를 블링크 서버로 보낸다. 타임은 setup 함수에서 정의됨
//자동설정모드이라면 사용자가 설정한 온도에서 측면을 개방한다.
void sendSensorAHouse() {


  //온도 습도를 float을 int로 변환 (소수점 절삭)
  int t = static_cast<int>(dhtA.readTemperature());  // 섭씨 온도, DHT11경우 int, 만약 DHT22센서 사용시 float형으로 바꿀 것
  int h = static_cast<int>(dhtA.readHumidity());


  if (isnan(t) || isnan(h)) {
    SensorErrorCnt_AHouse++;
    return;
  }


  if (SensorErrorCnt_AHouse == TEMP_ERR_CNT)  //온도 센서 에러가 10번 이상 나오면 경보 알람을 보낸다.
  {
    logEvent_Enqueue("waringalert", "A동 하우스의 온도습도센서의 데이타가 올바르지 않습니다. 온도습도센서를 확인해주세요. 기타기능 > 시스탬리부팅을 3번 클릭 해보시길 바랍니다.");
    SensorErrorCnt_AHouse = 0;
    return;
  }
#ifdef DEBUG_MODE
  Serial.println("현재 A동하우스 온도 :" + String(t));
  Serial.println("현재 A동하우스 습도 :" + String(h));
#endif

  //전온도값(prevAHouse_temp)에 현재의 온도가 없다면 새로운 현재온도값(currentAHouse_temp)을 만든다.
  if (!containsSubstring(prevAHouse_temp, String(t))) {
    if (TempCollectionMode == tempfrequentMode) {  //빈번한 온도수집

      currentAHouse_temp = String(t);                     // float을 int로 변환 (소수점 절삭)
    } else if (TempCollectionMode == tempSometimeMode) {  //적당한 온도수집

      currentAHouse_temp = String(t - 1) + "," + String(t) + "," + String(t + 1);
    } else {
#ifdef DEBUG_MODE
      Serial.println("sendSensorAHouse ERROR : 정의 되지 않는 else 입니다.");
#endif
    }
  }
#ifdef DEBUG_MODE
  Serial.println("A동 하우스 과거온도:" + prevAHouse_temp);
  Serial.println("A동 하우스 현재온도:" + currentAHouse_temp);
#endif

  //전에 온도와 지금의 온도가 다를때만 Blnk서버로 보낸다.
  if (prevAHouse_temp != currentAHouse_temp) {
    prevAHouse_temp = currentAHouse_temp;

    Blynk.virtualWrite(V3, t);  // 온도위젯에 온도전달
    Blynk.virtualWrite(V4, h);  // 습도위젯에 습도전달
#ifdef BLYNK_MODE
    Serial.println("A동 하우스 온도 위젯의 값을 변경하였습니다.");
#endif
  }


  if (RUN_MODE == AUTO_MODE) {  //자동모드이리면
#ifdef DEBUG_MODE
    Serial.println("setUserOpenTemp :" + String(setUserOpenTemp));
#endif


    //개페온도 - 마이너스 온도 사이는 그대로 둔다.
    if (t >= (setUserOpenTemp - setAutoMinusTemp) && t < setUserOpenTemp) {
      //그대로 둔다.
    } else if (t >= setUserOpenTemp) {  //측면창을 연다.
      if (!AHouse_WindowOpen_sendOn) {  //처음 OPen 했다면
        AHouse_WindowOpen_sendOn = true;
        AHouse_WindowOpen();
        Blynk.virtualWrite(V10, HIGH);  //화면자동개폐LED ON
        logEvent_Enqueue("infoalert", "A동의 창을 열어습니다. 현재 온도: " + String(t) + "°C, 사용자 설정온도: " + String(setUserOpenTemp) + "°C");
      }

    } else {
      if (AHouse_WindowOpen_sendOn) {  //열린상태이라면
        AHouse_WindowOpen_sendOn = false;
        AHouse_WindowClose();
        Blynk.virtualWrite(V10, LOW);  //화면자동개폐LED OFF

        logEvent_Enqueue("infoalert", "A동의 창을 닫았습니다. 현재 온도: " + String(t) + "°C, 사용자 설정온도: " + String(setUserOpenTemp) + "°C");
      }
    }
  }

  //사용자가 하우스 측면문을 개폐하는 온도 + 5도가 높으면 알람을 보낸다.

  if (t >= (setUserOpenTemp + setAutoNotiTemp)) {

    if (!AHouse_setUserOpenTempOver_Alarmsend) {  //처음 경보 Send 했다면
      // 온도 경보 알람을 보낸다.
      AHouse_setUserOpenTempOver_Alarmsend = true;

      logEvent_Enqueue("waringalert", "A동 하우스의 온도가 높습니다. 확인해주세요. 현재 온도: " + String(t) + "°C, 사용자 설정온도: " + String(setUserOpenTemp) + "°C");
    }
  } else {
    if (AHouse_setUserOpenTempOver_Alarmsend) {  //현재 알람이 ON상태라면 알람해제 되었습니다.

      logEvent_Enqueue("infoalert", "A동 하우스의 온도가 정상범위로 돌아왔습니다. 현재 온도: " + String(t) + "°C, 사용자 설정온도: " + String(setUserOpenTemp) + "°C");

      AHouse_setUserOpenTempOver_Alarmsend = false;
    }
  }
}

//A동 관수 스위치 On/Off했을때
BLYNK_WRITE(V6) {

  int pinValue = param.asInt();
  cfnet.digitalWrite(0, 6, pinValue);
}

//A동 오전관수타이머의 Start/Stop의 시간을 저장한다. blynk요일을  NTC요일로 변경한다.
BLYNK_WRITE(V7) {
  startTimerA1 = param[0].asStr();
  endTimerA1 = param[1].asStr();
  selectedWeekA1 = param[3].asStr();
  if (startTimerA1 == 0 && endTimerA1 == 0) {
    startTimerA1 = 999999;
    endTimerA1 = 999999;
  }

  /*
    다음과 같은 코드로 시간을 구할수 있음
    int startHours = startTimerA1 / 3600;
    int startMinutes = (startTimerA1 % 3600) / 60;
    int startSeconds = startTimerA1 % 60;
  */
}

//A동 오후관수타이머가 Start/Stop했을때
BLYNK_WRITE(V8) {

  startTimerA2 = param[0].asStr();
  endTimerA2 = param[1].asStr();
  selectedWeekA2 = param[3].asStr();
  if (startTimerA2 == 0 && endTimerA2 == 0) {
    startTimerA2 = 999999;
    endTimerA2 = 999999;
  }
}


//---------- B동 하우스 세팅 ---------
//B동의 창을 개폐을 한다.
void BHouse_WindowOpen() {
  cfnet.digitalWrite(1, 1, HIGH);  //촤측개패모터열림
  cfnet.digitalWrite(1, 2, LOW);   //좌측개폐모터닫힘
  cfnet.digitalWrite(1, 3, HIGH);  //우측개패모터열림
  cfnet.digitalWrite(1, 4, LOW);   //우측개폐모터닫힘
}
void BHouse_WindowClose() {
  cfnet.digitalWrite(1, 1, LOW);   //촤측개패모터열림
  cfnet.digitalWrite(1, 2, HIGH);  //좌측개폐모터닫힘
  cfnet.digitalWrite(1, 3, LOW);   //우측개패모터열림
  cfnet.digitalWrite(1, 4, HIGH);  //우측개폐모터닫힘
}

//B동 개패스위치을 On/Off했을때, 좌측,우측 개폐모터를  On,Off한다.
BLYNK_WRITE(V20) {
  int pinValue = param.asInt();
  if (pinValue) {  //열림
    BHouse_WindowOpen();

  } else {  //닫힘
    BHouse_WindowClose();
  }
}

//B동 하우스 온도 습도 데이타를 블링크 서버로 보낸다. 타임은 setup 함수에서 정의됨
void sendSensorBHouse() {

  //온도 습도를 float을 int로 변환 (소수점 절삭)
  int t = static_cast<int>(dhtB.readTemperature());  // 섭씨 온도, DHT11경우 int, 만약 DHT22센서 사용시 float형으로 바꿀 것
  int h = static_cast<int>(dhtB.readHumidity());


  if (isnan(t) || isnan(h)) {
#ifdef DEBUG_MODE
    Serial.println("Failed to read from SensorBHouse DHT sensor!");
#endif
    SensorErrorCnt_BHouse++;
    return;
  }


  if (SensorErrorCnt_BHouse == TEMP_ERR_CNT)  //온도 센서 에러가 10번 이상 나오면 경보 알람을 보낸다.
  {
    logEvent_Enqueue("waringalert", "B동 하우스의 온도습도센서의 데이타가 올바르지 않습니다. 온도습도센서를 확인해주세요. 기타기능 > 시스탬리부팅을 3번 클릭 해보시길 바랍니다.");

    SensorErrorCnt_BHouse = 0;
    return;
  }
#ifdef DEBUG_MODE
  Serial.println("현재 B동하우스 온도:" + String(t));
  Serial.println("현재 B동하우스 습도:" + String(h));
#endif



  //전온도값(prevBHouse_temp)에 현재의 온도가 없다면 새로운 현재온도값(currentBHouse_temp)을 만든다.
  if (!containsSubstring(prevBHouse_temp, String(t))) {
    if (TempCollectionMode == tempfrequentMode) {  //빈번한 온도수집

      currentBHouse_temp = String(t);                     // float을 int로 변환 (소수점 절삭)
    } else if (TempCollectionMode == tempSometimeMode) {  //적당한 온도수집

      currentBHouse_temp = String(t - 1) + "," + String(t) + "," + String(t + 1);
    } else {
      Serial.println("sendSensorBHouse ERROR : 정의 되지 않는 else 입니다.");
    }
  }
#ifdef DEBUG_MODE
  Serial.println("B동 하우스 과거온도:" + prevBHouse_temp);
  Serial.println("B동 하우스 현재온도:" + currentBHouse_temp);
#endif


  //그전의 온도와 현재의 온도가 다를때만 보낸다.
  if (prevBHouse_temp != currentBHouse_temp) {
    prevBHouse_temp = currentBHouse_temp;
    Blynk.virtualWrite(V21, t);  // 온도위젯에 온도전달
    Blynk.virtualWrite(V22, h);  // 습도위젯에 습도전달
#ifdef DEBUG_MODE
    Serial.println("B동 하우스 온도 위젯의 값을 변경하였습니다.");
#endif
  }



  if (RUN_MODE == AUTO_MODE) {  //자동모드이리면

    //개페온도 - 마이너스 온도 사이는 그대로 둔다.
    if (t >= (setUserOpenTemp - setAutoMinusTemp) && t < setUserOpenTemp) {
      //그대로 둔다.
    } else if (t >= setUserOpenTemp) {  //측면창을 연다.
      if (!BHouse_WindowOpen_sendOn) {  //처음 OPen 했다면
        BHouse_WindowOpen();
        Blynk.virtualWrite(V28, HIGH);  //화면자동개폐LED ON
        BHouse_WindowOpen_sendOn = true;
        //알람을 보내는 코드를 만든다.
        logEvent_Enqueue("infoalert", "B동의 창을 열어습니다. 현재 온도: " + String(t) + "°C, 사용자 설정온도: " + String(setUserOpenTemp) + "°C");
      }
    } else {
      if (BHouse_WindowOpen_sendOn) {  //열린적이 있다면
        BHouse_WindowOpen_sendOn = false;
        BHouse_WindowClose();
        Blynk.virtualWrite(V28, LOW);  //화면자동개폐LED OFF

        //알람을 보내는 코드를 만든다.
        logEvent_Enqueue("infoalert", "B동의 창을 닫았습니다. 현재 온도: " + String(t) + "°C, 사용자 설정온도: " + String(setUserOpenTemp) + "°C");
      }
    }
  }

  //사용자가 하우스 측면문을 개폐하는 온도 + 5도가 높으면 알람을 보낸다.

  if (t >= (setUserOpenTemp + setAutoNotiTemp)) {

    if (!BHouse_setUserOpenTempOver_Alarmsend) {  //처음 경보 Send 했다면
      // 온도 경보 알람을 보낸다.
      BHouse_setUserOpenTempOver_Alarmsend = true;

      logEvent_Enqueue("waringalert", "B동 하우스의 온도가 높습니다. 확인해주세요." + String(t) + "°C, 사용자 설정온도: " + String(setUserOpenTemp) + "°C");
    }
  } else {
    if (BHouse_setUserOpenTempOver_Alarmsend) {  //현재 알람이 ON상태라면 알람해제 되었습니다.

      logEvent_Enqueue("infoalert", "B동 하우스의 온도가 정상범위로 돌아왔습니다." + String(t) + "°C, 사용자 설정온도: " + String(setUserOpenTemp) + "°C");

      BHouse_setUserOpenTempOver_Alarmsend = false;
    }
  }
}


//B동 관수 스위치 On/Off했을때
BLYNK_WRITE(V24) {
  int pinValue = param.asInt();
  cfnet.digitalWrite(1, 6, pinValue);
}

//B동 오전관수타이머가 Start/Stop했을때
BLYNK_WRITE(V25) {
  startTimerB1 = param[0].asStr();
  endTimerB1 = param[1].asStr();
  selectedWeekB1 = param[3].asStr();
  if (startTimerB1 == 0 && endTimerB1 == 0) {
    startTimerB1 = 999999;
    endTimerB1 = 999999;
  }
}


//B동 오후관수타이머가 Start/Stop했을때
BLYNK_WRITE(V26) {
  startTimerB2 = param[0].asStr();
  endTimerB2 = param[1].asStr();
  selectedWeekB2 = param[3].asStr();
  if (startTimerB2 == 0 && endTimerB2 == 0) {
    startTimerB2 = 999999;
    endTimerB2 = 999999;
  }
}

/*1)
  환경설정에서 편집/일기 선택시
  읽기모드에서는 모두 Diable
  편집모드에서는 모두 Enable
*/
BLYNK_WRITE(V31) {
  int switchValue = param.asInt();  // 선택된 값 (0, 1)

  switch (switchValue) {
    case 0:
#ifdef DEBUG_MODE
      Serial.println("편집/읽기모드: 편집");  //편집
#endif
      Blynk.setProperty(V30, "isDisabled", false);   //개폐온도
      Blynk.setProperty(V127, "isDisabled", false);  //운전모드 Enable
      break;
    case 1:
#ifdef DEBUG_MODE
      Serial.println("편집/읽기모드: 읽기");  //읽기
#endif
      Blynk.setProperty(V30, "isDisabled", true);   //개폐온도 Disalbe
      Blynk.setProperty(V127, "isDisabled", true);  //운전모드 Disalbe
      break;
  }
}


/*
1. 재부팅시 또는 재 연결시 온도 수집모드필요하므로
   메뉴을 선택한 이후,다른 메뉴을 실행한후 반드시 0,1로 값을 되돌려 놓는다.

*/
BLYNK_WRITE(V126) {
  int selectedValue = param.asInt();


  if (selectedValue == 0 || selectedValue == 1) {
    selectedtMenuWiget = selectedValue;
  }

  //스마트폰의 화면에 값을 세팅한다.
  Blynk.virtualWrite(V126, selectedtMenuWiget);

  switch (selectedValue) {
    case 0:  //빈번한 온도수집
#ifdef DEBUG_MODE
      Serial.println("빈번한 온도수집");
#endif
      TempCollectionMode = tempfrequentMode;
      break;
    case 1:
#ifdef DEBUG_MODE
      Serial.println("적당한 온도수집");
#endif
      TempCollectionMode = tempSometimeMode;
      break;
    case 2:  //A동 조명 켜기
      bAHouse_Light = true;
      cfnet.digitalWrite(0, 5, HIGH);
      if (bBHouse_Light) {
        Blynk.setProperty(V32, "label", "조명:A-B");
      } else {
        Blynk.setProperty(V32, "label", "조명:A");
      }
      Blynk.virtualWrite(V32, HIGH);
#ifdef DEBUG_MODE
      Serial.println("A동 조명 켜기");
#endif
      break;
    case 3:  //A동 조명 끄기
      bAHouse_Light = false;
      cfnet.digitalWrite(0, 5, LOW);
#ifdef DEBUG_MODE
      Serial.println("A동 조명 끄기");
#endif
      if (bBHouse_Light) {
        Blynk.setProperty(V32, "label", "조명:B");
        Blynk.virtualWrite(V32, HIGH);
      } else {
        Blynk.setProperty(V32, "label", "조명");
        Blynk.virtualWrite(V32, LOW);
      }

      break;
    case 4:  //B동 조명 켜기
      bBHouse_Light = true;
#ifdef DEBUG_MODE
      Serial.println("B동 조명 켜기");
#endif
      cfnet.digitalWrite(1, 5, HIGH);
      if (bAHouse_Light) {
        Blynk.setProperty(V32, "label", "조명:A-B");
      } else {
        Blynk.setProperty(V32, "label", "조명:B");
      }
      Blynk.virtualWrite(V32, HIGH);

      break;
    case 5:  //B동 조명 끄기
      bBHouse_Light = false;
      cfnet.digitalWrite(1, 5, LOW);
      if (bAHouse_Light) {
        Blynk.setProperty(V32, "label", "조명:A");
        Blynk.virtualWrite(V32, HIGH);
      } else {
        Blynk.setProperty(V32, "label", "조명");
        Blynk.virtualWrite(V32, LOW);
      }
#ifdef DEBUG_MODE
      Serial.println("B동 조명 끄기");
#endif
      break;
    case 6:  //A동 카메라 찍기
#ifdef DEBUG_MODE
      Serial.println("A동 카메라 찍기");
#endif
      break;
    case 7:  //B동 카메라 찍기
#ifdef DEBUG_MODE
      Serial.println("B동 카메라 찍기");
#endif
      break;
    case 8:  //시스탬 리부팅
      touchRebootCount++;
      if (touchRebootCount == 3) {
        Blynk.disconnect();

        logEvent_Enqueue("waringalert", "사용자가 시스탬을 강제로  리부팅합니다.");

        resetW5100();
        //rebootJmp();
        break;
      }
      break;
  }
}

//측면개폐온도설정
BLYNK_WRITE(V30) {
  setUserOpenTemp = param.asInt();
  isReadsetUserOpenTemp = true;


  // 온도를 바꾸면 자동으로 편집모드가 읽기모드로 변환된다.
  if (!isFirstUserTemp) {
    Serial.println("사용자 설정온도가 변경되었습니다: " + String(setUserOpenTemp) + "°C");
    //자동으로 읽기모드로 변환
    Blynk.virtualWrite(V31, HIGH);
    Blynk.syncVirtual(V31);
  }
  isFirstUserTemp = false;
}

/*
  운전모드를 자동(0) 수동(1)했을때
  1.자동모드에서는 관수타이머가 활성화 되고 관수스위치와 측면개폐스위치는 비활성화된다. 
    RUN_MODE = AUTO_MODE 가 되면서 sendSensorAHouse, sendSensorBHouse에서 온도에 따른 자동 개폐가 된다.
    RUN_MODE = AUTO_MODE 가 되면서 waterTimeSchedule에서 시간에 따른 자동 관수가 된다.
  
  2.수동모드에선느 관수타이머가 비활성화 되고 관수스위치와 측면개폐스위치는 활성화된다. 
    RUN_MODE = MANU_MODE 가 되면서 자동개폐,자동관수는 안된다.

*/
BLYNK_WRITE(V127) {
  int pinValue = param.asInt();

  if (!isFirstRunMode) {
    //변경하면 자동으로 편집모드는 읽기모드로 간다.
    Blynk.virtualWrite(V31, HIGH);
    Blynk.syncVirtual(V31);
  }
  isFirstRunMode = false;


  if (isReadPrevModeCheck) {
    if (pinValue) {
      RUN_MODE = MANU_MODE;
#ifdef BLYNK_MODE
      Serial.println("전의 운전모드가 수동이었습니다.");
#endif
    } else {
      RUN_MODE = AUTO_MODE;
#ifdef BLYNK_MODE
      Serial.println("전의 운전모드가 자동이었습니다.");
#endif
    }
    delay(10);  // 값이 설정될 시간을 확보
    isReadPrevModeCheck = false;
  } else {  //두번째 부터 콜을 당하면 여기서 실행된다.


    auto_menu_switch_init();

    if (pinValue) {

      RUN_MODE = MANU_MODE;  //수동

      Blynk.setProperty(V2, "isDisabled", false);   //enable ,A동개폐스위치
      Blynk.setProperty(V6, "isDisabled", false);   //enable , A동관수스위치
      Blynk.setProperty(V20, "isDisabled", false);  //enable , B동개폐스위치
      Blynk.setProperty(V24, "isDisabled", false);  //enable, B동관수스위치

      Blynk.setProperty(V7, "isDisabled", true);   //disalbe ,A동오전관수타이머
      Blynk.setProperty(V8, "isDisabled", true);   //disalbe ,A동오후관수타이머
      Blynk.setProperty(V25, "isDisabled", true);  //disalbe ,B동오전관수타이머
      Blynk.setProperty(V26, "isDisabled", true);  //disalbe ,B동오후관수타이머

      Blynk.setProperty(V10, "isDisabled", true);  //disable , A동자동개폐LED
      Blynk.setProperty(V28, "isDisabled", true);  //disable, B동자동개폐LED

    } else {

      RUN_MODE = AUTO_MODE;                        //자동
      Blynk.setProperty(V2, "isDisabled", true);   //disable
      Blynk.setProperty(V6, "isDisabled", true);   //disable
      Blynk.setProperty(V20, "isDisabled", true);  //disable
      Blynk.setProperty(V24, "isDisabled", true);  //disable

      Blynk.setProperty(V7, "isDisabled", false);   //enable
      Blynk.setProperty(V8, "isDisabled", false);   //enable
      Blynk.setProperty(V25, "isDisabled", false);  //enable
      Blynk.setProperty(V26, "isDisabled", false);  //enable

      //Enable 상태에서는 관수타이머 싱크을 데이타를 가져온다.
      Blynk.syncVirtual(V7, V8, V25, V26);
      //관수 타이머의 색깔 녹색(OFF)
      Blynk.setProperty(V7, "color", "#006400");   //녹색 (#RRGGBB 형식)
      Blynk.setProperty(V8, "color", "#006400");   //녹색 (#RRGGBB 형식)
      Blynk.setProperty(V25, "color", "#006400");  //녹색 (#RRGGBB 형식)
      Blynk.setProperty(V26, "color", "#006400");  //녹색 (#RRGGBB 형식)


      Blynk.setProperty(V10, "isDisabled", false);  //enable , A동자동개폐LED
      Blynk.setProperty(V28, "isDisabled", false);  //enable, B동자동개폐LED
    }
  }
}

//NTP요일을 Blynk요일로 변경
int convertNtpDayToBlynkDay(int ntpDay) {
  return (ntpDay == 0) ? 7 : ntpDay;  // 일요일(0) → 7, 나머지는 그대로
}


/*
  0.자동모드 이고 요일이 맞고 시간이 맞으면 작동한다.
  1.Blynk 요일은  일(7),월(1),화(2),수(3),목(4),금(5),토(6) 이다.
*/
void waterTimeSchedule() {

  if (RUN_MODE == AUTO_MODE) {
    currentMillis = millis();
    elapsedSeconds += (currentMillis - lastUpdateMillis) / 1000;  // 초 단위 증가
    lastUpdateMillis = currentMillis;
    // 현재 시간 계산
    unsigned long epochTime = elapsedSeconds;

    int HH = (epochTime / 3600) % 24;
    int MM = (epochTime / 60) % 60;
    int SS = epochTime % 60;
    long gettime = (3600L * HH) + (60L * MM) + SS;

    int server_day = convertNtpDayToBlynkDay(timeClient.getDay());  //ntp요일을 blynk요일로 변경

    server_time = String(gettime);

#ifdef DEBUG_MODE
    Serial.println("startTimerA1:" + startTimerA1);
    Serial.println("server_time:" + server_time);
    Serial.println("endTimerA1:" + endTimerA1);
    Serial.println("server_day:" + String(server_day));
    Serial.println("selectedWeekA1:" + String(selectedWeekA1));
#endif

    //Timer1
    if (containsSubstring(selectedWeekA1, String(server_day)) && (startTimerA1.toInt() <= server_time.toInt()) && (endTimerA1.toInt() >= server_time.toInt())) {

      if (!AHouse_WaterTimer1_On) {  //Timer1과 Timer2 가 동시에 On일때 Relay을 On시킨다.
        AHouse_WaterTimer1_On = true;
      }

      if (!AHouse_WaterWiget1_On) {  //처음 OFF -> On일때 색변결
        AHouse_WaterWiget1_On = true;
        Blynk.setProperty(V7, "color", "#FF0000");  // 빨간색 (#RRGGBB 형식)
#ifdef BLYNK_MODE
        Serial.println("A동 관수타이머1 위젯의 색깔을 변경하였습니다.");
#endif
      }

    } else {
      AHouse_WaterTimer1_On = false;
      if (AHouse_WaterWiget1_On) {  // On -> OFF일때만 색변경
        AHouse_WaterWiget1_On = false;
        Blynk.setProperty(V7, "color", "#006400");  //녹색 (#RRGGBB 형식)
#ifdef BLYNK_MODE
        Serial.println("A동 관수타이머1 위젯의 색깔을 복원하였습니다.");
#endif
      }
    }

    //Timer2
    if (containsSubstring(selectedWeekA2, String(server_day)) && (startTimerA2.toInt() <= server_time.toInt()) && (endTimerA2.toInt() >= server_time.toInt())) {

      if (!AHouse_WaterTimer2_On) {  //Timer1과 Timer2 가 동시에 On일때 Relay을 On시킨다.
        AHouse_WaterTimer2_On = true;
      }

      if (!AHouse_WaterWiget2_On) {  //처음 OFF -> On일때 색변결
        AHouse_WaterWiget2_On = true;
        Blynk.setProperty(V8, "color", "#FF0000");  // 빨간색 (#RRGGBB 형식)
#ifdef BLYNK_MODE
        Serial.println("A동 관수타이머2 위젯의 색깔을 변경하였습니다.");
#endif
      }

    } else {
      AHouse_WaterTimer2_On = false;
      if (AHouse_WaterWiget2_On) {
        AHouse_WaterWiget2_On = false;
        Blynk.setProperty(V8, "color", "#006400");  //녹색 (#RRGGBB 형식)
#ifdef BLYNK_MODE
        Serial.println("A동 관수타이머2 위젯의 색깔을 복원하였습니다.");
#endif
      }
    }

    if (AHouse_WaterTimer1_On || AHouse_WaterTimer2_On) {  //Timer 1 또는 Timer2 둘중에 하나만 ON
      if (!AHouse_WaterRelay_On) {                         // Relay가 Off일때만 On시킨다.
        AHouse_WaterRelay_On = true;
        cfnet.digitalWrite(0, 6, HIGH);
        logEvent_Enqueue("infoalert", "A동하우스에 관수를 시작하였습니다.");
      }
    } else {
      if (AHouse_WaterRelay_On) {  //관수가 On 상태인 상태에서만 유효하다.
        AHouse_WaterRelay_On = false;
        cfnet.digitalWrite(0, 6, LOW);
        logEvent_Enqueue("infoalert", "A동하우스에 관수를 종료하였습니다.");
      }
    }

    //Timer3
    if (containsSubstring(selectedWeekB1, String(server_day)) && (startTimerB1.toInt() <= server_time.toInt()) && (endTimerB1.toInt() >= server_time.toInt())) {

      if (!BHouse_WaterTimer3_On) {  //Timer3과 Timer4 가 동시에 On일때 Relay을 On시킨다.
        BHouse_WaterTimer3_On = true;
      }

      if (!BHouse_WaterWiget3_On) {  //처음 OFF -> On일때 색변결
        BHouse_WaterWiget3_On = true;
        Blynk.setProperty(V25, "color", "#FF0000");  // 빨간색 (#RRGGBB 형식)
#ifdef BLYNK_MODE
        Serial.println("B동 관수타이머3 위젯의 색깔을 변경하였습니다.");
#endif
      }
    } else {
      BHouse_WaterTimer3_On = false;
      if (BHouse_WaterWiget3_On) {  // On -> OFF일때만 색변경
        BHouse_WaterWiget3_On = false;
        Blynk.setProperty(V25, "color", "#006400");  //녹색 (#RRGGBB 형식)
#ifdef BLYNK_MODE
        Serial.println("B동 관수타이머3 위젯의 색깔을 복원하였습니다.");
#endif
      }
    }

    //Timer4
    if (containsSubstring(selectedWeekB2, String(server_day)) && (startTimerB2.toInt() <= server_time.toInt()) && (endTimerB2.toInt() >= server_time.toInt())) {

      if (!BHouse_WaterTimer4_On) {  //Timer3과 Timer4 가 동시에 On일때 Relay을 On시킨다.
        BHouse_WaterTimer4_On = true;
      }

      if (!BHouse_WaterWiget4_On) {  //처음 OFF -> On일때 색변결
        BHouse_WaterWiget4_On = true;
        Blynk.setProperty(V26, "color", "#FF0000");  // 빨간색 (#RRGGBB 형식)
#ifdef BLYNK_MODE
        Serial.println("B동 관수타이머4 위젯의 색깔을 변경하였습니다.");
#endif
      }
    } else {
      BHouse_WaterTimer4_On = false;

      if (BHouse_WaterWiget4_On) {
        BHouse_WaterWiget4_On = false;
        Blynk.setProperty(V26, "color", "#006400");  //녹색 (#RRGGBB 형식)
#ifdef BLYNK_MODE
        Serial.println("B동 관수타이머4 위젯의 색깔을 복원하였습니다.");
#endif
      }
    }

    if (BHouse_WaterTimer3_On || BHouse_WaterTimer4_On) {  //Timer 3 또는 Timer4 둘중에 하나만 ON
      if (!BHouse_WaterRelay_On) {                         // Relay가 Off일때만 On시킨다.
        BHouse_WaterRelay_On = true;
        cfnet.digitalWrite(1, 6, HIGH);
        logEvent_Enqueue("infoalert", "B동하우스에 관수를 시작하였습니다.");
      }
    } else {
      if (BHouse_WaterRelay_On) {
        BHouse_WaterRelay_On = false;
        cfnet.digitalWrite(1, 6, LOW);
        logEvent_Enqueue("infoalert", "B동하우스에 관수를 종료하였습니다.");
      }
    }
  }
}

/*
  BLYNK_CONNECTED() 함수는 재부팅 되거나 재연결될때 실행된다.
  모든값은 초기값으로 재설정된다.
*/
BLYNK_CONNECTED() {
  isConnected = true;
  Serial.println("-- 서버에 연결되었습니다. --");
  Serial.print("연결된 IP 주소: ");
  Serial.println(Ethernet.localIP());  // Ethernet 모듈의 IP 출력
  logEvent_Enqueue("infoalert", "서버와 연결되었습니다.");
  setInitialMode();
}

// 아두이노 메가 소프트웨어 재부팅 함수
void rebootJmp() {
  Serial.println("시스탬을 rebootJmp 시킵니다.");
  asm volatile("  jmp 0");  // 메가의 프로그램 카운터를 0으로 강제 이동
}

void resetW5100() {
  Serial.println("시스탬을 reset 시킵니다.");
  pinMode(W5100_RESET_PIN, OUTPUT);
  digitalWrite(W5100_RESET_PIN, LOW);   // W5100 전원 끄기
  delay(500);                           // 0.5초 대기
  digitalWrite(W5100_RESET_PIN, HIGH);  // W5100 전원 켜기
  delay(1000);                          // 안정화 시간 대기
}


void checkConnection() {
  if (!Blynk.connected()) {

    Serial.println("연결이 끊어져서 시스탬을 강제로  리부팅합니다.");
    Blynk.disconnect();
    logEvent_Enqueue("waringalert", "연결이 끊어져서 시스탬을 강제로  리부팅합니다.");
    resetW5100();
    //rebootJmp();
  }
}

void ntpUpateTime() {

#ifdef DEBUG_MODE
  Serial.println("NTP 업데이트 중...");
#endif
  timeClient.update();
  elapsedSeconds = timeClient.getEpochTime();
}

void setup() {

  Serial.begin(115200);
  Ethernet.begin(mac, ip);  // Ethernet 초기화
  Serial.print("고정된 IP 주소: ");
  Serial.println(Ethernet.localIP());  // Ethernet 모듈의 IP 출력

  //고정된 IP을 받기위해 Blynk.begin 아닌 Blynk.config을 사용
  Blynk.config(BLYNK_AUTH_TOKEN);  //서버 설정 (자동 연결 X)

  /* BLYNK_CONNECTED() 에서 기본적인 초기화를 하므로 
   BLYNK_CONNECTED() 이벤트 발생까지 대기하지않고 그대로 다음단계로 가면 
   초기화 되지 않는상태로 지나면서 프로그램이 오류가 남
*/
  while (!isConnected) {
    Blynk.run();  // Blynk 이벤트 처리 (반드시 호출해야 함)
    delay(100);
  }


  timeClient.begin();
  timeClient.update();
  lastUpdateMillis = millis();
  elapsedSeconds = timeClient.getEpochTime();  // 현재 Epoch 시간 저장

  dhtE.begin();
  dhtA.begin();
  dhtB.begin();

  ntpUpdateTimer.setInterval(ntpUpdateinterval, ntpUpateTime);                  // 5분마다 NTP업데이트
  connectionTimer.setInterval(connectionCheckinterval, checkConnection);        // 1초마다 연결 상태 확인
  blnklogEventTimer.setInterval(blnklogEventTimerinterval, processEventQueue);  // 2초마다 실행
}

void loop() {

  Blynk.run();
  Ethernet.maintain();  // W5100 이더넷 연결 유지
  ntpUpdateTimer.run();
  connectionTimer.run();
  blnklogEventTimer.run();

  sensorBaeJoneBan();
  sendSensorAHouse();
  sendSensorBHouse();
  waterTimeSchedule();
}
// Подключение необходимых библиотек
#include <Arduino.h>          // Базовые функции Arduino
#include <IRremoteESP8266.h>  // Библиотека для работы с ИК-протоколами
#include <IRsend.h>           // Библиотека для отправки ИК-сигналов
#include <HomeSpan.h>         // Библиотека для интеграции с Apple HomeKit

// Настройки ШИМ для ИК-светодиода
#define IR_SEND_DUTY_CYCLE 80 // Рабочий цикл 80% для лучшей стабильности сигнала

// ИК-коды для кондиционера Bosch
const uint64_t POWER_ON = 0xB27BE1;  // Код включения питания
const uint64_t POWER_OFF = 0xB27BE0; // Код выключения питания

// ИК-код для телевизора Samsung
const uint64_t TV_POWER = 0xE0E040BF; // Код питания телевизора

// Аппаратные настройки
const uint16_t kIrLed = 14;    // Используем GPIO14 для ИК-светодиода, PIN D14
IRsend irsend(kIrLed);         // Инициализация объекта для отправки ИК-сигналов

// Перечисление режимов работы кондиционера
enum Mode { 
  OFF,    // 0 - Выключен
  COOL,   // 1 - Охлаждение
  HEAT,   // 2 - Обогрев
  AUTO    // 3 - Авторежим (Осушение)
};

// Прототип функции для отправки команд кондиционеру
void sendBoschCommand(Mode mode, uint8_t temp);

// Структура для сопоставления температуры с ИК-кодом
struct TempCode {
  uint8_t celsius;  // Температура в градусах
  uint16_t code;    // Соответствующий ИК-код
};

// Таблица соответствия температур и кодов
TempCode tempCodes[] = {
  {16, 0x00FF}, {17, 0x04FB}, {18, 0x14EB}, {19, 0x34CB},
  {20, 0x28D7}, {21, 0x6897}, {22, 0x7887}, {23, 0x58A7},
  {24, 0x48B7}, {25, 0xC837}, {26, 0xD827}, {27, 0x9867},
  {28, 0x8C73}, {29, 0xAC53}, {30, 0xBC43}
};

// Класс для управления телевизором через HomeKit
class TV_Controller : public Service::Switch {
  public:
    SpanCharacteristic *power; // Характеристика состояния питания
    
    TV_Controller() : Service::Switch() {
      power = new Characteristic::On();     // Создание характеристики On/Off
      new Characteristic::Name("TV Power"); // Задание имени сервиса
    }
  
    // Обработчик изменений состояния
    boolean update() override {
      if(power->getNewVal()) {
        irsend.sendSAMSUNG(TV_POWER, 32, 3); // Отправка кода Samsung
        power->setVal(0); // Автоматическое выключение тумблера
      }
      return true;
    }
};

// Класс для управления кондиционером через HomeKit
class AC_Controller : public Service::Thermostat {
public:
  // Характеристики HomeKit
  SpanCharacteristic *power;         // Активное состояние
  SpanCharacteristic *currentTemp;   // Текущая температура
  SpanCharacteristic *targetTemp;    // Целевая температура
  SpanCharacteristic *currentState;  // Текущий режим
  SpanCharacteristic *targetState;   // Целевой режим
  
  Mode lastMode = COOL;    // Последний выбранный режим
  uint8_t lastTemp = 22;   // Последняя установленная температура

  AC_Controller() : Service::Thermostat() {
    // Инициализация характеристик
    power = new Characteristic::Active(0);                 // Выкл по умолчанию
    currentTemp = new Characteristic::CurrentTemperature(22.0); // Начальная темп.
    targetTemp = new Characteristic::TargetTemperature(22.0, true); // Целевая темп.
    targetTemp->setRange(16, 30, 1);  // Диапазон 16-30°C с шагом 1
    
    currentState = new Characteristic::CurrentHeatingCoolingState(0);
    currentState->setValidValues(0, 2); // Допустимые значения: Off, Cool, Heat
    
    targetState = new Characteristic::TargetHeatingCoolingState(0, true);
    targetState->setValidValues(0, 3); // Допустимые значения: Off, Cool, Heat, Auto
  }

  // Обработчик обновлений характеристик
  boolean update() override {
    // Обработка изменения питания
    if (power->updated()) {
        if (power->getNewVal()) {
            sendBoschCommand(lastMode, lastTemp); // Включить с предыдущими настройками
        } else {
            sendPowerCommand(false); // Отправить команду выключения
            currentState->setVal(0); // Обновить состояние
            targetState->setVal(0);
        }
        Serial.printf("[STATE] Power: %d\n", power->getNewVal());
    }

    // Обработка изменения режима или температуры
    if (targetState->updated() || targetTemp->updated()) {
        Mode newMode = (Mode)targetState->getNewVal(); // Новый режим
        lastTemp = targetTemp->getNewVal();             // Новая температура

        if (newMode != OFF) {
            lastMode = newMode; // Сохранить режим
            power->setVal(1);   // Включить питание
            currentState->setVal(newMode == AUTO ? 2 : newMode); // Обновить состояние
            sendBoschCommand(newMode, lastTemp); // Отправить команду
        } else {
            power->setVal(0);          // Выключить питание
            sendPowerCommand(false);   // Отправить команду выключения
            currentState->setVal(0);   // Обновить состояние
        }

        Serial.printf("[STATE] Mode: %d, Temp: %d\n", newMode, lastTemp);
        currentTemp->setVal(lastTemp); // Обновить текущую температуру
    }
    return true;
  }

  // Отправка команды питания
  void sendPowerCommand(bool state) {
    Serial.printf("[IR] Power %s\n", state ? "ON" : "OFF");
    irsend.sendCOOLIX(state ? POWER_ON : POWER_OFF); // Отправка кода Coolix
    delay(100); // Задержка для стабильности
  }
};

// Поиск ИК-кода для заданной температуры
uint16_t getTempCode(uint8_t temp) {
  for(auto &tc : tempCodes) {
    if(tc.celsius == temp) return tc.code;
  }
  return 0x8C73; // Код по умолчанию (28°C)
}

// Формирование и отправка команды для кондиционера
void sendBoschCommand(Mode mode, uint8_t temp) {
  if (mode == OFF) return; // Выход если режим OFF

  uint8_t packet[18] = {0}; // Инициализация пакета нулями
  packet[0] = 0xB2;         // Стартовый байт
  packet[1] = 0x4D;         // Стартовый байт

  // Заполнение байтов режима
  switch(mode) {
    case COOL:
      packet[2] = 0xCF; packet[3] = 0x30; // Код режима Cool
      break;
    case HEAT:
      packet[2] = 0xBF; packet[3] = 0x40; // Код режима Heat
      break;
    case AUTO:
      packet[2] = 0x1F; packet[3] = 0xE0; // Код режима Dry (Осушение)
      break;
    default: return;
  }

  // Добавление кода температуры
  uint16_t tc = getTempCode(temp);
  packet[4] = (tc >> 8) & 0xFF; // Старший байт температуры
  packet[5] = tc & 0xFF;         // Младший байт температуры
  memcpy(&packet[6], &packet[0], 6); // Дублирование первых 6 байт
  
  // Завершение формирования пакета
  packet[12] = 0xD5;             // Контрольный байт
  packet[13] = (mode == AUTO) ? 0x65 : 0x66; // Байт режима
  packet[16] = 0x00;             // Резервный байт
  packet[17] = (mode == AUTO) ? 0x3A : 0x3B; // Контрольная сумма

  Serial.printf("[IR] Sending %s, %d°C\n", 
    mode == COOL ? "COOL" : 
    mode == HEAT ? "HEAT" : "DRY", 
    temp);
    
  irsend.sendBosch144(packet, 18, 3); // Отправка пакета протоколом Bosch 144
}

// Начальная настройка устройства
void setup() {
  Serial.begin(115200);     // Инициализация последовательного порта
  irsend.begin();           // Инициализация ИК-передатчика
  
  homeSpan.begin(Category::Bridges, "HomeSpan AC Bridge"); // Инициализация HomeSpan

  // Создание аксессуара-моста
  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("Bridge");
      new Characteristic::Manufacturer("HomeSpan");
      new Characteristic::SerialNumber("BS-001");
      new Characteristic::Model("1.0");
      new Characteristic::FirmwareRevision("1.0");
      new Characteristic::Identify();

  // Создание аксессуара кондиционера
  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("AC Controller");
      new Characteristic::Manufacturer("Kentacu");
      new Characteristic::SerialNumber("AC-123");
      new Characteristic::Model("Climate Pro");
      new Characteristic::FirmwareRevision("3.0");
      new Characteristic::Identify();
    
    new AC_Controller(); // Добавление сервиса кондиционера

  // Создание аксессуара телевизора
  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("TV Controller");
      new Characteristic::Manufacturer("Samsung");
      new Characteristic::SerialNumber("TV-001");    
      new Characteristic::Model("QLED 2023");
      new Characteristic::FirmwareRevision("1.0");
      new Characteristic::Identify();
  
    new TV_Controller(); // Добавление сервиса телевизора

  Serial.println("System initialized"); // Сообщение о готовности
}

// Основной цикл программы
void loop() {
  homeSpan.poll(); // Обработка HomeSpan событий
  delay(10);       // Короткая задержка
}
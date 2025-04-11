# Управление кондиционером Kentatsu через HomeKit с интеграцией Samsung TV

Проект позволяет управлять кондиционером Kentatsu через Apple HomeKit, используя ESP32 в качестве IR-трансивера. Дополнительно реализовано управление телевизором Samsung.

## 🔮 Основные функции
- **Интеграция с Apple HomeKit** через библиотеку HomeSpan
- Поддержка **протоколов Coolix и Bosch** для кондиционера
- **Кастомные характеристики**:
  - Режимы работы (охлаждение/обогрев)
  - Управление температурой
- **Дополнительный функционал**:
  - Управление телевизором Samsung через IR


## 🛠️ Используемые компоненты
- Плата ESP32
- ИК-светодиод для передачи
- Библиотеки:
  - [HomeSpan](https://github.com/HomeSpan/HomeSpan)
  - [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266)
  
## 📥 Установка
1. Клонируйте репозиторий:
```bash
git clone https://github.com/Kapral1990/IR_air_Kentatsu.git
```bash

# Control Kentatsu Air Conditioner via HomeKit with Samsung TV Integration

This project enables control of a Kentatsu air conditioner through Apple HomeKit using an ESP32 as an IR transceiver. Additionally implements Samsung TV control functionality.

## 🔮 Key Features
- **Apple HomeKit integration** via HomeSpan library
- Support for **Coolix and Bosch protocols** for air conditioner
- **Custom Characteristics**:
  - Operation modes (Cool/Heat)
  - Temperature control
- **Additional Functionality**:
  - Samsung TV control via IR

## 🛠️ Components Used
- ESP32 board
- IR LED transmitter
- Libraries:
  - [HomeSpan](https://github.com/HomeSpan/HomeSpan)
  - [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266)

## 📥 Installation
1. Clone the repository:
```bash
git clone https://github.com/Kapral1990/IR_air_Kentatsu.git

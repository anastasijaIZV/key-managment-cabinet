# Key managment cabinet

## Idea description

The idea of this project is to develop a smart key management system for a school that automates the process of issuing and returning physical keys. The system is based on a closed key cabinet equipped with electronic access control. Each key is attached to an NFC tag with a unique identifier, and users authenticate themselves using their school ID card. Only authorized users are allowed to unlock the cabinet and take a specific key.

The system operates without the need for a responsible person to manually distribute keys, which reduces human error and digitizes the data for easy acccess. All actions, such as successful authentication, denied access, key removal, and key return, are logged automatically in a database. The system provides visual feedback using LEDs to indicate whether access is granted or denied.

The solution is designed to be scalable, allowing it to be adapted for different cabinet sizes, user groups, or institutions. By combining hardware, software, and cloud-based data logging, the project demonstrates a practical Internet of Things (IoT) application that solves a real-world problem in an efficient and modern way.

---

## Problem analysis
### Target audience/users:
- Primary users of the system are school staff, such as teachers, administrators, and technical personnel, as well as students who are authorized to access specific rooms. The system is especially useful in environments where many keys are shared among different users throughout the day.
### Why it is worth developing this solution?
- In many schools, key management is still handled manually, often relying on a logbook or a single responsible person. This approach is inefficient and prone to errors, as keys can be lost, borrowed without permission, or returned without proper tracking. In some cases, it is impossible to determine who last used a key, which can lead to security risks.

Developing an automated key management system improves security, transparency, and efficiency. It ensures that only authorized users can access keys and provides a clear record of all key-related activities. Additionally, the system reduces administrative workload and can operate continuously without supervision, making it a valuable and practical solution for modern educational institutions.

---

## Technologies used
### Delivery format:
- Physical system with a closed cabinet
- Web-based data storage using Google Sheets as a lightweight database

### Hardware technologies
- ESP32 microcontroller
- PN532 NFC/RFID reader
- Electronic cabinet lock with feedback signal
- MOSFET driver module for lock control
- NFC tags for keys
- Status LEDs

### Software technologies
- Arduino IDE (ESP32 Arduino core)
- Google Apps Script (server endpoint)
- HTTP communication over Wi-Fi

--- 

## 10-day plan (1.5h)

| Day | Tag       | Todo task                                                                 | Status |
|-----|-----------|---------------------------------------------------------------------------|--------|
| 1  | schema    | Draw system diagram (hardware + data flow)              | ✍(◔◡◔)    |
| 1   | hardware  | Install ESP32 drivers and confirm board powers on via USB                 | ┗( T﹏T )┛     |
| 1   | software  | Install Arduino IDE ESP32 support and upload basic Serial test sketch     | (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧      |
| 2   | hardware  | Wire PN532 to ESP32 (SPI mode) and double-check pin connections            | (_　_)。゜zｚＺ     |
| 3   | software  | Run PN532 example and read UID from a card/tag via Serial Monitor          | (_　_)。゜zｚＺ     |
| 4   | hardware  | Wire red and green LEDs to ESP32 GPIOs and test basic on/off control       | (_　_)。゜zｚＺ     |
| 4   | software  | Write LED status functions (locked, unlocked, error)                      | (_　_)。゜zｚＺ     |
| 5   | hardware  | Wire lock to 12V PSU through IRF520 MOSFET and add flyback diode           | (_　_)。゜zｚＺ     |
| 5   | hardware  | Test lock unlock/lock manually using ESP32 GPIO                            | (_　_)。゜zｚＺ     |
| 6   | hardware  | Connect lock feedback signal to ESP32 and verify logic (locked/unlocked)  | (_　_)。゜zｚＺ     |
| 6   | software  | Read feedback pin in code and print lock state to Serial                   | (_　_)。゜zｚＺ     |
| 7   | software  | Implement basic access logic (card read → unlock → delay → relock)        | (_　_)。゜zｚＺ     |
| 7   | software  | Combine NFC + LEDs + lock into one sketch                          | (_　_)。゜zｚＺ     |
| 8   | software  | Create Google Sheets + Apps Script endpoint                                | (_　_)。゜zｚＺ     |
| 8   | software  | Send test log event from ESP32 to Google Sheets                            | (_　_)。゜zｚＺ     |
| 9   | software  | Log real events (AUTH_OK, AUTH_FAIL, LOCK, UNLOCK) to Sheets               | (_　_)。゜zｚＺ     |
| 9   | schema    | Define event naming and data structure (UIDs, timestamps, device ID)      | (_　_)。゜zｚＺ     |
| 10  | hardware  | Mount components inside cabinet            | (_　_)。゜zｚＺ     |

---

#### ~ Status meaning: 
- NOT STARTED (_　_)。゜zｚＺ
- IN PROGRESS ✍(◔◡◔)
- IN SLOW PROGRESS ┗( T﹏T )┛
- NOT WORKING (ㆆ_ㆆ)
- TO BE DECIDED ㄟ( ▔, ▔ )ㄏ
- FINISHED (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧ 

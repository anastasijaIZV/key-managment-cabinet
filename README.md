# key-managment-cabinet

## ~ Idea description

The idea of this project is to develop a smart key management system for a school that automates the process of issuing and returning physical keys. The system is based on a closed key cabinet equipped with electronic access control. Each key is attached to an NFC tag with a unique identifier, and users authenticate themselves using their school ID card. Only authorized users are allowed to unlock the cabinet and take a specific key.

The system operates without the need for a responsible person to manually distribute keys, which reduces human error and digitizes the data for easy acccess. All actions, such as successful authentication, denied access, key removal, and key return, are logged automatically in a database. The system provides visual feedback using LEDs to indicate whether access is granted or denied.

The solution is designed to be scalable, allowing it to be adapted for different cabinet sizes, user groups, or institutions. By combining hardware, software, and cloud-based data logging, the project demonstrates a practical Internet of Things (IoT) application that solves a real-world problem in an efficient and modern way.
---

## ~ Problem analysis
### Target audience/users:
- Primary users of the system are school staff, such as teachers, administrators, and technical personnel, as well as students who are authorized to access specific rooms. The system is especially useful in environments where many keys are shared among different users throughout the day.
### Why it is worth developing this solution?
- In many schools, key management is still handled manually, often relying on a logbook or a single responsible person. This approach is inefficient and prone to errors, as keys can be lost, borrowed without permission, or returned without proper tracking. In some cases, it is impossible to determine who last used a key, which can lead to security risks.

Developing an automated key management system improves security, transparency, and efficiency. It ensures that only authorized users can access keys and provides a clear record of all key-related activities. Additionally, the system reduces administrative workload and can operate continuously without supervision, making it a valuable and practical solution for modern educational institutions.
---

## ~ Technologies used
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

## ~ 10-day plan (1.5h)

| Day | Tag       | Todo task                                                                 | Status |
|-----|-----------|---------------------------------------------------------------------------|--------|
| 1  | schema    | Draw system diagram (hardware + data flow) for documentation              | ⬜     |
| 1   | hardware  | Install ESP32 drivers and confirm board powers on via USB                 | ⬜     |
| 2   | software  | Install Arduino IDE ESP32 support and upload basic Serial test sketch     | ⬜     |
| 2   | software  | Verify stable upload           | ⬜     |
| 3   | hardware  | Wire PN532 to ESP32 (SPI mode) and double-check pin connections            | ⬜     |
| 3   | software  | Run PN532 example and read UID from a card/tag via Serial Monitor          | ⬜     |
| 4   | hardware  | Wire red and green LEDs to ESP32 GPIOs and test basic on/off control       | ⬜     |
| 4   | software  | Write LED status functions (locked, unlocked, error)                      | ⬜     |
| 5   | hardware  | Wire lock to 12V PSU through IRF520 MOSFET and add flyback diode           | ⬜     |
| 5   | hardware  | Test lock unlock/lock manually using ESP32 GPIO                            | ⬜     |
| 6   | hardware  | Connect lock feedback signal to ESP32 and verify logic (locked/unlocked)  | ⬜     |
| 6   | software  | Read feedback pin in code and print lock state to Serial                   | ⬜     |
| 7   | software  | Implement basic access logic (card read → unlock → delay → relock)        | ⬜     |
| 7   | software  | Combine NFC + LEDs + lock into one sketch                          | ⬜     |
| 8   | software  | Create Google Sheets + Apps Script endpoint                                | ⬜     |
| 8   | software  | Send test log event from ESP32 to Google Sheets                            | ⬜     |
| 9   | software  | Log real events (AUTH_OK, AUTH_FAIL, LOCK, UNLOCK) to Sheets               | ⬜     |
| 9   | schema    | Define event naming and data structure (UIDs, timestamps, device ID)      | ⬜     |
| 10  | hardware  | Mount components inside cabinet            | ⬜     |

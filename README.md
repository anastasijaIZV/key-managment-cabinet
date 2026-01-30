# Key managment cabinet

---

## Idea description

The idea of this project is to develop a **smart key management system for a school that automates the process of issuing and returning physical keys**. The system is based on a closed key cabinet equipped with electronic access control.

Each key is attached to an NFC tag with a unique identifier, and users authenticate themselves using their school ID card. Only authorized users are allowed to unlock the cabinet and take a specific key. The system operates without the need for a responsible person to manually distribute keys, which reduces human error and digitizes the data for easy acccess. All actions, such as successful authentication, denied access, key removal, and key return, are logged automatically in a database. The system provides visual feedback using LEDs to indicate whether access is granted or denied. The solution is designed to be scalable, allowing it to be adapted for different cabinet sizes, user groups, or institutions.

---

## Problem analysis
### Target audience/users:
- Primary users of the system are school staff, such as teachers, administrators, and technical personnel, as well as students who are authorized to access specific rooms. The system is especially useful in environments where many keys are shared among different users throughout the day.

### Why it is worth developing this solution?
In many schools, key management is still handled manually, often relying on a logbook or a single responsible person. This approach is inefficient and prone to errors, as keys can be lost, borrowed without permission, or returned without proper tracking. In some cases, it is impossible to determine who last used a key, which can lead to security risks.

- Developing an automated key management system improves security, transparency, and efficiency. It ensures that only authorized users can access keys and provides a clear record of all key-related activities.
- The system reduces administrative workload and can operate continuously without supervision, making it a valuable and practical solution for modern educational institutions.

---

## Technologies used
### Delivery format:
- Physical system with a closed cabinet
- Web-based data storage using Google Sheets

### Hardware technologies
- ESP32 microcontroller
- PN532 NFC/RFID reader
- School ID Card
- Electronic cabinet lock with feedback signal
- MOSFET driver module for lock control
- NFC tags for keys
- Status LEDs (green + red)
- 3D printer (model)

### Software technologies
- Arduino IDE
- Google Apps Script
- HTTP communication over Wi-Fi
- Tinkercad

--- 

## Work plan

### Deliverables:
- [ ] **Week 1** &rarr; finalised written concept + system architecture

- [ ] **Week 2** &rarr;  printable cabinet piece models

- [ ] **Week 3** &rarr;  printed cabinet + ESC32 development environment

- [ ] **Week 4** &rarr;  serial shows correct UID

- [ ] **Week 5** &rarr;  visual feedback for system state

- [ ] **Week 6** &rarr; cabinet can lock/unlock

- [ ] **Week 7** &rarr;  combined system parts (taps card - cabinet opens - relocks automanically) 

- [ ] **Week 8** &rarr;  assembled cabinet prototype

- [ ] **Week 9** &rarr;  live event log visible in browser

- [ ] **Week 10** &rarr;  demo ready and finished documentation


| Week | Tag (S-school, H-home)     | Todo task                                                                 | Status |
|-----|-----------|---------------------------------------------------------------------------|--------|
| 1   | S    | Finalize system concept + requirements              | ✍(◔◡◔)    |
| 1   | H    | Draw full system diagram            | (_　_)。zＺ    |
| 2   | S  | Design cabinet layout     | (_　_)。zＺ      |
| 2   | H  | Create 3D model of cabinet           | (_　_)。zＺ     |
| 3   | S  | Print the parts for the cabinet          | (_　_)。zＺ     |
| 3   | S  | Install ESP32 drivers and Arduino IDE ESP32 support (test board power)      | (_　_)。zＺ     |
| 4   | S  | Wire PN532 and ESP32 and verify connection                      | (_　_)。zＺ     |
| 4   | H  | Run PN532 examples, read UID from school card + test tag           | (_　_)。zＺ     |
| 5   | S  | Wire red/green LEDs to ESP32 GPIOs                            | (_　_)。zＺ     |
| 5   | H  | Create LED states  | (_　_)。zＺ     |
| 6   | S  | Wire lock to 12V PSU via IRF520 + flyback diode                   | (_　_)。zＺ     |
| 6   | H  | Test lock control via GPIO        | (_　_)。zＺ     |
| 7   | S  | Implement access flow  (card - unlock - delay - relock)                        | (_　_)。zＺ     |
| 7   | H  | Combine NFC + LEDs + lock into one sketch                                | (_　_)。zＺ     |
| 8   | S  | Mount electronics inside printed cabinet                            | (_　_)。zＺ     |
| 8   | H  | Add door/lock feedback signal and read it in code               | (_　_)。zＺ     |
| 9   | S    | Define event structure      | (_　_)。zＺ     |
| 9  | H  | Create Google Sheets + Apps Script endpoint            | (_　_)。zＺ     |
| 9  | H  | Send test logs from ESP32            |(_　_)。zＺ     |
| 10  | S  | Full system testing            |(_　_)。zＺ     |
| 10  | S  | Finish documentation             | (_　_)。zＺ     |

---

> ### Status codes: 
> - NOT STARTED (_　_)。zＺ
> - IN PROGRESS ✍(◔◡◔)
> - IN SLOW PROGRESS ┗( T﹏T )┛
> - NOT WORKING (ㆆ_ㆆ)
> - TO BE DECIDED ㄟ( ▔, ▔ )ㄏ
> - FINISHED (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧ 

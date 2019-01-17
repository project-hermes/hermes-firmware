# hermes-firmware
[![Build Status](https://travis-ci.org/project-hermes/hermes-firmware.svg?branch=master)](https://travis-ci.org/project-hermes/hermes-firmware)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/3a73dc4f05684f4e8d8ff7b8446ddbcb)](https://www.codacy.com/app/project-hermes/hermes-firmware?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=project-hermes/hermes-firmware&amp;utm_campaign=Badge_Grade)

## How to get started
1. Install VS Code https://code.visualstudio.com/
2. Install driver https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers
3.  Install Platformio https://platformio.org/install/ide?install=vscode
4. Install the ESP32 board https://platformio.org/platforms/espressif32
5. Git clone the repo https://github.com/project-hermes/hermes-firmware
6. Open the repo in platformio
7. Click on the alien head on the left side tool bar
8. Click the “update library” task on the task list in the botton left
9. Click build in the same list
10. Attach esp32 via usb to computer
11. click upload on the same task list
12. click monitor on the task list
13. click the reset button on the esp32
14. use your finger to tap pin 27
15. you should see a new wifi hotspot open called “Hermes”, connect your computer to it (pass is 12345678)
16. a login window should open
17. click add ap
18. click connect on the ap for you home
19. add your password
20. the device should restart

## DS1306 RTC Module NTP Time Sync
### Description:
Debug code on the ESP8266 NodeMCU, with pin definitions marked as constants in the code using ` #define `

## About
> Requires a network connection. After connecting the DS1302 to the development board, connect to the network. The code provides a list of NTP server addresses; if the connection fails, please modify the indexes `  [0]-[3] `. Upon success, debugging output will be shown on port`  9600 `. The DS1302 will update the time every second while syncing with NTP.
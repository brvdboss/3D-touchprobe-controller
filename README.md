# 3D Touchprobe controller

This project is designed to work with a 3D touch probe on the SnapMaker 2 device and a 3D touchprobe. DIY version can be found here: https://github.com/brvdboss/3d-touchprobe

It should work with any Arduino like device. I am using and testing it with an ESP32 (Wemos D1 mini style board) and a 3.3V CAN transceiver.  When a touch is detected, it sends the appropriate CAN message on the bus to indicate a detection of the touch.

CAN message with ID 0x605 and value 0 for detection and value 1 when no detection. Exactly like the inductive sensor in the print head.

## CAN Monitor
To make it easier to monitor what's happening on the CAN bus a web interface has been added that monitors all messages on the CAN bus. The ESP logs these and sends them through a websocket to the browser. It shows both a full log of all messages as well as a table with for each PacketID, the data paylowds and a counter how often this particular message has been sent.

No data is stored in the ESP, it's just broadcasting it through the websocket. This also means that if you refresh the browser window, the historical data is gone.  However, this does make it easy to test certain things. Refresh the browser, perform the actions you want on the device. (e.g. do a Go Home) And see which messages appear on the CAN bus. (in this case, all the end stops).

![Screenshot](pictures/CANMonitor.png "Screenshot")

The ESP will print the ip-address on the Serial console. Open that in your browser and you should be good to go. Obviously you should not allow this to be accessible from the Internet.

## Requirements

### Software
This project uses platformIO (https://platformio.org/) and this CAN library(https://github.com/sandeepmistry/arduino-CAN)

VSCode with platformIO plugin is my development environment. The platformIO config should make sure you download all dependecies automatically

### Hardware
* An ESP32 board (Wemos d1 mini style)
  * Should workk with any ESP32 style board
  * Should work with other similar boards (Arduino etc)
* CAN transceiver
  * If your board does not have a can controller built-in, you'll need an external CAN controller (MCP2515 based boards)

# Warning
This is very much work in progress!

* Right now this only works when the printhead is connected.  The G30 probe command (https://marlinfw.org/docs/gcode/G030.html) doesn't work when in CNC mode.
* The ID of the can packet seems to change (I've seen 0x604 and 0x605) and still need to figure out when and where these ID's are assigned
* 
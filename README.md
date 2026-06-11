# QWatch Companion

Flutter companion app for the QWatch ESP32-S3 smartwatch.

## Features
- BLE scan/connect/reconnect
- Watch keyboard bridge
- Weather, battery, calendar, music, find-phone
- Smart home control UI
- Codemagic-ready structure

## Notes
This app assumes the BLE protocol described in the QWatch BLE protocol spec.
If the firmware GATT server is not implemented yet, the UI still runs and the BLE layer will wait for a matching device.

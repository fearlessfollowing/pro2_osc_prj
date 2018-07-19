#!/bin/bash
adb connect 192.168.2.101
#adb shell killall monitor
#adb shell killall pro2_service
adb push out/pro2_service   /usr/local/bin/
adb shell chmod +x /usr/local/bin/pro2_service
adb shell killall pro2_service

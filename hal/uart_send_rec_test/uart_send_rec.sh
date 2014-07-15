adb remount
adb push uart_test_send /system/bin/uart_test_send
adb push uart_test_rec /system/bin/uart_test_rec

adb shell chmod 777 /system/bin/uart_test_send
adb shell chmod 777 /system/bin/uart_test_rec

# adb shell /system/bin/uart_test_rec r 4 115200 100
# adb shell /system/bin/uart_test_send 1 115200 100


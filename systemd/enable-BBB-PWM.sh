#!/bin/bash
echo am33xx_pwm > /sys/devices/bone_capemgr.*/slots
echo bone_pwm_P8_13 > /sys/devices/bone_capemgr.*/slots
echo bone_pwm_P9_14 > /sys/devices/bone_capemgr.*/slots

echo 9000 >  /sys/devices/ocp.*/pwm_test_P8_13.16/period
echo 9000 >  /sys/devices/ocp.*/pwm_test_P8_13.16/duty
echo 9000 >  /sys/devices/ocp.*/pwm_test_P9_14.18/period
echo 9000 >  /sys/devices/ocp.*/pwm_test_P9_14.18/duty
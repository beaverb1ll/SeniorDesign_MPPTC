#!/bin/bash
echo 60 > /sys/class/gpio/export
echo "out" > /sys/class/gpio/gpio60/direction
echo 1 > /sys/class/gpio/gpio60/value
echo 0 > /sys/class/gpio/gpio49/value

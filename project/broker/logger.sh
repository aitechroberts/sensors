#!/bin/sh
# Simple subscriber that *prints every payload* passing through the broker
while true; do
  mosquitto_sub -h 127.0.0.1 -p 1883 -t '#' -v
done
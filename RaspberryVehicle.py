#!/usr/bin/env python3

import re
import cv2
import time
import struct
import socket
import _thread
import threading
import RPi.GPIO as GPIO

stop = False

interval = 0.1

udp_addr = ("192.168.1.1", 6000)

GPIO.setmode(GPIO.BCM)

speedl_pin = 18
speedr_pin = 17

GPIO.setup(speedl_pin, GPIO.IN)
GPIO.setup(speedr_pin, GPIO.IN)

speed_mutex = threading.Lock()
speedl_count = 0
speedr_count = 0

def speedl_callback(pin):

    global speedl_count
    global speed_mutex

    speed_mutex.acquire()
    speedl_count = speedl_count + 1
    speed_mutex.release()

def speedr_callback(pin):

    global speedr_count
    global speed_mutex

    speed_mutex.acquire()
    speedr_count = speedr_count + 1
    speed_mutex.release()

GPIO.add_event_detect(speedl_pin, GPIO.RISING, callback = speedl_callback)
GPIO.add_event_detect(speedr_pin, GPIO.RISING, callback = speedr_callback)

ultrasound_tri_pin = 13
ultrasound_ech_pin = 16

GPIO.setup(ultrasound_tri_pin, GPIO.OUT)
GPIO.setup(ultrasound_ech_pin, GPIO.IN)

ultrasound_distance = 0.0

def ultrasound_thread():

    global stop

    global ultrasound_tri_pin
    global ultrasound_ech_pin
    global ultrasound_distance

    while not stop:

        GPIO.output(ultrasound_tri_pin, True)
        time.sleep(0.00001)
        GPIO.output(ultrasound_tri_pin, False)

        count = 10000
        while GPIO.input(ultrasound_ech_pin) != True and count > 0:
            count = count - 1

        start = time.time()

        count = 10000
        while GPIO.input(ultrasound_ech_pin) != False and count > 0:
            count = count - 1

        finish = time.time()

        pulse_len = finish - start
        ultrasound_distance = pulse_len / 0.000058

        time.sleep(interval * 0.01)

motor_mutex = threading.Lock()
motor_pins = [21, 26, 20, 19]
motor_pwms = []

GPIO.setup(motor_pins, GPIO.OUT)

for motor_index in range(0, 4):

    motor_pwms.append(GPIO.PWM(motor_pins[motor_index], 500))
    motor_pwms[motor_index].start(0)

motor_mode = 0
motor_time = 0
motor_speed = 0

def motor_thread():

    global stop

    global motor_mode
    global motor_time
    global motor_speed
    global motor_mutex

    while not stop:

        motor_mutex.acquire()

        if motor_time < time.time():

            motor_pwms[0].ChangeDutyCycle(0)
            motor_pwms[1].ChangeDutyCycle(0)
            motor_pwms[2].ChangeDutyCycle(0)
            motor_pwms[3].ChangeDutyCycle(0)

        elif motor_mode == 0:

            motor_pwms[0].ChangeDutyCycle(motor_speed)
            motor_pwms[1].ChangeDutyCycle(0)
            motor_pwms[2].ChangeDutyCycle(motor_speed)
            motor_pwms[3].ChangeDutyCycle(0)

        elif motor_mode == 1:

            motor_pwms[0].ChangeDutyCycle(0)
            motor_pwms[1].ChangeDutyCycle(motor_speed)
            motor_pwms[2].ChangeDutyCycle(0)
            motor_pwms[3].ChangeDutyCycle(motor_speed)

        elif motor_mode == 2:

            motor_pwms[0].ChangeDutyCycle(0)
            motor_pwms[1].ChangeDutyCycle(motor_speed * 0.75)
            motor_pwms[2].ChangeDutyCycle(motor_speed * 0.75)
            motor_pwms[3].ChangeDutyCycle(0)

        elif motor_mode == 3:

            motor_pwms[0].ChangeDutyCycle(motor_speed * 0.75)
            motor_pwms[1].ChangeDutyCycle(0)
            motor_pwms[2].ChangeDutyCycle(0)
            motor_pwms[3].ChangeDutyCycle(motor_speed * 0.75)

        elif motor_mode == 4:

            motor_pwms[0].ChangeDutyCycle(motor_speed)
            motor_pwms[1].ChangeDutyCycle(motor_speed)
            motor_pwms[2].ChangeDutyCycle(motor_speed)
            motor_pwms[3].ChangeDutyCycle(motor_speed)

        motor_mutex.release()

        time.sleep(interval * 0.01)


def cap_thread():

    global stop

    global udp_addr

    cap = cv2.VideoCapture(0)
    cap.set(3, 256)
    cap.set(4, 256)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    if cap == None: return

    while not stop:

        for chunk_index in range(0, 256):

            if chunk_index % 32 == 0:

                ret, frame = cap.read()
                if ret == False: return

            chunk_bytes = struct.pack("!H", chunk_index) + frame.tobytes()[chunk_index * 768 : chunk_index * 768 + 768]

            sock.sendto(chunk_bytes, udp_addr)

            if stop: return

    cap.release()
    
clp_pin = 23
near_pin = 27
is_pins = [22, 24, 25, 12, 5]

GPIO.setup(clp_pin, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)
GPIO.setup(near_pin, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)
GPIO.setup(is_pins, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)

def udp_thread():

    global stop

    global udp_addr

    global speedl_count
    global speedr_count
    global speed_mutex

    global motor_mode
    global motor_time
    global motor_speed
    global motor_mutex

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", 6000))
    sock.setblocking(False)

    while not stop:

        try:

            (data, source) = sock.recvfrom(1024)

            if len(data) == 8:

                udp_addr = source

                order, param = struct.unpack("!ii", data)

                if order >= 0 and order < 5:

                    motor_mutex.acquire()

                    motor_mode = order
                    motor_speed = param

                    if motor_speed < 0.0:

                        motor_speed = 0.0

                    elif motor_speed > 100.0:

                        motor_speed = 100.0

                    motor_time = time.time() + 0.13

                    motor_mutex.release()

        except socket.error:

            motor_mode = 0

        data = bytes()

        clp_state = GPIO.input(clp_pin) == True
        near_state = GPIO.input(near_pin) == True

        data = data + struct.pack("!??", clp_state, near_state)

        for is_pin in is_pins:

            is_state = GPIO.input(is_pin) == True

            data = data + struct.pack("!?", is_state)

        speed_mutex.acquire()

        data = data + struct.pack("!BB", speedl_count, speedr_count)

        speedl_count = 0
        speedr_count = 0

        speed_mutex.release()

        data = data + struct.pack("!f", ultrasound_distance)

        sock.sendto(data, udp_addr)

        time.sleep(interval)

    sock.close()

threads = []

try:

    threads.append(threading.Thread(target = ultrasound_thread, args = ()))
    threads.append(threading.Thread(target = motor_thread, args = ()))
    threads.append(threading.Thread(target = cap_thread, args = ()))
    threads.append(threading.Thread(target = udp_thread, args = ()))

    for thread in threads: thread.start()

    while True:

        time.sleep(interval)

except KeyboardInterrupt:

    print("\nKeyboardInterrupt")

finally:

    stop = True

    for thread in threads:

        if thread.isAlive():

            thread.join()

    GPIO.cleanup()

    print("Exit")

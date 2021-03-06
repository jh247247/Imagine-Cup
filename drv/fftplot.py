#!/usr/bin/python2.7
import serial
import time

import numpy as np
import matplotlib.pyplot as plt

ser = serial.Serial('/dev/ttyUSB0',115200*4,timeout=None)
data = ""
t = ser.read()
plt.ion()
a = plt.axes()
print(hex(ord(t)),t)
while t is not '\0': # sync
    t = ser.read()
while True:
    plt.clf()
    y = []
    data = ""

    
    while t is '\0': # sync
        t = ser.read()
    
    while t is not '\0': # grab data
        t = ser.read()
        if t is not '\0':
            data += t
        
    if data[-1:] is ',':
        data = data[:-1]
    y = [int(i) for i in data.split(',') if i]
    #y = y[880:930]

    #x = np.linspace(0,22000,len(y))
    x = range(len(y))
    #x = [i+880 for i in x]
    line, = plt.plot(x, y)
    #plt.ylim([0,100000])
    plt.draw()


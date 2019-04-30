from motor import *
from time import sleep #Only used for example

#Motor(AIN1,AIN2,APWM,BIN1,BIN2,BPWM,STANDBY,(Reverse polarity?))

test = Motor(23,24,18,16,20,21,4,False)

test.drive(100) #Forward 100% dutycycle
sleep(1)
test.drive(-100) #Backwards 100% dutycycle
sleep(1)
test.brake() #Short brake
sleep(0.1)

test.standby(True) #Enable standby
test.standby(False) #Disable standby


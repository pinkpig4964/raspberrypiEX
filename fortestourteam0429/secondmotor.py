#Raspberry Pi TB6612FNG Library
import RPi.GPIO as GPIO

#See https://raspberrypi.stackexchange.com/a/12967 for more info
GPIO.setmode(GPIO.BCM)
#GPIO.setmode(GPIO.BOARD)

class Motor:
	Ain1 = 23
	Ain2 = 24
	Apwm = 18
	Bin1 = 16
	Bin2 = 20
	Bpwm = 21
	standbyPin = 4

	#Defaults
	hertz = 1000
	reverse = False #Reverse flips the direction of the motor

	#Constructor
	def __init__(self, Ain1, Ain2, Apwm, Bin1, Bin2, Bpwm, standbyPin, reverse):
		self.Ain1 = Ain1
		self.Ain2 = Ain2
		self.Apwm = Apwm
		self.Bin1 = Bin1
		self.Bin2 = Bin2
		self.Bpwm = Bpwm
		self.standbyPin = standbyPin
		self.reverse = reverse

		GPIO.setup(Ain1,GPIO.OUT)
		GPIO.setup(Ain2,GPIO.OUT)
		GPIO.setup(Apwm,GPIO.OUT)
		GPIO.setup(Bin1,GPIO.OUT)
		GPIO.setup(Bin1,GPIO.OUT)
		GPIO.setup(Bin2,GPIO.OUT)
		GPIO.setup(Bpwm,GPIO.OUT)
		GPIO.setup(standbyPin,GPIO.OUT)
		GPIO.output(standbyPin,GPIO.HIGH)
		self.ap = GPIO.PWM(Apwm, self.hertz)
		self.bp = GPIO.PWM(Bpwm, self.hertz)
		self.ap.start(0)
		self.bp.start(0)

	#Speed from -100 to 100
	def Ldrive(self, speed):
		#Negative speed for reverse, positive for forward
		#If necessary use reverse parameter in constructor
		dutyCycle = speed
		if(speed < 0):
			dutyCycle = dutyCycle * -1

		if(self.reverse):
			speed = speed * -1

		if(speed > 0):
			GPIO.output(self.Ain1,GPIO.HIGH)
			GPIO.output(self.Ain2,GPIO.LOW)
			GPIO.output(self.Bin1,GPIO.HIGH)
			GPIO.output(self.Bin2,GPIO.LOW)
		else:
			GPIO.output(self.Ain1,GPIO.LOW)
			GPIO.output(self.Ain2,GPIO.HIGH)
			GPIO.output(self.Bin1,GPIO.LOW)
			GPIO.output(self.Bin2,GPIO.HIGH)
		self.ap.ChangeDutyCycle(dutyCycle)
		self.bp.ChangeDutyCycle(dutyCycle)
	def Rdrive(self, speed):
		#Negative speed for reverse, positive for forward
		#If necessary use reverse parameter in constructor
		dutyCycle = speed
		if(speed < 0):
			dutyCycle = dutyCycle * -1

		if(self.reverse):
			speed = speed * -1

		if(speed > 0):
			GPIO.output(self.Ain1,GPIO.HIGH)
			GPIO.output(self.Ain2,GPIO.LOW)
			GPIO.output(self.Bin1,GPIO.HIGH)
			GPIO.output(self.Bin2,GPIO.LOW)
		else:
			GPIO.output(self.Ain1,GPIO.LOW)
			GPIO.output(self.Ain2,GPIO.HIGH)
			GPIO.output(self.Bin1,GPIO.LOW)
			GPIO.output(self.Bin2,GPIO.HIGH)
		self.ap.ChangeDutyCycle(dutyCycle)
		self.bp.ChangeDutyCycle(dutyCycle)

	def drive(self, speed):
		#Negative speed for reverse, positive for forward
		#If necessary use reverse parameter in constructor
		dutyCycle = speed
		if(speed < 0):
			dutyCycle = dutyCycle * -1

		if(self.reverse):
			speed = speed * -1

		if(speed > 0):
			GPIO.output(self.Ain1,GPIO.HIGH)
			GPIO.output(self.Ain2,GPIO.LOW)
			GPIO.output(self.Bin1,GPIO.HIGH)
			GPIO.output(self.Bin2,GPIO.LOW)
		else:
			GPIO.output(self.Ain1,GPIO.LOW)
			GPIO.output(self.Ain2,GPIO.HIGH)
			GPIO.output(self.Bin1,GPIO.LOW)
			GPIO.output(self.Bin2,GPIO.HIGH)
		self.ap.ChangeDutyCycle(dutyCycle)
		self.bp.ChangeDutyCycle(dutyCycle)


	def brake(self):
		self.ap.ChangeDutyCycle(0)
		self.bp.ChangeDutyCycle(0)
		GPIO.output(self.Ain1,GPIO.HIGH)
		GPIO.output(self.Ain2,GPIO.HIGH)
		GPIO.output(self.Bin1,GPIO.HIGH)
		GPIO.output(self.Bin2,GPIO.HIGH)

	def standby(self, value):
		self.ap.ChangeDutyCycle(0)
		self.bp.ChangeDutyCycle(0)
		GPIO.output(self.standbyPin,value)

	def __del__(self):
		GPIO.cleanup()


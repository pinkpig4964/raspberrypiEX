#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
// ./hellogpio [pinNumber]
#define delaytime 500 //500ms
#define loopCount 10
int main(int argc, char** argv)
{
    int gpioNo;
    int i;
    //Pin Number error
	//step1 : wiringPi init
    wiringPiSetup();
    if(argc < 2)	
    {
	printf("usage : %s GpioNumber\n",argv[0]);
	return -1;
    }
    gpioNo = atoi(argv[1]);

    //step2 Pin dorection setup
    pinMode(gpioNo, OUTPUT);
    for(i = 0; i<loopCount;i++){
    //setp 3: Pin Write
	digitalWrite(gpioNo , HIGH);
	delay(delaytime);

	digitalWrite(gpioNo , LOW);
	delay(delaytime);
    }

    return 0;
}

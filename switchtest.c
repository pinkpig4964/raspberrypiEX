#include <stdio.h>
#include <wiringPi.h>
#include <unistd.h>


#define SW1 0 //GPIO 7

int main(void)
{
    int swflag=0;
    //INIT wiringPi
    wiringPiSetup();
    int count=0;
    //pinAssign
    pinMode(SW1, INPUT);

    while(1)
    {
	//swflag=0이고 스위치가 눌렸을 때
	if(digitalRead(SW1)&&!swflag)
	{
	    count++;
	
	    printf("switch is on, %d\n",count);
	    swflag=1;
	}
	else if(!digitalRead(SW1)&& swflag)
	{
	    swflag=0;
	}
	usleep(10000);
    }
}

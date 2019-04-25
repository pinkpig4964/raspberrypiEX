#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <wiringPi.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "raspi.h"

#define	LED		1	// GPIO18
#define trig	4	// GPIO23
#define echo	5	// GPIO24

#define DEBUG
#define MAX_CLNT 256

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;

struct Data buf;
struct Data data;

//LED자원을 관리하기 위해 mutex변수를 선언한다.
pthread_mutex_t led_lock;
pthread_mutex_t hc04_lock;

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

//=====================================================
// LED Function
//=====================================================
void ledWrite(struct Data *data, int value)
{
	data->led_Value = value;
}

void ledControl(int value)
{
	// LED를 위한 MUTEX구현
	if (pthread_mutex_trylock(&led_lock) != EBUSY)
	{
		digitalWrite(LED, value);
		pthread_mutex_unlock(&led_lock);
	}
	return;
}

void* ledFunction(void *arg)
{
	pinMode(LED, OUTPUT);

	while (!data.endFlag)
	{
		ledControl(data.led_Value);
		//printf("led=%d\n", data.led_Value);
		usleep(200000);
	}

}

//=====================================================
// HC_SR04 Function
//=====================================================
float hc04Control(void)
{
	int startTime, endTime;
	float distance;

	// LED를 위한 MUTEX구현
	if (pthread_mutex_trylock(&hc04_lock) != EBUSY)
	{
		// Trig신호 생성 (10us)
		digitalWrite(trig, HIGH);
		usleep(10);					//wiringPi : delayMicroseconds(10); 
		digitalWrite(trig, LOW);

		// Echo back이 '1'이면 시작시간 기록 
		while (digitalRead(echo) == 0);
		//wiringPi : wiringPiSetup이후의 시간을 마이크로초로 측정하는 함수
		startTime = micros();

		// Echo back이 '1'이면 대기 
		while (digitalRead(echo) == 1);
		// Echo back이 '0'이면 종료시간 기록 
		endTime = micros();

		distance = (endTime - startTime) / 58;
		pthread_mutex_unlock(&hc04_lock);
		usleep(10000);
		//printf("distance:%f\n", distance);
	}
	return distance;
}

void* hc04Function(void *arg)
{
	float ret;
	pinMode(trig, OUTPUT);
	pinMode(echo, INPUT);
	digitalWrite(trig, LOW);
	while (!data.endFlag)
	{
		ret = hc04Control();
		data.hc04_dist = ret;

	}
	return NULL;
}

void* userThread(void *arg)
{
	int clnt_sock = *((int*)arg);
	int str_len = 0, i;
	int read_cnt;
	int wrDone = 0;
	pthread_t t_id[2];
	FILE *fp;
	char filebuf[BUF_SIZE];
	//led와 초음파센서를 위한 쓰레드 생성
	//pthread_create(&t_id[0], NULL, ledFunction, 0);
	//pthread_create(&t_id[1], NULL, hc04Function, 0);


	while ((str_len = read(clnt_sock, &buf, sizeof(buf))) != 0) // 클라이언트거 읽어서 buf구조체에 저장
	{
		switch (buf.cmd) //구조체의 변수 cmd에 해야될 명령이 저장됨.
		{
			//LED
		case WR_LED: data.led_Value = buf.led_Value;
			printf("data.led_Value=%d\n", data.led_Value);
			break;
			//초음파센서
		case RD_HC04:
			data.cmd = WR_DIST; //거리 적게함
			write(clnt_sock, &data, sizeof(data)); //적힌 거리를 클라이언트 소켓에 data란 구조체로 보냄
			printf("data.dist:%f\n", data.hc04_dist);
			break;
			//이미지
		case RD_IMG:
			data.cmd = WR_IMG; //위와 비슷
			write(clnt_sock, &data, sizeof(data));
			fp = fopen(FILENAME, "rb"); // 이진 갱신모드의 읽기
			while (1)
			{
				read_cnt = fread((void*)filebuf, 1, BUF_SIZE, fp);
				// 남아있는 파일의 사이즈가 BUF_SIZE보다 작을 경우
				if (read_cnt < BUF_SIZE)
				{
					write(clnt_sock, filebuf, read_cnt);
					break;
				}
				// 남아있는 파일의 사이즈가 BUF_SIZE보다 큰 경우
				write(clnt_sock, filebuf, BUF_SIZE);
			}
			fclose(fp);
			wrDone = 0;
			printf("File Write is done\n");
			break;
		default:
			break;
		}
	}
	data.endFlag = 1;
	//쓰레드 분리
	//pthread_detach(t_id[0]);
	//pthread_detach(t_id[1]);
	printf("userThread is end\n");
	pthread_mutex_lock(&mutx);//critical section 시작
	for (i = 0; i < clnt_cnt; i++)   // remove disconnected client
	{
		if (clnt_sock == clnt_socks[i])
		{
			while (i++ < clnt_cnt - 1)
				clnt_socks[i] = clnt_socks[i + 1];
			break;
		}
	}
	clnt_cnt--;
	pthread_mutex_unlock(&mutx);//critical section 종료
	close(clnt_sock);


}

static void sigHandler(int signum)
{
	printf("sigHanlder\n");
	exit(0);
}

int main(int argc, char **argv)
{
	int err;
	int serv_sock, clnt_sock;
	pthread_t thread_LED;
	pthread_t t_id[3];
	int str_len;
	struct sockaddr_in serv_adr, clnt_adr; //서버와 클라이언트 소켓 정보를 저장할 구조체
	int clnt_adr_sz; // 클라이언트 소켓 정보를 저장할 구조체의 길이를 저장할 변수
	data.endFlag = 0;
	//Init
	wiringPiSetup();

	signal(SIGINT, sigHandler);

	// STEP 1. socket생성
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	//주소 체계로서 ipv4를 사용하는 인터넷망에 접속
	serv_adr.sin_family = AF_INET;
	//모든 아이피의 접근을 허용. 16진수로 변경해서 저장
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	//프로그램 실행시 입력된 포트번호를 10진수로 변경해서 저장
	serv_adr.sin_port = htons(atoi(argv[1]));

	// bind() error는 발생하지 않는다.
	//setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &option, optlen);

	//STEP 2. bind : IP주소와 포트에 bind하기
	//소켓에 주소 할당
	if (bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	
	//STEP 3. 서버 소켓의 연결 요청 대기 큐를 5개로 설정
	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");
	//led와 초음파센서를 위한 쓰레드 생성
	pthread_create(&t_id[0], NULL, ledFunction, 0);
	pthread_create(&t_id[1], NULL, hc04Function, 0);
	while (1)
	{

		clnt_adr_sz = sizeof(clnt_adr); ///클라이언트 구조체의 크기를 구한다.
		// //클라이언트의 소켓으로부터 데이터를 받아들인다.
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
		//임계영역 시작
		pthread_mutex_lock(&mutx);
		//클라이언스 소켓 배열에 넣기
		clnt_socks[clnt_cnt++] = clnt_sock;
		//임계영역 해제
		pthread_mutex_unlock(&mutx);

		// 사용자의 요청에 대응하기 위해 클라이언트당 1개의 스레드 생성
		pthread_create(&t_id[2], NULL, userThread, (void*)&clnt_sock);
		pthread_detach(t_id[2]);
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr)); //inet_ntoa : 네트워크 바이트 순서의 32비트 값을 Dotted-Decimal Notation으로 변환
	}
	//led와 초음파센서 위한 쓰레드 해제
	pthread_detach(t_id[0]);
	pthread_detach(t_id[1]);

	close(serv_sock);
	return 0;

}

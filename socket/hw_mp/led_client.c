#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "raspi.h"
/*
<sys/socket.h>
 socket (), socketpair (), bind (), getsockname (), connect (), getpeername ()
 send (), recv (), sendto (), recvfrom (), sendmsg (), rcvmsg ()
 getsockopt (), setsockopt (), listen (), accept (), shutdown ()
 enum {SHUT_RD, SHUT_WD, SHUT_RDWD}
*/
/*
<arpa/inet.h>
 inet_addr (), inet_ntoa (), inet_aton ()
*/
void error_handling(char *message);
void read_routine(int sock, char *buf);
void write_routine(int sock, char *buf);

struct Data data;

int main(int argc, char *argv[])
{
	int sock;
	pid_t pid;
	char buf[BUF_SIZE];
	struct sockaddr_in serv_adr;


	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	//소켓 생성
	sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	//주소 체계로서 ipv4를 사용하는 인터넷망에 접속
	serv_adr.sin_family = AF_INET;
	//프로그램 실행시 입력된 아이피의 접근을 허용
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	//프로그램 실행시 입력된 포트번호를 10진수로 변경해서 저장
	serv_adr.sin_port = htons(atoi(argv[2]));
	// 소켓을 이용해 서버의 정보를 지닌 구조체를 가지고 접속 요청을 한다.
	if (connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("connect() error!");

	pid = fork();
	//자식 프로세스
	if (pid == 0)
		write_routine(sock, buf);
	//부모 프로세스
	else
		read_routine(sock, buf);

	close(sock);
	return 0;
}

void read_routine(int sock, char *buf)
{
	int read_cnt;
	char filebuf[BUF_SIZE];
	FILE *fp;

	while (1)
	{
		int str_len = read(sock, &data, sizeof(data));
		if (str_len == 0)
			return;
		switch (data.cmd)
		{
			// HC04의 거리데이터를 수신하면 값을 출력한다.
		case WR_DIST: printf("data.hc04_dist=%f\n", data.hc04_dist);
			break;
		case WR_IMG:
			//result.jpg에 쓴다.
			fp = fopen("result.jpg", "wb");
			while ((read_cnt = read(sock, filebuf, BUF_SIZE)) != 0)
			{
				fwrite((void*)filebuf, 1, read_cnt, fp);
				if (read_cnt < BUF_SIZE)
					break;
			}
			fclose(fp);
			printf("File Write is done\n");
			break;
		}
	}
}
void write_routine(int sock, char *buf)
{
	data.led_Value = 0;

	while (1)
	{
		// LED를 깜빡이기 위한 코드
		if (data.led_Value == 1)
			data.led_Value = 0;
		else
			data.led_Value = 1;

		// 서버에 LED값 변경을 위한 요청 데이터 보내기
		data.cmd = WR_LED;
		printf("data.led_Value=%d\n", data.led_Value);
		write(sock, &data, sizeof(data));

		// 서버에 HC04 거리 요청 데이터 보내기
		data.cmd = RD_HC04;
		write(sock, &data, sizeof(data));
		// 서버에 이미지 요청 데이터 보내기
		data.cmd = RD_IMG;
		write(sock, &data, sizeof(data));


		sleep(3);
	}

}
void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
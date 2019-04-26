#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>


#define DEBUG
#define VIDEOTIME	60000	//unit:msec
#define p_width	320
#define p_height 240

const char *MMOUNT = "/proc/mounts";
pid_t pid;

struct f_size
{
	long blocks;	// ��ü �� ��
	long avail;		// ��밡�� �� ��
};

typedef struct
{
	FILE *fp;               // ���� ��Ʈ�� ������    
	char devname[80];       // ��ġ �̸�
	char mountdir[80];      // ����Ʈ ���丮 �̸�
	char fstype[12];        // ���� �ý��� Ÿ��
	long f_type;
	long f_blocks;	    // Total Block count
	long f_bavail;	    // unused block count(Normal User)
	struct f_size size;     // ���� �ý����� ��ũ��/����� 
} MOUNTP;

MOUNTP *dfopen()
{
	MOUNTP *MP;

	// /proc/mounts ������ ����.
	MP = (MOUNTP *)malloc(sizeof(MOUNTP));
	if (!(MP->fp = fopen(MMOUNT, "r")))
	{
		return NULL;
	}
	else
		return MP;
}


MOUNTP *dfget(MOUNTP *MP)
{
	char buf[256];
	char *bname;
	char null[16];
	struct statfs lstatfs;
	struct stat lstat;
	int is_root = 0;

	// /proc/mounts�� ���� ����Ʈ�� ��Ƽ���� ������ ���´�.
	//fgets�� /proc/mounts������ ������ 1���ξ� �о�´�.
	while (fgets(buf, 256, MP->fp))
	{
		//is_root = 0;
		sscanf(buf, "%s%s%s", MP->devname, MP->mountdir, MP->fstype);
		if (strcmp(MP->mountdir, "/") == 0) is_root = 1;
		if (stat(MP->devname, &lstat) == 0 || is_root)
		{
			if (strstr(buf, MP->mountdir) && S_ISBLK(lstat.st_mode) || is_root)
			{
				// ���Ͻý����� �� �Ҵ�� ũ��� ��뷮�� ���Ѵ�.        
				statfs(MP->mountdir, &lstatfs);
				MP->size.blocks = lstatfs.f_blocks * (lstatfs.f_bsize / 1024);
				MP->size.avail = lstatfs.f_bavail * (lstatfs.f_bsize / 1024);
				return MP;
			}
		}
	}
	rewind(MP->fp);
	return NULL;
}

int dfclose(MOUNTP *MP)
{
	fclose(MP->fp);
}

static void sig_handler(int signal)
{
    kill(pid,SIGKILL);
    exit(0);
}

int main(int argc, char **argv)
{
	time_t UTCtime;
	struct tm *tm;
	char dirNamebuf[BUFSIZ];
	char fileNamebuf[BUFSIZ];
	char tempbuf[25];
	char cmdbuf[BUFSIZ];
	int width;
	int height;
	int status;
	float remain;
	signal(SIGINT, (void*)sig_handler);

	MOUNTP *MP;

	//argv[1]==0, FHD
	if (!strcmp("0",argv[1]))
	{
		width = 1920;
		height = 1080;
	}
	//argv[1]==1, HD
	else if (!strcmp("1", argv[1]))
	{
		width = 1280;
		height = 720;
	}
	else
	{
		printf("Error: argv[1] is not valid\n");
		return -1;
	}

#ifdef DEBUG
	printf("width:%d, height:%d\n", width, height);
#endif

	while (1)
	{
		//Ŀ�ηκ��� �ð��� �о�´�.
		time(&UTCtime);

		//tm����ü : ���ص� �ð����� ����
		tm = localtime(&UTCtime);

		//���丮���� ����
		strftime(dirNamebuf, sizeof(dirNamebuf), "%Y%m%d%H", tm);

		//���ϸ� ����
		strftime(tempbuf, sizeof(tempbuf), "%Y%m%d_%H%M%S.h264", tm);
		sprintf(fileNamebuf, "%s/%s", dirNamebuf, tempbuf);

#ifdef DEBUG
		printf("dirNamebuf:%s\n", dirNamebuf);
		printf("fileNamebuf:%s\n", fileNamebuf);
#endif

		struct dirent *de = NULL;

		if ((MP = dfopen()) == NULL)
		{
			printf("Error:dfopen()\n");
			return -1;
		}

		dfget(MP);

		remain = ((((float)MP->size.avail*100)/(float)MP->size.blocks));
#ifdef DEBUG
		printf("size.blocks:%ld\n", MP->size.blocks);
		printf("size.avail:%ld\n", MP->size.avail);
		printf("remain(%):%f\n", remain);
#endif
		//�����ִ� ��ũ ������ 5%�̸��̶��
		//���� ������ ���丮�� �����Ѵ�.
		if (remain < 5)
		{
			//TODO : ���� ������ ���丮�� ã�Ƽ� ���� �߰�
		}
		dfclose(MP);

		//���� �ð����� ���丮 ����
		if (mkdir(dirNamebuf, 0666) == -1)
		{
			if (errno != EEXIST)
				return -1;
		}
		chmod(dirNamebuf, 0777);

		sprintf(cmdbuf, "raspivid -p 0,0,%d,%d -w %d -h %d -t %d -o %s", \
			p_width,p_height, width, height, VIDEOTIME, fileNamebuf);
#ifdef DEBUG
		printf("cmdbuf:%s\n", cmdbuf);
#endif
		pid = fork();
		
		//�θ� ���μ���
		if (pid > 0)
		{
			// none
		}
		else if (pid == 0)
		{
			system(cmdbuf);
			exit(1);
		}

		wait(&status);
	}



	return 0;
}

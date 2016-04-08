#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <paths.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <syslog.h>
#include <signal.h>
#include <dirent.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <string.h>
#include <net/route.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#include "common.h"

#define MD5_OUT_BUFSIZE 36

static struct timeval tick_val= {0, 0};
int get_net_info(char *ifname, struct net_info *info)
{
	int sk;
	struct ifreq ifr;
	unsigned char buffer[6];
	int ret;
	struct link_data {
		unsigned int cmd;
		unsigned int value;
	}link;
	
	sk = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (sk < 0)
		return sk;
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

	// 检查连接
	link.cmd = 0x0000000A;
	link.value = 0;
	ifr.ifr_data = (caddr_t)&link;
	if (ioctl(sk, SIOCETHTOOL, &ifr) == 0)
	{
		if (link.value)
		{
			//info->link = 1;
			DBG(" %s link....\n", ifname);
		}
		else
		{

		}
	}
	// 获取IP
	if(ioctl(sk, SIOCGIFADDR, &ifr) == -1)
	{
		fprintf(stderr,"err\n");
		ret = -1;
		goto err;
	}
	memcpy(buffer, ifr.ifr_addr.sa_data, sizeof(buffer));
	memcpy(info->ip, &buffer[2], 4);

	// 获取netmask
	if(ioctl(sk, SIOCGIFNETMASK, &ifr) == -1)
	{
		fprintf(stderr,"err\n");
		ret = -1;
		goto err;
	}
	memcpy(buffer, ifr.ifr_netmask.sa_data, sizeof(buffer));
	memcpy(info->mask, &buffer[2], 4);

err:
	close(sk);
	return ret;
}

int get_mac(char *devname, char *mac)
{
	struct ifreq ifr;
	unsigned char tmbuf[IFHWADDRLEN];
	int ret = 0;
	int s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	
	memset(mac,0,6);
	strncpy(ifr.ifr_name, devname, IFNAMSIZ-1);
	if(ioctl(s, SIOCGIFHWADDR, &ifr) == -1){
		ret = -1;
	}
	else
	{
		memcpy(tmbuf, ifr.ifr_hwaddr.sa_data, sizeof(tmbuf));
		memcpy(mac, &tmbuf[0], 6);
	}

	close(s);

	return ret;
}

int get_netmask(char *devname, char *netmask)
{
	struct ifreq ifr;
	unsigned char tmbuf[6];
	int ret = 0;
	int s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	
	memset(netmask,0,4);
	strncpy(ifr.ifr_name, devname, IFNAMSIZ-1);
	if(ioctl(s, SIOCGIFNETMASK, &ifr)==-1){
		ret = -1;
	}
	else
	{
		memcpy(tmbuf, ifr.ifr_netmask.sa_data, sizeof(tmbuf));
		memcpy(netmask, &tmbuf[2], 4);
	}

	close(s);

	return ret;
}

int get_ip(char *devname, char *ip)
{
	struct ifreq ifr;
	unsigned char tmbuf[6];
	int ret = 0;
	int s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	
	memset(ip,0,4);
	strncpy(ifr.ifr_name, devname, IFNAMSIZ-1);
	if(ioctl(s, SIOCGIFADDR, &ifr)==-1){
		ret = -1;
	}
	else
	{
		memcpy(tmbuf, ifr.ifr_addr.sa_data, sizeof(tmbuf));
		memcpy(ip, &tmbuf[2], 4);
	}

	close(s);
	
	return ret;
}

int get_gateway(char *gw)
{
	char msg[64];
	char *via;
	FILE *fpin;
	int a, b, c, d;
	memset(gw,0,4);
	if((fpin = popen("ip route show|grep default","r")) == NULL)
		return -1;
	while(fgets(msg, sizeof(msg), fpin) != NULL)
	{
		if((via = strstr(msg,"via")) != NULL)
		{
		        a = gw[0];
		        b = gw[1];
		        c = gw[2];
		        d = gw[3];
			sscanf(via+4,"%d.%d.%d.%d ",&a, &b,&c,&d);
			pclose(fpin);
			return 0;
		}
	}
	pclose(fpin);
	return -1;
}

int getmask(char *nmask)
{

	int loopmask = 0;
	int ip[4] = {
		0, 0, 0, 0
	};

	sscanf(nmask, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);

	int n = 8;

	for (--n; n >= 0; --n)	// test all 4 bytes in one pass
	{
		if (ip[0] & 1 << n)
			loopmask++;
		if (ip[1] & 1 << n)
			loopmask++;
		if (ip[2] & 1 << n)
			loopmask++;
		if (ip[3] & 1 << n)
			loopmask++;
	}
	return loopmask;
}

char *get_complete_ip(char *from, char *to)
{
	static char ipaddr[20];

	int i[4];

	if (!from || !to)
		return "0.0.0.0";

	if (sscanf(from, "%d.%d.%d.%d", &i[0], &i[1], &i[2], &i[3]) != 4)
		return "0.0.0.0";

	snprintf(ipaddr, sizeof(ipaddr), "%d.%d.%d.%s", i[0], i[1], i[2], to);

	return ipaddr;
}

static char *get_ifname(char *name, int nsize, char *buf)
{
	char *end;

	// 跳过空格
	while(isspace(*buf))
		buf++;

	end = strstr(buf, ": ");
	if (end == NULL || ((end - buf) + 1) > nsize)
		return NULL;

	memcpy(name, buf, (end - buf));
	name[end - buf] = '\0';

	return end;
}

int sys_get_eth_ifname(char *ifname, int nsize)
{
	FILE *fp;
	char buf[256];
	int ret = 0;
	
	fp = fopen("/proc/net/dev", "r");
	if (fp == NULL)
		return -1;

	while (fgets(buf, sizeof(buf), fp))
	{
		if (buf[0] == '\0' || buf[1] == '\0')
			continue;

		if (get_ifname(ifname, nsize, buf) != NULL)
		{
			if (!strncmp(ifname, "eth", 3))
			{
				ret = 1;
				break;
			}
		}
	}
	fclose(fp);
	return ret;
}

int sys_get_wlan_ifname(char *ifname, int nsize)
{
	FILE *fp;
	char buf[256];
	int ret = 0;
	
	fp = fopen("/proc/net/wireless", "r");
	if (fp == NULL)
		return -1;

	while (fgets(buf, sizeof(buf), fp))
	{
		if (buf[0] == '\0' || buf[1] == '\0')
			continue;
		if (get_ifname(ifname, nsize, buf) != NULL)
		{
			if (!strncmp(ifname, "wlan", 4))
			{
				ret = 1;
				break;
			}
		}
	}
	fclose(fp);
	return ret;
}

int pipe_exec(char *shell)
{
	FILE *fp;
	char buffer[64];
	int ret = -1;

	fp = popen(shell, "r");
	if (fp != NULL)
	{
		memset(buffer, 0, sizeof(buffer));
		if (fread(buffer, 1, sizeof(buffer)-1, fp) > 0)
		{
			ret = 1;
		}
		else
		{
			ret = 0;
		}

		pclose(fp);
	}
	
	return ret;
}

int get_eth_link(char *name)
{
	char cmd[32];

	memset(cmd, 0x0, sizeof(cmd));
	sprintf(cmd, "ifconfig %s | grep RUNNING", name);

	return pipe_exec(cmd);
}

int LSMOD(char *module)
{
	char cmd[32];

	memset(cmd, 0x0, sizeof(cmd));
	sprintf(cmd, "lsmod | grep %s", module);

	return pipe_exec(cmd);
}

int file_to_buf(char *path, char *buf, int len)
{
	FILE *fp;

	memset(buf, 0, len);

	if ((fp = fopen(path, "r"))) {
		fclose(fp);
		return 1;
	}

	return 0;
}

int get_ppp_pid(char *file)
{
	char buf[80];
	int pid = -1;

	if (file_to_buf(file, buf, sizeof(buf))) {
		char tmp[80], tmp1[80];

		snprintf(tmp, sizeof(tmp), "/var/run/%s.pid", buf);
		file_to_buf(tmp, tmp1, sizeof(tmp1));
		pid = atoi(tmp1);
	}
	return pid;
}

char *find_name_by_proc(char *out, int pid)
{
	FILE *fp;
	char line[254];
	char filename[80];
	static char name[80];

	if(!out)
	    return NULL;

	snprintf(filename, sizeof(filename), "/proc/%d/status", pid);

	if ((fp = fopen(filename, "r"))) {
		if(fgets(line, sizeof(line), fp)){
		    sscanf(out, "%*s %s", line);
                }else{
                    out = NULL;
                }
		fclose(fp);
		return out;
	}
	return NULL;
}

int check_wan_link(int num)
{
	return 1;
}


static int system2(char *command)
{
        return system(command);
}

int sysprintf(const char *fmt, ...)
{
        char varbuf[256];
        va_list args;

        va_start(args, (char *)fmt);
        vsnprintf(varbuf, sizeof(varbuf), fmt, args);
        va_end(args);
        return system2(varbuf);
}

int _evalpid(char *const argv[], char *path, int timeout, int *ppid)
{
	pid_t pid;
	int status;
	int fd;
	int flags;
	int sig;

	switch (pid = fork()) {
	case -1:		/* error */
		perror("fork");
		return errno;
	case 0:		/* child */
		/*
		 * Reset signal handlers set for parent process 
		 */
		for (sig = 0; sig < (_NSIG - 1); sig++)
			signal(sig, SIG_DFL);

		/*  Clean up  */
		ioctl(0, TIOCNOTTY, 0);
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		setsid();

		/*
		 * We want to check the board if exist UART? , add by honor
		 * 2003-12-04 
		 */
		if ((fd = open("/dev/console", O_RDWR)) < 0) {
			(void)open("/dev/null", O_RDONLY);
			(void)open("/dev/null", O_WRONLY);
			(void)open("/dev/null", O_WRONLY);
		} else {
			close(fd);
			(void)open("/dev/console", O_RDONLY);
			(void)open("/dev/console", O_WRONLY);
			(void)open("/dev/console", O_WRONLY);
		}

		/*
		 * Redirect stdout to <path> 
		 */
		if (path) {
			flags = O_WRONLY | O_CREAT;
			if (!strncmp(path, ">>", 2)) {
				/*
				 * append to <path> 
				 */
				flags |= O_APPEND;
				path += 2;
			} else if (!strncmp(path, ">", 1)) {
				/*
				 * overwrite <path> 
				 */
				flags |= O_TRUNC;
				path += 1;
			}
			if ((fd = open(path, flags, 0644)) < 0)
				perror(path);
			else {
				dup2(fd, STDOUT_FILENO);
				close(fd);
			}
		}

		setenv("PATH", "/sbin:/bin:/usr/sbin:/usr/bin", 1);
		alarm(timeout);
		execvp(argv[0], argv);
		perror(argv[0]);
		exit(errno);
	default:		/* parent */
		if (ppid) {
			*ppid = pid;
			return 0;
		} else {
			waitpid(pid, &status, 0);
			if (WIFEXITED(status))
				return WEXITSTATUS(status);
			else
				return status;
		}
	}
}

int _eval(char *const argv[],int timeout, int *ppid)
{
       return _evalpid(argv, ">/dev/console", timeout, ppid);
}

int f_exists(const char *path)  // note: anything but a directory
{
        struct stat st;

        return (stat(path, &st) == 0) && (!S_ISDIR(st.st_mode));
}


int f_read(const char *path, void *buffer, int max)
{
        int f;
        int n;

        if ((f = open(path, O_RDONLY)) < 0)
                return -1;
        n = read(f, buffer, max);
        close(f);
        return n;
}

int f_read_string(const char *path, char *buffer, int max)
{
        if (max <= 0)
                return -1;
        int n = f_read(path, buffer, max - 1);

        buffer[(n > 0) ? n : 0] = 0;
        return n;
}


char *psname(int pid, char *buffer, int maxlen)
{
        char buf[512];
        char path[64];
        char *p;

        if (maxlen <= 0)
                return NULL;
        *buffer = 0;
        sprintf(path, "/proc/%d/stat", pid);
        if ((f_read_string(path, buf, sizeof(buf)) > 4)
            && ((p = strrchr(buf, ')')) != NULL)) {
                *p = 0;
                if (((p = strchr(buf, '(')) != NULL) && (atoi(buf) == pid)) {
                        strncpy(buffer, p + 1, maxlen);
                }
        }
        return buffer;
}


static int _pidof(const char *name, pid_t ** pids)
{
        const char *p;
        char *e;
        DIR *dir;
        struct dirent *de;
        pid_t i;
        int count;
        char buf[256];

        count = 0;
        *pids = NULL;
        if ((p = strchr(name, '/')) != NULL)
                name = p + 1;
        if ((dir = opendir("/proc")) != NULL) {
                while ((de = readdir(dir)) != NULL) {
                        i = strtol(de->d_name, &e, 10);
                        if (*e != 0)
                                continue;
                        if (strcmp(name, psname(i, buf, sizeof(buf))) == 0) {
                                if ((*pids =
                                     realloc(*pids,
                                             sizeof(pid_t) * (count + 1))) ==
                                    NULL) {
                                        return -1;
                                }
                                (*pids)[count++] = i;
                        }
                }
        }
        closedir(dir);
        return count;
}

int pidof(const char *name)
{
        pid_t *pids;
        pid_t p;

        if (_pidof(name, &pids) > 0) {
                p = *pids;
                free(pids);
                return p;
        }
        return -1;
}

int killall(const char *name, int sig)
{
        pid_t *pids;
        int i;
        int r;

        if ((i = _pidof(name, &pids)) > 0) {
                r = 0;
                do {
                        r |= kill(pids[--i], sig);
                }
                while (i > 0);
                free(pids);
                return r;
        }
        return -2;
}

int kill_pid(int pid)
{
	int deadcnt = 20;
	struct stat s;
	char pfile[32];
	
	if (pid > 0)
	{
		kill(pid, SIGTERM);

		sprintf(pfile, "/proc/%d/stat", pid);
		while (deadcnt--)
		{
			usleep(100*1000);
			if ((stat(pfile, &s) > -1) && (s.st_mode & S_IFREG))
			{
				kill(pid, SIGKILL);
			}
			else
				break;
		}
		return 1;
	}
	
	return 0;
}

int stop_process(char *name)
{
	int deadcounter = 20;

        if (pidof(name) > 0) {
                killall(name, SIGTERM);
                while (pidof(name) > 0 && deadcounter--) {
                        usleep(100*1000);
                }
                if (pidof(name) > 0) {
                        killall(name, SIGKILL);
                }
                return 1;
        }
        return 0;
}

void get_sys_date(struct tm *tm)
{
	time_t timep;
	
	time(&timep);


	tm = localtime(&timep);

}

int get_tick_1hz(void)
{
	struct timeval tv;
	int tick;

	gettimeofday(&tv, NULL);
	tick = (tv.tv_sec - tick_val.tv_sec)*1000 + (tv.tv_usec - tick_val.tv_usec)/1000;

	return tick/1000;
}

void clktick_init(void)
{
	gettimeofday(&tick_val, NULL);
}

int get_tick_1khz(void)
{
	int tick;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	tick = (tv.tv_sec - tick_val.tv_sec)*1000 + (tv.tv_usec - tick_val.tv_usec)/1000;

	return tick;
}

#define GPIO_SYS_PATH	"/sys/class/gpio"

void gpio_export(int gpio, char *dir, int value)
{
	// set gpio-pinmux
	sysprintf("echo \"%d\" >%s/export", gpio, GPIO_SYS_PATH);
	// set gpio-direction and gpio-level: "in" or "out", 1 or 0
	if (!strcmp(dir, "in"))
	{
		sysprintf("echo \"in\" >%s/gpio%d/direction", GPIO_SYS_PATH, gpio);
	}
	else
	{
		sysprintf("echo \"out\" >%s/gpio%d/direction", GPIO_SYS_PATH, gpio);
		sysprintf("echo \"%d\" >%s/gpio%d/value", value, GPIO_SYS_PATH, gpio);
	}
}

void gpio_unexport(int gpio)
{
	sysprintf("echo \"%d\" >%s/unexport", gpio, GPIO_SYS_PATH);
}

void set_gpio_high(int gpionum)
{
//	sysprintf("echo \"out\" >%s/gpio%d/direction", GPIO_SYS_PATH, gpionum);
	sysprintf("echo \"1\" >%s/gpio%d/value", GPIO_SYS_PATH, gpionum);
}

void set_gpio_low(int gpionum)
{
//	sysprintf("echo \"out\" >%s/gpio%d/direction", GPIO_SYS_PATH, gpionum);
	sysprintf("echo \"0\" >%s/gpio%d/value", GPIO_SYS_PATH, gpionum);
}

int get_gpio_level(int gpionum)
{
	return sysprintf("cat %s/gpio%d/value", GPIO_SYS_PATH, gpionum); 
}

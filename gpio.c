#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>
#include <signal.h>

#include "gpio.h"

/*
int cont = 1;
void  finish(int sig)
  {
  cont = 0;
  }

int main(int argc,char** argv)
{
    int value;
    int pnr = 17;
    int e_cnt = 0;
    struct sigaction sig_struct;

    sig_struct.sa_handler = finish;
    sigemptyset(&sig_struct.sa_mask);
    sig_struct.sa_flags = 0;
    sigaction(SIGINT,&sig_struct,NULL);

    gpio_init(pnr,MODE_READ);
    value = gpio_read(pnr);
    //the infamous blinking led example
	
    value = 1;
	while(1)
	{
		gpio_write(pnr,value);
		usleep(500000);
		value = (value + 1)%2;
	}
	
	gpio_edge_prepare(pnr,E_BOTH);

	int fd;
    char bfr[BUFFER_SIZE];

    memset(bfr,0,BUFFER_SIZE);
    sprintf(bfr,"/sys/class/gpio/gpio%d/value",pnr);
    fd = open(bfr,O_RDONLY | O_NONBLOCK);

	while(cont)
	{
		value = wait_for_edge_of(fd);
		if (value == 1)
		{
			printf("falling edge #%d detected\n",e_cnt);
			e_cnt++;
		}
		usleep(300000);
	}
    gpio_close(pnr);
}
*/

int gpio_init(int pin_nr,int mode)
{
    int fd;
    char bfr[BUFFER_SIZE];
    int cnt;
    fd = open("/sys/class/gpio/export",O_WRONLY);
    if (fd < 0)
    {
        fprintf(stderr,"unable to initialize GPIO %d, file access error\n",pin_nr);
        return(-1);
    }
    cnt = sprintf(bfr,"%d",pin_nr);
    if (write(fd,bfr,cnt)<0)
    {
        fprintf(stderr,"unable to initialize GPIO %d, write error\n",pin_nr);
        return(-1);
    }
    close(fd);
    memset(bfr,0,BUFFER_SIZE);
    sprintf(bfr,"/sys/class/gpio/gpio%d/direction",pin_nr);
    fd = -1;
    int waitcnt=0;
    struct stat statbfr;
    while (stat(bfr,&statbfr) == 0 && waitcnt < 10000)
    {
	usleep(100);
	waitcnt++;
    }
    fd = open(bfr,O_WRONLY);
    if (fd < 0)
    {
	fprintf(stderr,"unable to access direction for GPIO %d\n",pin_nr);
        return(-1);
    }
    // printf("had to wait %d ms for GPIO export\n",waitcnt/10);
    switch (mode)
    {
        case MODE_WRITE:
            sprintf(bfr,"out");
            if (write(fd,bfr,3)<0)
	    {
		fprintf(stderr,"unable to set write mode on GPIO %d\n",pin_nr);
	        return(-1);
	    }
            break;
        case MODE_READ:
            sprintf(bfr,"in");
            if(write(fd,bfr,2)<0)
	    {
		fprintf(stderr,"unable to set read mode on GPIO %d\n",pin_nr);
	        return(-1);
	    }
            break;
        close(fd);
    }
    return 0;
}

int gpio_close(int pin_nr)
{
    int fd;
    char bfr[BUFFER_SIZE];
    int cnt;
    fd = open("/sys/class/gpio/unexport",O_WRONLY);
    if (fd < 0)
    {
        fprintf(stderr,"unable to close GPIO %d, file access error\n",pin_nr);
        return(-1);
    }
    memset(bfr,0,BUFFER_SIZE);
    cnt = sprintf(bfr,"%d",pin_nr);
    if (write(fd,bfr,cnt)<0)
    {
        fprintf(stderr,"unable to close GPIO %d, write error\n",pin_nr);
        return(-1);
    }
    close(fd);
    return 0;
}

int gpio_read(int pin_nr)
{
    int fd;
    char bfr[BUFFER_SIZE];
    int cnt;
    memset(bfr,0,BUFFER_SIZE);
    sprintf(bfr,"/sys/class/gpio/gpio%d/value",pin_nr);
    fd = open(bfr,O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr,"unable to read from GPIO %d, file access error\n",pin_nr);
        return(-1);
    }
    if (read(fd,bfr,1)<0)
    {
        fprintf(stderr,"unable to read from GPIO %d, reading error\n",pin_nr);
        return(-1);
    }
    close(fd);
    return atoi(bfr);
}

int gpio_write(int pin_nr,int val)
{
    int fd;
    char bfr[BUFFER_SIZE];
    int cnt;
    memset(bfr,0,BUFFER_SIZE);
    sprintf(bfr,"/sys/class/gpio/gpio%d/value",pin_nr);
    fd = open(bfr,O_WRONLY);
    if (fd < 0)
    {
        fprintf(stderr,"unable to write to GPIO %d, file access error\n",pin_nr);
        return(-1);
    }
    memset(bfr,0,BUFFER_SIZE);
    sprintf(bfr,"%d",val);
    printf("writing %s to GPIO %d\n",bfr,pin_nr);
    if (write(fd,bfr,1)<0)
    {
        fprintf(stderr,"unable to write to GPIO %d, writing error\n",pin_nr);
        return(-1);
    }
    close(fd);
    return 0;
}

int gpio_edge_prepare(int pin_nr,int edgetype)
{
	int fd;
	char bfr[BUFFER_SIZE];
    memset(bfr,0,BUFFER_SIZE);
	sprintf(bfr,"/sys/class/gpio/gpio%d/edge",pin_nr);
	fd = open(bfr, O_WRONLY | O_NONBLOCK);
	if (fd < 0)
	{
		fprintf(stderr,"unable to access edge definition for GPIO %d, file access error\n",pin_nr);
		return(-1);
	}
    memset(bfr,0,BUFFER_SIZE);
	switch(edgetype)
	{
		case E_RISING:
			sprintf(bfr,"rising");
			break;
		case E_FALLING:
			sprintf(bfr,"falling");
			break;
		case E_BOTH:
			sprintf(bfr,"both");
			break;
		case E_NONE:
			sprintf(bfr,"none");
			break;
		default:
			return(-2);
	}
	if (write(fd,bfr,strlen(bfr)+1)<0)
	{
		fprintf(stderr,"unable to set edge trigger for GPIO %d\n",pin_nr);
		return(-1);
	}
	close(fd);
	return(0);
}

int wait_for_edge(int pin_nr)
{
	int fd;
    char bfr[BUFFER_SIZE];
    int cnt;
    int rc;
	struct pollfd pollconf[1];
    memset(bfr,0,BUFFER_SIZE);
    sprintf(bfr,"/sys/class/gpio/gpio%d/value",pin_nr);
    fd = open(bfr,O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        fprintf(stderr,"unable to read from GPIO %d, file access error\n",pin_nr);
        return(-1);
    }
	memset(bfr,0,BUFFER_SIZE);
	pollconf[0].fd = fd;
	pollconf[0].events = POLLPRI;
	lseek(fd,0,SEEK_SET);
	read(fd,bfr,BUFFER_SIZE-1);
	rc = poll(pollconf,1,-1);
	if (rc < 0)
	{
		fprintf(stderr,"Poll() call for GPIO %d returned an error\n",pin_nr);
		return(-1);
	}
	else if (rc == 0)
	{
		close(fd);
		return(-2);
	}
	else if (pollconf[0].revents & POLLPRI)
	{
		rc = read(fd,bfr,1);
		if (rc < 0)
		{
			fprintf(stderr,"Error upon reading from Poll() for GPIO %d\n",pin_nr);
			close(fd);
			return(-1);
		}
		close(fd);
		return(atoi(bfr));
	}
	else
	{
		return(-3);
	}
}

int wait_for_edge_of(int fd)
{
    char bfr[BUFFER_SIZE];
    int cnt;
    int rc;
	struct pollfd pollconf[1];

	memset(bfr,0,BUFFER_SIZE);
	pollconf[0].fd = fd;
	pollconf[0].events = POLLPRI;
	lseek(fd,0,SEEK_SET);
	read(fd,bfr,BUFFER_SIZE-1);
	rc = poll(pollconf,1,-1);
	if (rc < 0)
	{
		fprintf(stderr,"Poll() call for GPIO returned an error\n");
		return(-1);
	}
	else if (rc == 0)
	{
		return(-2);
	}
	else if (pollconf[0].revents & POLLPRI)
	{
		rc = read(fd,bfr,1);
		if (rc < 0)
		{
			fprintf(stderr,"Error upon reading from Poll() for GPIO\n");
			return(-1);
		}
		return(atoi(bfr));
	}
	else
	{
		return(-3);
	}
}



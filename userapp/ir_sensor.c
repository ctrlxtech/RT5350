/*
the ioport is only useful for
port
7  i2s clk
9  i2s sws
10  i2s ssd
11 RTSN
12	RXD
14 TXD
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>

#define DEVICE_IR "/dev/ir"

#define IR_SENSOR_SEND_NEC      0x90
#define IR_SENSOR_REC_DATA      0x91
#define IR_SENSOR_SEND_ASWAV    0x92
#define RALINK_GPIO_DATA_LEN     24
#define record_length 10000
//#define comp_length record_length/8
/*
*this is a function for distinguish NEC code
*if the code is not NEC return -1
*else return address of the client
*input char* req (The record from I/O)
*output char * cmd
*/
int isNEC (char * req,char * cmd)
{
	int address;
	char data;
	int count;
	int i;
	int j;
	i=0;
	j=0;
	address=0;
	data=0;
	count=0;
	while(req[i]==1)
	{
		count++;
		i++;
	}
	if(count>75)
		count=0;
	else
		return -1;

	while(req[i]==0)
	{
		count++;
		i++;
	}
	if(count>30)
	{
		count=0;
	}
	else
		return -1;
	// begin to 
	for(j=0;j<16;j++)
	{
		while(req[i]==1)
		{
			i++;
			count++;
		}
		if(count>3)
		{
			count==0;
		}
		else
		{
			return -1;
		}
		while(req[i]==0)
		{
			i++;
			count++;
		}
		if(count<4)
		{
			return -1;
		}
		else if(count>10)
		{
			count=0;
			address=address<<1+1;
		}
		else
		{
			count=0;
			address=address<<1;
		}
	}
	for(j=0;j<8;j++)
	{
		while(req[i]==1)
		{
			i++;
			count++;
		}
		if(count>3)
		{
			count==0;
		}
		else
		{
			return -1;
		}
		while(req[i]==0)
		{
			i++;
			count++;
		}
		if(count<4)
		{
			return -1;
		}
		else if(count>10)
		{
			count=0;
			data=data<<1+1;
		}
		else
		{
			count=0;
			data=data<<1;
		}
	}
	*cmd=data;
	return address;
}

int main(int argc, char * argv[])
{
	printf("Break Point -3\n");
	int fd;
	int flag;
	unsigned int req;
	unsigned char buf[record_length];
	unsigned char write_buf[record_length];
	int iter;
	int client_addr;
	char data;
	
	printf("Break Point -2\n");
	if((fd=open("/dev/ir",O_RDWR))<0)
	{
		printf("Break Point -1\n");
		perror("can not open device\n");
	}
	char cmd;
	printf("Break Point 0\n");
	cmd= *argv[1];
	printf("Break Point 0.5\n");

	printf("cmd=%c\n",cmd);
				
   //send function
	/*if(cmd=='s')
	{
		printf("always send\n");
		
		while(1)
		{
			req= ((0xF0L)<<RALINK_GPIO_DATA_LEN)|IR_SENSOR_SEND_NEC ;
			flag=ioctl(fd,req,0x3333L);		
		}
	}
	else if(cmd=='w')
	{
		printf("send buf\n");
		int i;
		memset(buf,1,1000);
		memset(buf+1000,0,1000);
		memset(buf+2000,1,1000);
		memset(buf+3000,0,1000);
		memset(buf+4000,1,1000);
		while(1)
		{
			write(fd,buf,comp_length);
			req=IR_SENSOR_SEND_ASWAV ;
			ioctl(fd,req,0x00L);
		}
		
	}
	//recv function
	else if(cmd=='r')
	{
		printf("rec\n");
	   while(1)
		{
			printf("a new round");
			req = IR_SENSOR_REC_DATA;
			ioctl(fd,req,0x00L);

			unsigned int num=read(fd,buf,5000);

			sleep(5);
		}	
	}*/
	printf("break point 1 \n");
	if(cmd=='l')
	{
			printf("learn\n");
			req = IR_SENSOR_REC_DATA;
			ioctl(fd,req,0x00L);
			printf("break point 1.1 \n");
			unsigned int num=read(fd,buf,record_length);
			printf("break point 1.2 \n");

			FILE * fp=fopen("learn.ir","w");
			printf("break point 1.3 \n");

			//for(iter=0;iter<record_length;iter++)
			//	fprintf(fp,"%u",buf[iter]);
			fwrite(buf,sizeof(char),record_length,fp);
			printf("break point 1.4 \n");
			fclose(fp);
			printf("finish learn \n");
	
	}
	else
	{
         printf("Break Point 2!\n");
			FILE * fp=fopen("control.ir","r");
			/*for(iter=0;iter<record_length;iter++)
			{
				fscanf(fp,"%u",&write_buf[iter]);
				printf("%u",write_buf[iter]);
				if(iter%100)
					printf("\n");			
			}*/
			
			fread(write_buf,sizeof(char),record_length,fp);
			//FILE * fp_tmp=fopen("learn_tmp.ir","w");
			//fwrite(buf,sizeof(char),record_length,fp_tmp);
			//fclose(fp_tmp);
			write(fd,write_buf,record_length);
			req=IR_SENSOR_SEND_ASWAV ;
			ioctl(fd,req,0x00L);

			fclose(fp);
	}
}

#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <sys/wait.h>
 
// #define SERVER "127.0.0.1"
#define BUFLEN 512  //Max length of buffer
#define PORT_0 4444   //The port on which to send data
#define PORT_1 7777   //The port on which to send data
#define PORT_2 8888   //The port on which to send data
#define PORT_MAX 3   
#define COMMAND_CALL_SIZE 1024
#define RETRY_MAX 10
#define RETRY_INTERVAL 1
#define IP_MAX 16
#define MAC_MAX 18
#define BROADCAST_INTERVAL 5
#define NUM_BCAST_MAX 2
#define EXIT_NEED_RECONNECT_WIFI_FR_BCAST 3


char *ip_find_bcast_addr(char *broadcast_ip_t);

void die(char *s)
{
    perror(s);
    exit(1);
}
 
int main(int argc, char **argv)
{
    struct sockaddr_in si_other;
    int client_socket, i, j, slen=sizeof(si_other);
    char buf[BUFLEN];
    char message[BUFLEN];
 
    
	
	
	//read device ip and compose message
	char command[COMMAND_CALL_SIZE];
	char result_buf[COMMAND_CALL_SIZE],IP[IP_MAX], MASK[IP_MAX], BROADCAST[IP_MAX], MAC[MAC_MAX];
	int rc, MAC_read, IP_read, timeout_count = 0; 
	char *MAC_t, *IP_t, *MASK_t, *BROADCAST_t;
    FILE *fp;
	
	MAC_t = NULL;
	IP_t = NULL;
	MASK_t = NULL;
	BROADCAST_t = NULL;
	
	snprintf(command, sizeof(command), "ifconfig ra0");
	
	// get MAC, IP, MASK address
	while((timeout_count<RETRY_MAX) && (!MAC_t || !IP_t || !MASK_t))
	{
		sleep(RETRY_INTERVAL);
		timeout_count += 1;
		rc = 0;
		
		fp = popen(command, "r");
		if(NULL == fp)
		{
			printf("fail to execute popen!");
			return 1;
		}
		while(fgets(result_buf, sizeof(result_buf), fp) != NULL)
		{
			if('\n' == result_buf[strlen(result_buf)-1])
			{
				result_buf[strlen(result_buf)-1] = '\0';
			}
			
			char *c_t;
			
			if(!MAC_t && (MAC_t=strstr(result_buf,"HWaddr ")))
			{
				printf("read MAC_t...\n");
				MAC_t = MAC_t+7*sizeof(char);
				c_t = MAC_t+17*sizeof(char);
				*c_t = '\0';
				strcpy(MAC,MAC_t);
				printf("device MAC_t: %s\n",MAC_t);
			}

			
			if(!IP_t && (IP_t=strstr(result_buf,"inet addr:")))
			{
				printf("read IP_t...\n");
				IP_t = IP_t+10*sizeof(char);
				c_t = strstr(result_buf,"  Bcast");
				*c_t = '\0';
				strcpy(IP,IP_t);
				printf("device IP_t: %s\n",IP_t);
			}
			
			if(!MASK_t && (MASK_t=strstr(result_buf,"Mask:")))
			{
				printf("read MASK_t...\n");
				MASK_t = MASK_t+5*sizeof(char);
				// c_t = strstr(result_buf,"0");
				// *c_t = '\0';
				strcpy(MASK,MASK_t);
				printf("subnet MASK: %s\n",MASK_t);
			}
			
			if(!BROADCAST_t && (BROADCAST_t=strstr(result_buf,"Bcast:")))
			{
				printf("read BROADCAST_t...\n");
				BROADCAST_t = BROADCAST_t+6*sizeof(char);
				c_t = strstr(result_buf,"  Mask");
				*c_t = '\0';
				strcpy(BROADCAST,BROADCAST_t);
				printf("BROADCAST IP: %s\n",BROADCAST_t);
			}
		}
		
		
		printf("device MAC: %s\n",MAC);
		printf("device IP: %s\n",IP);
		// printf("device MASK: %s\n",MASK);
		printf("broadcast IP: %s\n",BROADCAST);

		rc = pclose(fp);
		if(-1 == rc)
		{
			printf("fail to close file pointer");
			return 1;
		}
		else
		{
			printf("command [%s] closing status of child process [%d] returl value [%d]\r\n", command, rc, WEXITSTATUS(rc));
		}
		
		
	}
	
	if(timeout_count==RETRY_MAX)
	{
		printf("time out! force exit! \n");
		exit(EXIT_NEED_RECONNECT_WIFI_FR_BCAST);
	}
	
	
	sprintf(message,"hello:%s/%s",MAC,IP);
	printf("message to be sent: %s\n", message);
	
	char IP_t1[IP_MAX], IP_masked[IP_MAX+1], MASK_t1[IP_MAX];
	char *ip_t, *mask_t, *ip_masked_t;
	char *ip_t_f, *mask_t_f;
	unsigned int i_ip, i_mask;
	strcpy(MASK_t1,MASK);
	strcpy(IP_t1, IP);
	ip_masked_t = IP_masked;
	ip_t_f = IP_t1;
	mask_t_f = MASK_t1;
	i = 0;
	while(i<4)
	{
		if(i<3)
		{
			ip_t = strchr(ip_t_f,'.');
			*ip_t = '\0';
			mask_t = strchr(mask_t_f,'.');
			*mask_t = '\0';
		}
		i_ip = atoi(ip_t_f);
		i_mask = atoi(mask_t_f);
		printf("i_ip:%i\ni_mask:%i\n",i_ip,i_mask);
		char c[IP_MAX];
		// itoa((i_ip & i_mask),c,10);
		if(i<3)
		{
			sprintf(ip_masked_t,"%i.",(i_ip & i_mask));
		}
		else{
			sprintf(ip_masked_t,"%i",(i_ip & i_mask));
		}
		ip_masked_t = strchr(IP_masked,'\0');
		ip_t_f = ip_t + sizeof(char);
		mask_t_f = mask_t + sizeof(char);
		printf("%s\n",IP_masked);
		
		i++;
	}
	
	
	printf("IP_masked: %s\n",IP_masked);
	
	
	
	//start socket
	if ( (client_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		die("socket");
	}
	
	int bcastNum = 0;
	char broadcast_ip_t[IP_MAX];
    char bcast_addr[IP_MAX];
	int Broadcast = 1;
	while(bcastNum < NUM_BCAST_MAX)
	{
		bcastNum++;
		i = 0;
		strcpy(broadcast_ip_t,IP_masked);
		ip_find_bcast_addr(broadcast_ip_t);
		strcpy(bcast_addr, broadcast_ip_t);
		printf("broad cast address: %s\n",bcast_addr);
		
		memset((char *) &si_other, 0, sizeof(si_other));
		si_other.sin_family = AF_INET;
		if (inet_aton(bcast_addr , &si_other.sin_addr) == 0) 
		{
			fprintf(stderr, "inet_aton() failed\n");
			exit(1);
		}
		// enable udp broadcast
	    if( setsockopt(client_socket,SOL_SOCKET,SO_BROADCAST,&(Broadcast),sizeof(int)) == -1)
        {
            close(client_socket);
            printf( "   broadcast   setup   error/n ");
            return -1;
        }
		
		int j = 0;
		int ports[PORT_MAX];
		ports[0] = PORT_0;
		ports[1] = PORT_1;
		ports[2] = PORT_2;
			
		while(j<PORT_MAX)
		{
			si_other.sin_port = htons(ports[j]);
			//send the message
			printf("broadcasting to host:%s  port:%i!\n",bcast_addr,ports[j]);
			if (sendto(client_socket, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
			{
				die("sendto()");
			}
			j++;
		}	
		
		sleep(BROADCAST_INTERVAL);
	}
	
    return 0;
}


char *ip_find_bcast_addr(char *broadcast_ip_t)
{
	char broadcast_ip[IP_MAX], *sub_ip_t, *sub_ip_t_f;
	unsigned int sub_ip[4];
	strcpy(broadcast_ip,broadcast_ip_t);
	sub_ip_t = broadcast_ip;
	sub_ip_t_f = broadcast_ip;
	int i = 0;
	while(i<4)
	{
		if(i<3)
		{
			sub_ip_t = strchr(sub_ip_t_f,'.');
			*sub_ip_t = '\0';
		}
		sub_ip[i] = atoi(sub_ip_t_f);
		sub_ip_t_f = sub_ip_t + sizeof(char);
		
		i++;
	}
	sub_ip[3] = 255;
	bzero(broadcast_ip_t, IP_MAX);
	sprintf(broadcast_ip_t,"%i.%i.%i.%i",sub_ip[0],sub_ip[1],sub_ip[2],sub_ip[3]);
	// printf("self added broadcast_ip: %s\n",broadcast_ip_t);
	return broadcast_ip_t;
}

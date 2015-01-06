#include <netinet/in.h>    // for sockaddr_in
#include <sys/types.h>    // for socket
#include <sys/socket.h>    // for socket
#include <stdio.h>        // for printf
#include <stdlib.h>        // for exit
#include <string.h>        // for bzero

#include <errno.h>
#include <sys/wait.h>
/*
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
*/
#define HELLO_WORLD_SERVER_PORT 6666 
#define LENGTH_OF_LISTEN_QUEUE 20
#define BUFFER_SIZE 1024
#define REQUEST_MAX_SIZE 512
#define CONTROL_MAX_SIZE 16
#define PASSWORD_MAX_SIZE 256
#define SSID_MAX_SIZE 256
#define FILE_NAME_MAX_SIZE 256
#define COMMAND_CALL_SIZE 1024

// general mode definition
#define MODE_EMPTY 0
#define MODE_INI_WIFI 1
#define MODE_INI_CONNECTION 2
#define MODE_INI_CONTROL 3
#define MODE_INI_LEARNING 4

// control mode definition
#define CONTROL_MODE_EMPTY "0"
#define CONTROL_MODE_IR_SEND "1"

#define IR_CONTROL_FILE_NAME "control.ir"


// learning mode definition
#define IR_LEARNING_FILE_NAME "learn.ir"
#define IR_LEARNING_ERROR_EMPTY_FILE "0"




// socket
#define NUM_OF_PHONES_MAX 8
#define SOCKET_END_FLAG "xend"
#define SOCKET_RECV_SEND_TIMEOUT 5
#define SOCKET_ACCEPT_TIMEOUT 15

// exit code
#define EXIT_NEED_RECONNECT_WIFI_FR_SERVER 2

// other definition
#define TIMEOUT_TMP_MAX 3
#define TIMEOUT_TIME_IN_S 5
#define IP_TIMEOUT_TIME_IN_S 3
#define IP_TIMEOUT_TMP_MAX 20
#define TIMEOUT_CONN_TEST_TMP_MAX 1//10
#define RECONNECT_INTERVAL 10//1200 // 5min
#define SSID_MAX 1024
#define PSW_MAX 1024
#define IP_MAX 18



// int learning_mode(char *ip);
int cmdCall(char *cmd);// return 0 means success, 1 means failure
//int conn_WIFI(char *SSID, char *PSW);// return 0 means success, 1 means failure
int device_ap_reset();
inline void change_to_AP();
inline void change_to_STA();
int iniServerSocket();
int FLASH_read(char *result, char *c_t);// re 6turn 0 means success, 1 means failure
int LOCAL_WIFI_connection_test();


int main(int argc, char **argv)
{
	short mode,loop_count, retry;
	char command[COMMAND_CALL_SIZE],Control_Mode[CONTROL_MAX_SIZE], Local_WIFI_pwd[PASSWORD_MAX_SIZE], Local_WIFI_SSID[SSID_MAX_SIZE]; //operation mode:0-Empty; 1-iniWIFI; 2-iniConnection; 3-iniControl; 4-iniLearning
	struct sockaddr_in Phone_IP[NUM_OF_PHONES_MAX];
	int callState, conn_fail_count;
	struct timeval timeout;
	
	retry = 0;
	loop_count = 0;
	conn_fail_count = 0;
	mode = MODE_EMPTY;
	strcpy(Control_Mode,CONTROL_MODE_EMPTY); //control command: 0-Empty; 1-send IR signal
	
	int server_socket = iniServerSocket();
	
	timeout.tv_sec = SOCKET_ACCEPT_TIMEOUT;
	timeout.tv_usec = 0;
	if(setsockopt(server_socket,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval)))
	{
		printf("cannot set socket timeout!\n");
		exit(1);
	}
	
	char file_name[FILE_NAME_MAX_SIZE+1]; //specify the IR command file need to be sent
    while (1)
    {
        struct sockaddr_in client_addr;
		
        socklen_t length = sizeof(client_addr);
		
        int new_server_socket = accept(server_socket,(struct sockaddr*)&client_addr,&length);
		printf("new_server_socket: %i\n",new_server_socket);
		if(new_server_socket==-1)
		{
			char ssid[SSID_MAX], psw[PSW_MAX], op_mode[2];
			snprintf(command, sizeof(command), "OperationMode");
			FLASH_read(op_mode,command);
			printf("operation mode: %s\n", op_mode);
			
			printf("no incoming connection...\n");
			
			if(!strcmp(op_mode,"0") && conn_fail_count<=RECONNECT_INTERVAL)
			{
				conn_fail_count++;
				printf("conn_fail_count: %i\n",conn_fail_count);
			}
			
			
			
			
			if(!strcmp(op_mode,"2") && LOCAL_WIFI_connection_test()) // if failed to connect to LOCAL WIFI
			{
				change_to_AP();
			}
			else if(!strcmp(op_mode,"2"))
			{
				conn_fail_count = 0;
			}
			
			// try to reconnect
			if(!strcmp(op_mode,"0") && conn_fail_count>=RECONNECT_INTERVAL)
			{
				snprintf(command, sizeof(command), "LOCAL_WIFI_SSID");
				FLASH_read(ssid,command);
				snprintf(command, sizeof(command), "LOCAL_WIFI_PSW");
				FLASH_read(psw,command);
				
				printf("Saved:\nssid= %i %s\npsw= %i %s\n",strlen(ssid),ssid,strlen(psw),psw);
				
				if(strlen(ssid) && strlen(psw) && strcmp(ssid,"0") && strcmp(psw,"0"))
				{
					printf("let's try to reconnect local WIFI...\n");
					close(server_socket);
					exit(EXIT_NEED_RECONNECT_WIFI_FR_SERVER);
					
				}
			}
			continue;
		}
		else if(new_server_socket < 0)
        {
            printf("Server Accept Failed!\n");
            break;
        }
		
		
		
		timeout.tv_sec = SOCKET_RECV_SEND_TIMEOUT;
		timeout.tv_usec = 0;
		if(setsockopt(new_server_socket,SOL_SOCKET,SO_SNDTIMEO,(char *)&timeout,sizeof(struct timeval)))
		{
			printf("cannot set socket timeout!\n");
			break;
		}
		if(setsockopt(new_server_socket,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval)))
		{
			printf("cannot set socket timeout\n");
			break;
		}
		
		Phone_IP[loop_count].sin_addr = client_addr.sin_addr;
		
		
        char buffer[BUFFER_SIZE];
		char request[REQUEST_MAX_SIZE+1];
		
	
		if(MODE_EMPTY==mode)
		{
			bzero(buffer, BUFFER_SIZE);
			bzero(request, REQUEST_MAX_SIZE+1);
			length = recv(new_server_socket,buffer,BUFFER_SIZE,0);
			if (length < 0)
			{
				printf("Server Recieve Data Failed!\n");
				break;
			}
			strncpy(request, buffer, strlen(buffer)>REQUEST_MAX_SIZE?REQUEST_MAX_SIZE:strlen(buffer));
			printf("%s\n",request);
		
		
			if(strstr(request,"hello"))
			{
				if(strstr(request,"iniWIFI"))
				{
					bzero(buffer, BUFFER_SIZE);
					strncpy(buffer, "rdy:iniWIFI", 11);
					if((send(new_server_socket,buffer,strlen(buffer),0))<=0)
					{
						printf("server send data failed!\n");
						close(new_server_socket);
						mode = MODE_EMPTY;
						strcpy(Control_Mode,CONTROL_MODE_EMPTY);
					
						if(loop_count == (NUM_OF_PHONES_MAX-1))
						{
							loop_count = 0;
						}else{
							loop_count++;
						}
						continue;
					}
					mode = MODE_INI_WIFI;
					
					printf("finished send\n");
				}
				
				else if(strstr(request,"iniConnection"))
				{
					bzero(buffer, BUFFER_SIZE);
					strncpy(buffer, "rdy:iniConnection", 17);
					if((send(new_server_socket,buffer,strlen(buffer),0))<=0)
					{
						printf("server send data failed!\n");
						close(new_server_socket);
						mode = MODE_EMPTY;
						strcpy(Control_Mode,CONTROL_MODE_EMPTY);
					
						if(loop_count == (NUM_OF_PHONES_MAX-1))
						{
							loop_count = 0;
						}else{
							loop_count++;
						}
						continue;
					}
					// mode = MODE_INI_CONNECTION;
					close(new_server_socket);
					if(loop_count == (NUM_OF_PHONES_MAX-1))
					{
						loop_count = 0;
					}else{
						loop_count++;
					}
					
					
					// //test code
					// char command[COMMAND_CALL_SIZE];
					// // snprintf(command, sizeof(command), "insmod ir_sensor.ko");
					// // int callState = cmdCall(command);
					// // snprintf(command, sizeof(command), "mknod /dev/ir c 214 0");
					// // callState = cmdCall(command);
					// snprintf(command, sizeof(command), "./ir_sensor l");
					// int callState = cmdCall(command);
					
					
					continue;
				}
				
				else if(strstr(request,"iniControl"))
				{
					char *cmd_tmp;
					cmd_tmp = strchr(request,':');
					cmd_tmp += sizeof(char);
					cmd_tmp = strchr(cmd_tmp,':');
					cmd_tmp += sizeof(char);
					strcpy(Control_Mode,cmd_tmp);
					
					printf("Control mode: %s\n",Control_Mode);
					bzero(buffer, BUFFER_SIZE);
					if(!strcmp(Control_Mode,"0"))
						strncpy(buffer, "failed:iniControl", 17);
					else
						strncpy(buffer, "rdy:iniControl:1", 16);
					if((send(new_server_socket,buffer,strlen(buffer),0))<=0)
					{
						printf("server send data failed!\n");
						close(new_server_socket);
						mode = MODE_EMPTY;
						strcpy(Control_Mode,CONTROL_MODE_EMPTY);
					
						if(loop_count == (NUM_OF_PHONES_MAX-1))
						{
							loop_count = 0;
						}else{
							loop_count++;
						}
						continue;
					}
					mode = MODE_INI_CONTROL;
				}
				
				else if(strstr(request,"iniLearn"))
				{
					bzero(buffer, BUFFER_SIZE);
					strncpy(buffer, "rdy:iniLearn", 12);
					if((send(new_server_socket,buffer,strlen(buffer),0))<=0)
					{
						printf("server send data failed!\n");
						close(new_server_socket);
						mode = MODE_EMPTY;
						strcpy(Control_Mode,CONTROL_MODE_EMPTY);
					
						if(loop_count == (NUM_OF_PHONES_MAX-1))
						{
							loop_count = 0;
						}else{
							loop_count++;
						}
						continue;
					}
					mode = MODE_INI_LEARNING;
					
					// close(new_server_socket);
					
					// read from IR receiver
					printf("start to learn from IR sensor...\n");
					snprintf(command, sizeof(command), "./ir_sensor l");
					printf("command:%s\n",command);
					callState = cmdCall(command);
					
					
					
					// send data to smartphone
					// char *ip_t = inet_ntoa(Phone_IP[loop_count].sin_addr);
					// printf("requesting device ip: %s\n",ip_t);
					// learning_mode(ip_t);
					
					// char command[COMMAND_CALL_SIZE];
					// char *arg1 = "10.10.10.3";
					// snprintf(command, sizeof(command), "./server_learning_mode_sender %s",arg1);
					// printf("command:%s\n",command);
					// int callState = cmdCall(command);
					
					// if(loop_count == (NUM_OF_PHONES_MAX-1))
					// {
						// loop_count = 0;
					// }else{
						// loop_count++;
					// }
					// continue;
				}
			}
		}
		
		if(MODE_INI_CONTROL != mode && MODE_INI_LEARNING != mode)
		{
			bzero(buffer, BUFFER_SIZE);
			length = recv(new_server_socket,buffer,BUFFER_SIZE,0);
			if (length < 0)
			{
				printf("Server Recieve Data Failed!\n");
				break;
			}
			bzero(request, REQUEST_MAX_SIZE+1);
			strncpy(request, buffer, strlen(buffer)>REQUEST_MAX_SIZE?REQUEST_MAX_SIZE:strlen(buffer));

			printf("%s\n",request);
			
			if(strstr(request,"info"))
			{
				if(MODE_INI_WIFI==mode)
				{
					// save the ssid and password to some where
					char *pwd_tmp, *ssid_tmp;
					pwd_tmp = strchr(request,'/');
					*pwd_tmp = '\0';
					pwd_tmp += sizeof(char);
					ssid_tmp = strchr(request,':')+1;
					strcpy(Local_WIFI_pwd,pwd_tmp);
					strcpy(Local_WIFI_SSID,ssid_tmp);
					
					printf("WIFI password: %s\nWIFI SSID: %s\n",Local_WIFI_pwd,Local_WIFI_SSID);
					mode = MODE_EMPTY;
					
					bzero(buffer, BUFFER_SIZE);
					strncpy(buffer, "recvd:iniWIFI", 13);
					if((send(new_server_socket,buffer,strlen(buffer),0))<=0)
					{
						printf("server send data failed!\n");
						close(new_server_socket);
						mode = MODE_EMPTY;
						strcpy(Control_Mode,CONTROL_MODE_EMPTY);
					
						if(loop_count == (NUM_OF_PHONES_MAX-1))
						{
							loop_count = 0;
						}else{
							loop_count++;
						}
						continue;
					}
					
					close(new_server_socket);
					close(server_socket);
					
					if(loop_count == (NUM_OF_PHONES_MAX-1))
					{
						loop_count = 0;
					}else{
						loop_count++;
					}
					
					
					// save ssid and password to flash
					snprintf(command, sizeof(command), "nvram_set 2860 LOCAL_WIFI_SSID %s", Local_WIFI_SSID);
					printf("command:%s\n",command);
					callState = cmdCall(command);
					snprintf(command, sizeof(command), "nvram_set 2860 LOCAL_WIFI_PSW %s", Local_WIFI_pwd);
					printf("command:%s\n",command);
					callState = cmdCall(command);
					
					// connect to local WIFI
					exit(EXIT_NEED_RECONNECT_WIFI_FR_SERVER);
					
					
					// system("./addr_bc");
					
					
					continue;
				}
				
				
			}
		}
		
		else if(MODE_INI_CONTROL==mode)
		{
			if(!strcmp(CONTROL_MODE_IR_SEND,Control_Mode))
			{
				strcpy(file_name,IR_CONTROL_FILE_NAME);
			
				FILE * fp = fopen(file_name,"w");
				if(NULL == fp )
				{
					printf("File:\t%s Can Not Open To Write\n", file_name);
					exit(1);
				}
				
				int flength = 0;
				int is_failed = 0;
				int is_end = 0;
				char *end_flag;
				bzero(buffer,BUFFER_SIZE);
				while( flength = recv(new_server_socket,buffer,BUFFER_SIZE,0))
				{
					printf("flength: %i\n",flength);
					printf("buffer: %s\n",buffer);
					if(flength < 0)
					{
						printf("Recieve Data From Client Failed!\n");
						is_failed = 1;
						break;
					}
					char *c_t;
					if(c_t=strstr(buffer,SOCKET_END_FLAG))
					{
						// while(*c_t!='\0')
						// {
							// *c_t = '\0';
							// c_t += 1;//sizeof(char);
						// }
						printf("found end flag\n");
						is_end = 1;
					}
			//        int write_flength = write(fp, buffer,flength);
					
					
					if(is_end)
					{
						int write_length = fwrite(buffer,sizeof(char),flength-sizeof(SOCKET_END_FLAG)+1,fp);
						write_length += 4;
						printf("write_length: %i\n",write_length);
						if (write_length<flength)
						{
							printf("File:\t%s Write Failed\n", file_name);
							is_failed = 1;
							break;
						}
						bzero(buffer,BUFFER_SIZE);
						break;
					}else{
						int write_length = fwrite(buffer,sizeof(char),flength,fp);
						write_length += 4;
						printf("write_length: %i\n",write_length);
						if (write_length<flength)
						{
							printf("File:\t%s Write Failed\n", file_name);
							is_failed = 1;
							break;
						}
					
					}
					bzero(buffer,BUFFER_SIZE);
					
				}
				if(is_failed)
				{
					printf("receive file failed\n");
					bzero(buffer, BUFFER_SIZE);
					sprintf(buffer, "failed:iniControl:%s", Control_Mode);
					if((send(new_server_socket,buffer,strlen(buffer),0))<=0)
					{
						printf("server send data failed!\n");
						close(new_server_socket);
						mode = MODE_EMPTY;
						strcpy(Control_Mode,CONTROL_MODE_EMPTY);
					
						if(loop_count == (NUM_OF_PHONES_MAX-1))
						{
							loop_count = 0;
						}else{
							loop_count++;
						}
						fclose(fp);
						continue;
					}
				}
				else 
				{
					printf("received: %s\n",file_name);
					
					bzero(buffer, BUFFER_SIZE);
					sprintf(buffer, "failed:iniControl:%s", Control_Mode);
					if((send(new_server_socket,buffer,strlen(buffer),0))<=0)
					{
						printf("server send data failed!\n");
						close(new_server_socket);
						mode = MODE_EMPTY;
						strcpy(Control_Mode,CONTROL_MODE_EMPTY);
					
						if(loop_count == (NUM_OF_PHONES_MAX-1))
						{
							loop_count = 0;
						}else{
							loop_count++;
						}
						fclose(fp);
						continue;
					}
					
					// do the control operation
					//..
					//..
					
					// snprintf(command, sizeof(command), "insmod ir_sensor.ko");
					// int callState = cmdCall(command);
					// snprintf(command, sizeof(command), "mknod /dev/ir c 214 0");
					// callState = cmdCall(command);
					snprintf(command, sizeof(command), "./ir_sensor f");
					callState = cmdCall(command);
				}
				fclose(fp);
				strcpy(Control_Mode,CONTROL_MODE_EMPTY);
				
				
			}
			mode = MODE_EMPTY;
		}
		
		else if(MODE_INI_LEARNING==mode)
		{
			int length = 0;
			
			
			char file_name[256];
			strcpy(file_name,IR_LEARNING_FILE_NAME);
			
			printf("prepare to send!\n");
			FILE * fp = fopen(file_name,"r");// detect if target file exists
			if(NULL == fp )
			{
				printf("File:\t%s Not Found\n", file_name);
				if(loop_count == (NUM_OF_PHONES_MAX-1))
				{
					loop_count = 0;
				}else{
					loop_count++;
				}
				mode = MODE_EMPTY;
				
				
				bzero(buffer, BUFFER_SIZE);
				sprintf(buffer, "failed:iniLearn:%s",IR_LEARNING_ERROR_EMPTY_FILE);
				if(send(new_server_socket,buffer,strlen(buffer),0)<=0)
				{
					printf("Send File:\t%s Failed\n", file_name);
				}
				
				continue;
			}
			
			
			fp = fopen(file_name,"a+"); // append ending flag into the file
			fputs(SOCKET_END_FLAG,fp);
			fclose(fp);
			
			printf("ready to send!\n");
			fp = fopen(file_name,"r");
			if(NULL == fp )
			{
				printf("File:\t%s Not Found\n", file_name);
			}
			else
			{
				bzero(buffer, BUFFER_SIZE);
				int file_block_length = 0;
				int success_flag = 0;

				while( (file_block_length = fread(buffer,sizeof(char),BUFFER_SIZE,fp))>0)
				{
					printf("file_block_length = %d\n",file_block_length);

					if(send(new_server_socket,buffer,file_block_length,0)<=0)
					{
						printf("Send File:\t%s Failed\n", file_name);
					}
					bzero(buffer, BUFFER_SIZE);
				}

				fclose(fp);
				// printf("File:\t%s Transfer Finished\nremoving file...\n",file_name);
				// snprintf(command, sizeof(command), "rm learn.ir");
				// if(callState = cmdCall(command))
				// {
					// printf("fail to remove learnt file! System will ignore this error!\n");
				// }
			}
		
			
			
			length = 0;
			bzero(buffer,BUFFER_SIZE);
			length = recv(new_server_socket,buffer,BUFFER_SIZE,0);
			if (length <= 0)
			{
				printf("Server Receive Data Failed!\n");
			}
			bzero(request, REQUEST_MAX_SIZE+1);
			strncpy(request, buffer, strlen(buffer)>REQUEST_MAX_SIZE?REQUEST_MAX_SIZE:strlen(buffer));
			printf("%s\n",request);
			
			if(strstr(request,"recvd:iniLearn"))
			{
				
			}
			else if(strstr(request,"failed:iniLearn"))
			{
			
			}
			
			
			mode = MODE_EMPTY;
		}
		
		
        
		printf("loop ends\n");
        close(new_server_socket);
		
		if(loop_count == (NUM_OF_PHONES_MAX-1))
		{
			loop_count = 0;
		}else{
			loop_count++;
		}
    }
    close(server_socket);
    return 0;
}

inline void change_to_AP()
{
	char command[COMMAND_CALL_SIZE];
	int callState;
	
	printf("change to AP mode...\n");
	snprintf(command, sizeof(command), "nvram_set 2860 OperationMode 0");
	printf("command:%s\n",command);
	callState = cmdCall(command);
	// snprintf(command, sizeof(command), "nvram_set 2860 dhcpEnabled 1");
	// printf("command:%s\n",command);
	// callState = cmdCall(command);
	snprintf(command, sizeof(command), "internet.sh");
	printf("command:%s\n",command);
	callState = cmdCall(command);
	device_ap_reset();
}

inline void change_to_STA()
{
	char command[COMMAND_CALL_SIZE];
	int callState;
	
	printf("change to STA mode...\n");
	snprintf(command, sizeof(command), "nvram_set 2860 OperationMode 2");
	printf("command:%s\n",command);
	callState = cmdCall(command);
	// snprintf(command, sizeof(command), "nvram_set 2860 dhcpEnabled 0");
	// printf("command:%s\n",command);
	// callState = cmdCall(command);
	snprintf(command, sizeof(command), "internet.sh");
	printf("command:%s\n",command);
	callState = cmdCall(command);
}


int iniServerSocket()
{
	char command[COMMAND_CALL_SIZE];
	int retry, callState;
	struct timeval timeout;
	struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(HELLO_WORLD_SERVER_PORT);

	retry = 0;
	
	
    int server_socket = socket(PF_INET,SOCK_STREAM,0);
    if( server_socket < 0)
    {
        printf("Create Socket Failed!");
        exit(1);
    }
 
    int opt =1;
    setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    
    while( bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr)))
    {
        printf("Server Bind Port : %d Failed!", HELLO_WORLD_SERVER_PORT); 
		if(0==retry)
		{
			retry += 1;
			snprintf(command, sizeof(command), "internet.sh");
			printf("command:%s\n",command);
			callState = cmdCall(command);
		}
		else
		{
			exit(1);
		}
    }

    if ( listen(server_socket, LENGTH_OF_LISTEN_QUEUE) )
    {
        printf("Server Listen Failed!"); 
        exit(1);
    }
	
	printf("Server Init Success!"); 
	
	return(server_socket);

}






int cmdCall(char *cmd)// return 0 means success, 1 means failure
{
	char result_buf[COMMAND_CALL_SIZE];
	int rc = 0; 
	FILE *fp;

	fp = popen(cmd, "r");
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
		
		// solve the bug that internet.sh cannot exit correctly
		if(strstr(result_buf,"udhcpc") && strstr(result_buf,"started"))
		{
			break;
		}
		
		printf("command [%s] output [%s]\r\n", cmd, result_buf);
	}

	rc = pclose(fp);
	if(-1 == rc)
	{
		printf("fail to close file pointer");
		return 1;
	}
	else
	{
		printf("command [%s] closing status of child process [%d] returl value [%d]\r\n", cmd, rc, WEXITSTATUS(rc));
	}

	return 0;
}

/*
int conn_WIFI(char *SSID, char *PSW)// return 0 means success, 1 means failure
{
	char *SSID_t;
	char AuthMode[12];
	char EncrypType[12];
	char *key_identifier[2];
	char SSID_l[SSID_MAX];
	char IP[IP_MAX];
	char *IP_t;
	char command[COMMAND_CALL_SIZE];
	int callState;
	int mode_t = 0;
	int SSID_match = 0;
	strcpy(SSID_l,"");
	strcpy(IP,"");
	strcpy(AuthMode,"");
	strcpy(EncrypType,"");
	
	snprintf(command, sizeof(command), "iwpriv ra0 set SiteSurvey=1");
	callState = cmdCall(command);
	snprintf(command, sizeof(command), "iwpriv ra0 get_site_survey");
	
	char result_buf[COMMAND_CALL_SIZE];
	int rc; 
	FILE *fp;
	
	int time_out_tmp = 0;
	while((time_out_tmp<TIMEOUT_TMP_MAX) && (!strlen(AuthMode) || !strlen(EncrypType)))
	{
		sleep(TIMEOUT_TIME_IN_S);
		// char *SSID_t = NULL;
		time_out_tmp += 1;
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
			
			if((!strlen(AuthMode) || !strlen(EncrypType)) && (SSID_t=strstr(result_buf,SSID)))
			{
				char *secure_mode[3], *secure_t, *c_t;
				secure_t = NULL;
				c_t = NULL;
				secure_mode[0] = "NONE";
				secure_mode[1] = "WPA";
				secure_mode[2] = "WEP";
				
				SSID_t = strchr(SSID_t,':');
				SSID_t = SSID_t+18*sizeof(char);
				c_t = strchr(SSID_t,' ');
				*c_t = '\0';
				printf("SSID_t: %s\n",SSID_t);
				while(!secure_t)
				{
					secure_t = strstr(SSID_t,secure_mode[mode_t]);
					printf("secure_t: %s\n",secure_t);
					if(mode_t<3)
					{
						mode_t++;
					}
					else
					{
						printf("SSID found! But failed to find the secure mode of the specified WIFI!\n");
						pclose(fp);
						return 1;
					}
				}
				printf("mode_t: %i\n",mode_t);
				
				if(mode_t==1)
				{
					
					strcpy(AuthMode,"OPEN");
					printf("AuthMode: %s\n",AuthMode);
					strcpy(EncrypType,SSID_t);
					printf("EncrypType: %s\n",EncrypType);
				}
				else if(mode_t==2)
				{
					
					c_t = strchr(SSID_t,'/');
					*c_t = '\0';
					printf("AuthMode: %s\n",SSID_t);
					strcpy(AuthMode,SSID_t);
					SSID_t = c_t + sizeof(char);
					printf("EncrypType: %s\n",SSID_t);
					strcpy(EncrypType,SSID_t);
				}
				else if(mode_t==3)
				{
					strcpy(AuthMode,"SHARED");
					printf("AuthMode: %s\n",AuthMode);
					strcpy(EncrypType,SSID_t);
					printf("EncrypType: %s\n",EncrypType);
				}
				else
				{
					printf("fatal error!\n");
					return 1;
				}
				// break;
			}
			printf("result buff: %s\n",result_buf);
		}
		
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
		
		
		if(strlen(AuthMode) && strlen(EncrypType))
		{
			printf("successfully read secure mode...\n");
		}
	}
	
	if(!strlen(AuthMode) || !strlen(EncrypType))
	{
		printf("failed to find specified SSID!\n");
		return 1;
	}
	
	
	
	key_identifier[0] = "Key1";
	key_identifier[1] = "WPAPSK";
	short int count;


	if(4==count)
	{
		printf("failed to connect local WIFI");
		return 1;
	}
	
	
	snprintf(command, sizeof(command), "iwpriv ra0 set NetWorkType=Infra");
	callState = cmdCall(command);
	if(callState)
	{
		printf("cannot set network type!\n");
		return 1;
	}
	
	
	snprintf(command, sizeof(command), "iwpriv ra0 set AuthMode=%s",AuthMode);
	printf("command:%s\n",command);
	callState = cmdCall(command);
	if(callState)
	{
		printf("cannot set authentication mode!\n");
		return 1;
	}
	
	snprintf(command, sizeof(command), "iwpriv ra0 set EncrypType=%s",EncrypType);
	printf("command:%s\n",command);
	callState = cmdCall(command);
	if(callState)
	{
		printf("cannot set Encryption type!\n");
		return 1;
	}
	
	if(3==mode_t)
	{
		snprintf(command, sizeof(command), "iwpriv ra0 set DefaultKeyID=1");
		printf("command:%s\n",command);
		callState = cmdCall(command);
		if(callState)
		{
			printf("cannot set Default Key ID type!\n");
			return 1;
		}
		
		
		snprintf(command, sizeof(command), "iwpriv ra0 set Key1=%s",PSW);
		printf("command:%s\n",command);
		callState = cmdCall(command);
		if(callState)
		{
			printf("cannot set key!\n");
			return 1;
		}
		
	}
	else if(2==mode_t)
	{
		snprintf(command, sizeof(command), "iwpriv ra0 set WPAPSK=%s",PSW);
		printf("command:%s\n",command);
		callState = cmdCall(command);
		if(callState)
		{
			printf("cannot set password!\n");
			return 1;
		}
	}
	
	snprintf(command, sizeof(command), "iwpriv ra0 set SSID=%s",SSID);
	printf("command:%s\n",command);
	callState = cmdCall(command);
	if(callState)
	{
		printf("cannot set SSID!\n");
		return 1;
	}
	
	snprintf(command, sizeof(command), "iwconfig ra0");
	
	time_out_tmp = 0;
	while((time_out_tmp<TIMEOUT_TMP_MAX) && (!strlen(SSID_l)))
	{
		sleep(TIMEOUT_TIME_IN_S);
		// char *SSID_t = NULL;
		time_out_tmp += 1;
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
			
			if(!strlen(SSID_l) && (SSID_t=strstr(result_buf,"ESSID:\"")))
			{
				SSID_t = SSID_t+7*sizeof(char);
				c_t = strchr(SSID_t,'\"');
				*c_t = '\0';
				printf("connected to SSID: %s\n",SSID_t);
				strcpy(SSID_l,SSID_t);
				// break;
			}
			
			if(!strlen(IP) && (IP_t=strstr(result_buf,"inet addr:")))
			{
				printf("read IP_t...\n");
				IP_t = IP_t+10*sizeof(char);
				c_t = strstr(result_buf,"  Bcast");
				*c_t = '\0';
				strcpy(IP,IP_t);
				printf("device IP_t: %s\n",IP_t);
			}
			// printf("command [%s] output [%s]\r\n", command, result_buf);
			printf("get SSID: %s\n",SSID_l);
		}
		
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
		
		
		// strcat(SSID,"\0");
		if(strcmp(SSID,SSID_l))
		{
			printf("SSID does not match: %s/%s\n",SSID_l,SSID);
			SSID_match = 0;
		}
		else
		{
			printf("SSID match: %s/%s\n",SSID_l,SSID);
			SSID_match = 1;
			break;
		}
	}
	
	if(!SSID_match)
	{
		return 1;
	}
	
	
	
	// end the old udhcpc
	FILE *udhcp_pid_f = fopen("/var/run/udhcpc.pid","r");
	char buffer_pid[32];
	int udhcp_pid;
	if(NULL == udhcp_pid_f )
	{
		printf("File:\t/var/run/udhcpc.pid Not Found\n");
		return 1;
	}
	if(fread(buffer_pid,sizeof(char),32,udhcp_pid_f) > 0)
	{
		snprintf(command, sizeof(command), "kill %s",buffer_pid);
		callState = cmdCall(command);
	}
	
	
	
	snprintf(command, sizeof(command), "udhcpc -i ra0 -s /sbin/udhcpc.sh -p /var/run/udhcpc.pid");
	callState = cmdCall(command);
	printf("restart dhcp...\n");
	
	
	snprintf(command, sizeof(command), "ifconfig ra0");
	
	time_out_tmp = 0;
	while((time_out_tmp<IP_TIMEOUT_TMP_MAX) && !strlen(IP))
	{
		sleep(IP_TIMEOUT_TIME_IN_S);
		// char *SSID_t = NULL;
		time_out_tmp += 1;
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
			
			if(!strlen(IP) && (IP_t=strstr(result_buf,"inet addr:")))
			{
				printf("read IP_t...\n");
				IP_t = IP_t+10*sizeof(char);
				c_t = strstr(result_buf,"  Bcast");
				*c_t = '\0';
				strcpy(IP,IP_t);
				printf("device IP_t: %s\n",IP_t);
			}
			// printf("command [%s] output [%s]\r\n", command, result_buf);
			printf("get IP: %s\n",IP);
		}
		
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
		
		
		if(!strlen(IP))
		{
			printf("cannot get IP!\n");
		}
		else
		{
			printf("ip got: %s\n",IP);
			return 0;
		}
	}

	return 1;
}
*/
int FLASH_read(char *result, char *c_t)// re 6turn 0 means success, 1 means failure
{
	char result_buf[COMMAND_CALL_SIZE], cmd[COMMAND_CALL_SIZE];
	int rc = 0;
	int is_get = 0;
	FILE *fp;
	
	sprintf(cmd,"nvram_get 2860 %s",c_t);

	fp = popen(cmd, "r");
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
		
		if(strlen(result_buf)>0)
		{
			printf("result_buf: %i %s\n",strlen(result_buf),result_buf);
			strcpy(result,result_buf);
			is_get = 1;
			break;
		}
		// printf("command [%s] output [%s]\r\n", cmd, result_buf);
	}

	rc = pclose(fp);
	if(-1 == rc)
	{
		printf("fail to close file pointer");
		return 1;
	}
	else
	{
		printf("command [%s] closing status of child process [%d] return value [%d]\r\n", cmd, rc, WEXITSTATUS(rc));
	}
	
	if(is_get)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

int LOCAL_WIFI_connection_test()
{
	char command[COMMAND_CALL_SIZE];
	snprintf(command, sizeof(command), "iwconfig ra0");
	char result_buf[COMMAND_CALL_SIZE];
	int rc; 
	FILE *fp;
	char *SSID_t;
	char SSID_l[SSID_MAX];
	strcpy(SSID_l,"");
	
	int time_out_tmp = 0;
	while((time_out_tmp<TIMEOUT_CONN_TEST_TMP_MAX) && !strlen(SSID_l))
	{
		sleep(TIMEOUT_TIME_IN_S);
		// char *SSID_t = NULL;
		time_out_tmp += 1;
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
			
			if(!strlen(SSID_l) && (SSID_t=strstr(result_buf,"ESSID:\"")))
			{
				SSID_t = SSID_t+7*sizeof(char);
				c_t = strchr(SSID_t,'\"');
				*c_t = '\0';
				printf("connected to SSID: %s\n",SSID_t);
				strcpy(SSID_l,SSID_t);
				// break;
			}
			// printf("command [%s] output [%s]\r\n", command, result_buf);
			printf("get SSID: %s\n",SSID_l);
		}
		
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
		
		
		// strcat(SSID,"\0");
		if(strlen(SSID_l))
		{
			printf("connection is fine...\n");
			return 0;
		}
	}
	printf("connection is done...\n");
	return 1;
}


int device_ap_reset()
{
	char command[COMMAND_CALL_SIZE];
	snprintf(command, sizeof(command), "iwpriv ra0 set NetWorkType=Infra");
	int callState = cmdCall(command);
	if(callState)
	{
		printf("cannot set network type!\n");
		return 1;
	}
	
	
	snprintf(command, sizeof(command), "iwpriv ra0 set AuthMode=WPA2PSK");
	printf("command:%s\n",command);
	callState = cmdCall(command);
	if(callState)
	{
		printf("cannot set authentication mode!\n");
		return 1;
	}
	
	snprintf(command, sizeof(command), "iwpriv ra0 set EncrypType=AES");
	printf("command:%s\n",command);
	callState = cmdCall(command);
	if(callState)
	{
		printf("cannot set Encryption type!\n");
		return 1;
	}
	
	
	
	snprintf(command, sizeof(command), "iwpriv ra0 set SSID=XCTRL_STATION");
	printf("command:%s\n",command);
	callState = cmdCall(command);
	if(callState)
	{
		printf("cannot set SSID!\n");
		return 1;
	}
	
	snprintf(command, sizeof(command), "iwpriv ra0 set WPAPSK=1234567890");
	printf("command:%s\n",command);
	callState = cmdCall(command);
	if(callState)
	{
		printf("cannot set password!\n");
		return 1;
	}
	
	return 0;
}

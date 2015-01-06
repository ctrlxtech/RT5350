#include <stdio.h>        // for printf
#include <stdlib.h>        // for exit
#include <string.h>        // for bzero

#include <errno.h>
#include <sys/wait.h>


#define COMMAND_CALL_SIZE 1024
#define SSID_MAX 1024
#define PSW_MAX 1024
#define IP_MAX 18

#define TIMEOUT_TMP_MAX 3
#define IP_TIMEOUT_TMP_MAX 20
#define TIMEOUT_TIME_IN_S 5
#define IP_TIMEOUT_TIME_IN_S 3
#define MAX_NUM_SITE_SURVEY_FAILURE 3

// exit code
#define EXIT_NEED_RECONNECT_WIFI_FR_SERVER 2
#define EXIT_NEED_RECONNECT_WIFI_FR_BCAST 3


int cmdCall(char *cmd);// return 0 means success, 1 means failure
int FLASH_read(char *result, char *c_t);
int conn_WIFI(char *SSID, char *PSW);
int device_ap_reset();
inline void change_to_AP();
inline void change_to_STA();


int main(int argc, char **argv)
{
	// change ra0 mode to AP 
	char command[COMMAND_CALL_SIZE], ssid[SSID_MAX], psw[PSW_MAX], op_mode[2];
	int callState;
	snprintf(command, sizeof(command), "mknod /dev/ir c 214 0");
   callState = cmdCall(command);
	snprintf(command, sizeof(command), "LOCAL_WIFI_SSID");
	FLASH_read(ssid,command);
	snprintf(command, sizeof(command), "LOCAL_WIFI_PSW");
	FLASH_read(psw,command);
	printf("ssid= %i %s\npsw= %i %s\n",strlen(ssid),ssid,strlen(psw),psw);
	snprintf(command, sizeof(command), "OperationMode");
	FLASH_read(op_mode,command);
	printf("op_mode: %i %s\n",strlen(op_mode),op_mode);
	
	if(strlen(ssid) && strlen(psw) && strcmp(ssid,"0") && strcmp(psw,"0"))
	{
	    if(!strcmp(op_mode, "2"))
		{
		    printf("already in STA mode \n");
			printf("executing internet.sh \n");
			snprintf(command, sizeof(command), "internet.sh");
			callState = cmdCall(command);
			printf("finish executing internet.sh \n");
		}
		if(strcmp(op_mode,"2"))
		{
		    printf("change to STA mode \n");
			change_to_STA();
			printf("finish changing to STA mode \n");
		}
		
		label_5:
		printf("reconnect wifi...\n");
		int conn_wifi_state = conn_WIFI(ssid, psw);
		if(conn_wifi_state == EXIT_NEED_RECONNECT_WIFI_FR_BCAST)
		{
		    printf("fail to execute UDP_broadcast \n");
			printf("try to reconnect wifi \n");
			goto label_5;
		} 
		else if(conn_wifi_state) 
		{
		    printf("fail to connect to WIFI:ssid= %i %s\n", strlen(ssid),ssid);
			printf("change to AP mode \n");
			change_to_AP();
			printf("finish changing to AP mode \n");
		}
		else
		{
		    printf("Successfully connected to WIFI:ssid= %i %s\n", strlen(ssid),ssid);
		}
	}
	else
	{
	    printf("no correct format of ssid/pwd found in flash \n");
		printf("change to AP mode \n");
		change_to_AP();
		printf("finish changing to AP mode \n");
	}
	
	while(1)
	{
		printf("restart server\n");
		char command[COMMAND_CALL_SIZE];
		snprintf(command, sizeof(command), "./server");
		int callState = cmdCall(command);
		printf("callState: %i\n",callState);
		
		
		snprintf(command, sizeof(command), "LOCAL_WIFI_SSID");
		FLASH_read(ssid,command);
		snprintf(command, sizeof(command), "LOCAL_WIFI_PSW");
		FLASH_read(psw,command);
		printf("ssid= %i %s\npsw= %i %s\n",strlen(ssid),ssid,strlen(psw),psw);
		snprintf(command, sizeof(command), "OperationMode");
		FLASH_read(op_mode,command);
		printf("op_mode: %i %s\n",strlen(op_mode),op_mode);
		
		
		if(callState==EXIT_NEED_RECONNECT_WIFI_FR_SERVER)
		{
			if(strcmp(op_mode,"2"))
			{
				change_to_STA();
			}
			printf("reconnect wifi...\n");
			if(conn_WIFI(ssid, psw))
			{
				change_to_AP();
			}
			
		}
	}
	return 1;
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
	printf("Begin executing internet.sh \n");
	snprintf(command, sizeof(command), "internet.sh");
	printf("command:%s\n",command);
	callState = cmdCall(command);
	printf("Finish executing internet.sh \n");
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
	printf("Begin executing internet.sh \n");
	snprintf(command, sizeof(command), "internet.sh");
	printf("command:%s\n",command);
	callState = cmdCall(command);
	printf("Finish executing internet.sh \n");

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
	
	return WEXITSTATUS(rc);
}

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
		printf("command [%s] closing status of child process [%d] returl value [%d]\r\n", cmd, rc, WEXITSTATUS(rc));
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

/** Connect to wifi and broadcast IP*/
int conn_WIFI(char *SSID, char *PSW)// return 0 means success, 1 means failure, 3 means udp broadcast fail
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
	
	char result_buf[COMMAND_CALL_SIZE];
	int rc; 
	FILE *fp;
	int time_out_tmp;
    int num_fail_open_sitesurvey = 0;
	
	label_0:
	time_out_tmp = 0;
	snprintf(command, sizeof(command), "iwpriv ra0 set SiteSurvey=1");
	callState = cmdCall(command);
	snprintf(command, sizeof(command), "iwpriv ra0 get_site_survey");
	
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
				char *secure_mode[4], *secure_t, *c_t;
				secure_t = NULL;
				c_t = NULL;
				secure_mode[0] = "NONE";
				secure_mode[1] = "WPA2";
				secure_mode[2] = "WEP";
				secure_mode[3] = "WPA";
				
				SSID_t = strchr(SSID_t,':');
				SSID_t = SSID_t+18*sizeof(char);
				c_t = strchr(SSID_t,' ');
				*c_t = '\0';
				printf("SSID_t: %s\n",SSID_t);
				while(!secure_t)
				{
					secure_t = strstr(SSID_t,secure_mode[mode_t]);
					printf("secure_t: %s\n",secure_t);
					if(mode_t<4)
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
				else if(mode_t==4)
				{
				    c_t = strchr(SSID_t,'/');
					*c_t = '\0';
					printf("AuthMode: %s\n",SSID_t);
					strcpy(AuthMode,SSID_t);
					SSID_t = c_t + sizeof(char);
					printf("EncrypType: %s\n",SSID_t);
					strcpy(EncrypType,SSID_t);
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
			printf("command [%s] closing status of child process [%d] reture value [%d]\r\n", command, rc, WEXITSTATUS(rc));
		}
		
		
		if(strlen(AuthMode) && strlen(EncrypType))
		{
			printf("successfully read secure mode...\n");
		}
	}
	
	if(!strlen(AuthMode) || !strlen(EncrypType))
	{
		printf("failed to find specified SSID!\n");
		printf("fail to execute iwpriv ra0 get_site_survey \n");
		printf("execute internet.sh \n");
		num_fail_open_sitesurvey++;
		snprintf(command, sizeof(command), "internet.sh");
		callState = cmdCall(command);
		if(num_fail_open_sitesurvey < MAX_NUM_SITE_SURVEY_FAILURE)
		{
		    goto label_0;
		}
		else
		{
		    printf("cannot open site survey, force quit! \n");
		    return 1;
		}
	}
	
	
	
	key_identifier[0] = "Key1";
	key_identifier[1] = "WPAPSK";
	short int count;


	if(4==count)
	{
		printf("failed to connect local WIFI");
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
	
	snprintf(command, sizeof(command), "iwpriv ra0 set NetWorkType=Infra");
	callState = cmdCall(command);
	if(callState)
	{
		printf("cannot set network type!\n");
		return 1;
	}
	
	
	if(4==mode_t)
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
		goto label_3;
	}
	if(fread(buffer_pid,sizeof(char),32,udhcp_pid_f) > 0)
	{
		snprintf(command, sizeof(command), "kill %s",buffer_pid);
		callState = cmdCall(command);
	}
	
	label_3:
	printf("restart dhcp...\n");
	snprintf(command, sizeof(command), "udhcpc -i ra0 -s /sbin/udhcpc.sh -p /var/run/udhcpc.pid &");
	label_1:
	fp = popen(command, "r");
	sleep(5);
	if(NULL == fp)
	{
		printf("fail to execute popen!");
		return 1;
	}
	while(fgets(result_buf, sizeof(result_buf), fp) == NULL)
	{
	    printf("fail to execute udhcpc -i ra0 -s /sbin/udhcpc.sh -p /var/run/udhcpc.pid & \n execute the command again \n");
		fp = popen(command, "r");
		goto label_1;
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
		
		printf("command [%s] output [%s]\r\n", command, result_buf);
	}
	close(fp);
	
	/*
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
			printf("result buffer: %s\n", result_buf);
			
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
	*/
	snprintf(command, sizeof(command), "./UDP_broadcast");
	callState = cmdCall(command);
	
	return callState;
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

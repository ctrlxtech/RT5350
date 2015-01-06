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
int main(int argc, char **argv)
{
	char command[COMMAND_CALL_SIZE], op_mode[2];
	int callState;
	
	snprintf(command, sizeof(command), "nvram_set 2860 OperationMode 0");
	callState = cmdCall(command);
	snprintf(command, sizeof(command), "internet.sh");
	callState = cmdCall(command);
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
/*
 * =====================================================================================
 *
 *       Filename:  getUserinfor.c
 *
 *    Description:  get User info
 *
 *        Version:  1.0
 *        Created:  2010年11月30日 14时37分47秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Xuan Fei (悬非), xuanfei2046@163.com
 *        Company:  GUN GPL
 *
 * =====================================================================================
 */

#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include "clog.h"


//logrotate
void wrename(char *path,int pathlen, int begin){
			int count = begin + 1;
			char *newpath = calloc(1,pathlen+2);
			char *tmppath = calloc(1,pathlen+2);
			//printf("%s --> %d\n",path,count);
			sprintf(newpath,"%s.%d",path,count);
			if(access(newpath,F_OK)){
				int i = count;
				for(i; i > 0; i--){
					sprintf(newpath,"%s.%d",path,i);
					int t = i - 1;
					if(t){
						sprintf(tmppath,"%s.%d",path,i-1);
					}else{
						//默认log名称
						sprintf(tmppath,"%s",path);
					}

					//printf("Rename%s -> %s\n",tmppath,newpath);
					int re = rename(tmppath,newpath);
					if(re)
						perror("rename");
				}
			}else{
				wrename(path,pathlen,count);
			}

			free(tmppath);
			free(newpath);
}

int writelog(char *path, char *log)
{
	if(!access(path,F_OK)){
		struct stat buf;
		stat(path,&buf);
		if(buf.st_size > MAX_LOGFILE_SIZE){
			wrename(path,strlen(path),0);
		}
	}

    int fd =
	open(path, O_APPEND | O_WRONLY | O_CREAT,
	     S_IRWXU | S_IRWXG | S_IRWXO);
	if(fd >= 0){
    	write(fd, log, strlen(log));
	}else{
		perror("open cssh log");
	}
    close(fd);
    return 0;
}

int clog_r(char *rip,char *cmd, int cmdlen)
{
    //printf("wirtelog func:%s %d\n",cmd,cmdlen);
    char username[255] = {0};
    uid_t my_uid;
    struct passwd *my_info;
    my_uid = getuid();
    my_info = getpwuid(my_uid);	//可以指定用户id
	if (my_info) {
		strcpy(username, my_info->pw_name);
	} else {
		strcpy(username, "Unkown");
	}

    time_t seconds = time((time_t *) NULL);
    struct tm *t;
    t = localtime(&seconds);
    char strtime[64] = { 0 };
    sprintf(strtime, "%04d%02d%02d %02d:%02d:%02d",
	    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,  t->tm_hour,t->tm_min,
	    t->tm_sec);
    char *plog = (char *) malloc(cmdlen + 100);
    memset(plog, 0, cmdlen + 100);
    sprintf(plog, "%s %s IP[%s] CMD:%s\n", strtime, username, rip,cmd);
	char logfile[24] = LOGNAME;
    writelog(logfile, plog);
    free(plog);
    return 0;
}


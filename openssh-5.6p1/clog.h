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

#define MAX_LOGFILE_SIZE 	102400
#define LOGNAME			"cssh.log"


//logrotate
void wrename(char *path,int pathlen, int begin);

int writelog(char *path, char *log);

int clog_r(char *rip,char *cmd, int cmdlen);

#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


#include "ro_logger.h"

static void ro_getErrorDescription(char *errordesc);

void ro_log (const char *srvclnt,const char *fmt, ...) {
  va_list vap;
  /*int rc;*/
  time_t t;
  struct tm *ttime = NULL;

  static int fd_s = -1;
  static int fd_c = -1;
  int fd = -1;
  int srv = 0;
  static char buf[1024];
  static char buf2[1024];
  static char time_s[50];
  char errordesc[20];

  memset(buf,'\0',1024);
  memset(buf2,'\0',1024);
  memset(time_s,'\0',50);
  memset(errordesc,'\0',20);

  if(strcmp(srvclnt , RO_LOG_NAME_CHECKER) == 0) {
    srv = 1;
  }

  if(srv)
    fd= fd_s;
  else 
    fd = fd_c;

  if (fd < 0) {
    if(srv) {
      fd = open("/home/nwn/logs.0/nwRecvOver.txt", O_WRONLY|O_APPEND|O_CREAT, 0644);
    }
    else {
      fd = open("/home/nwn/logs.0/nwRecvOver.txt", O_WRONLY|O_APPEND|O_CREAT, 0644);
    }
  }  

  if (fd < 0) {
    fprintf(stderr,"Cannot write to log file. srv?=%d",srv);
    return;
  }
  if(srv) {
    fd_s = fd;
  } 
  else 
    fd_c = fd;

  va_start(vap, fmt);
  vsnprintf(buf, (sizeof(buf)), fmt, vap);
  va_end(vap);


  /* Add time to log */
  t = time(NULL);
  ttime = localtime(&t);
  strftime(time_s,49,"%G-%m-%d %H:%M:%S",ttime);

  /* Add error descritopn */
  ro_getErrorDescription(errordesc);
  sprintf(buf2,"[%s] %s: %s (%s)\n",time_s, srvclnt, buf, errordesc);

  write(fd, buf2, strlen(buf2));
}

static void ro_getErrorDescription(char *errordesc) {
  static char buff[20];
  memset(buff, '\0',20);

  if(errno == 0) {
    strcpy(errordesc, "");
  }

  switch(errno) {
    case EACCES: strcpy(errordesc, "EACCES"); break;
    case EEXIST: strcpy(errordesc, "EEXIST"); break;
    case EFAULT: strcpy(errordesc, "EFAULT"); break;
    case EFBIG: strcpy(errordesc, "EFBIG"); break;
    case EINTR: strcpy(errordesc, "EINTR"); break;
    case EISDIR: strcpy(errordesc, "EISDIR"); break;
    case ELOOP: strcpy(errordesc, "ELOOP"); break;
    case EMFILE: strcpy(errordesc, "EMFILE"); break;
    case ENAMETOOLONG: strcpy(errordesc, "ENAMETOOLONG"); break;
    case ENFILE: strcpy(errordesc, "ENFILE"); break;
    case ENODEV: strcpy(errordesc, "ENODEV"); break;
    case ENOENT: strcpy(errordesc, "ENOENT"); break;
    case ENOMEM: strcpy(errordesc, "ENOMEM"); break;
    case ENOSPC: strcpy(errordesc, "ENOSPC"); break;
    case ENOTDIR: strcpy(errordesc, "ENOTDIR"); break;
    case ENXIO: strcpy(errordesc, "ENXIO"); break;
    case EOVERFLOW: strcpy(errordesc, "EOVERFLOW"); break;
    case EPERM: strcpy(errordesc, "EPERM"); break;
    case EROFS: strcpy(errordesc, "EROFS"); break;
    case ETXTBSY: strcpy(errordesc, "ETXTBSY"); break;
    case EWOULDBLOCK: strcpy(errordesc, "EWOULDBLOCK"); break;
    default: strcpy(errordesc, "-UNKNOWN-"); break;  
  }

  sprintf(buff,"%s(%d)",errordesc, errno);
  strcpy(errordesc, buff);
  

}

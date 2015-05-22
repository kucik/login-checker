#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "ro_logger.h"
#include "ro_comm.h"

/************** NWN SIDE **********/

int  ro_sendToLC(ro_logind_p logdata) {

  static int fd = -1;
  int ret;

  /* Check opened pipe */
  if(fd < 0) {
    /* If Pipe does not exist */
    if(access(RO_PIPE_SRV_RCV, F_OK) == -1) {
      ret = mkfifo(RO_PIPE_SRV_RCV, 0777);
      if( ret != 0) {
        ro_log(RO_LOG_NAME_NWN,"Cannot create NWN output pipe %s. Retuned %d.",
              RO_PIPE_SRV_RCV, ret);
        return -1;
      }
    }
    /* open pipe */
    ro_log(RO_LOG_NAME_NWN,"Opening NWN output pipe %s.", RO_PIPE_SRV_RCV);
    fd = open(RO_PIPE_SRV_RCV, O_WRONLY | O_NONBLOCK);
    if(fd < 0 ) {
      ro_log(RO_LOG_NAME_NWN,"Cannot open NWN output pipe %s. returned %d", 
             RO_PIPE_SRV_RCV, fd);
      return -1;
    }
  }

  /* write data */
  ret = write(fd, logdata, sizeof(ro_logind_t));

  /* check written data */
  if(ret != sizeof(ro_logind_t)) {
    ro_log(RO_LOG_NAME_NWN,"Cannot send data to LC. Returned %d.", ret);
    return -1;
  }

  return 0;
}

int ro_readFromLC(ro_logind_p logdata) {
  static int fd = -1;
  int rcv = -1;
  int ret;
  int wait = 0;
  
  memset(logdata,'\0',sizeof(ro_logind_t));

  /* Check opened pipe */
  if(fd < 0) {
    /* If Pipe does not exist */
    if(access(RO_PIPE_SRV_SEND, F_OK) == -1) {
      ret = mkfifo(RO_PIPE_SRV_SEND, 0777);
      if( ret != 0) {
        ro_log(RO_LOG_NAME_NWN,"Cannot create NWN input pipe %s. Retuned %d.",
               RO_PIPE_SRV_SEND, ret);
        return -1;
      }
    }
    /* open pipe */
    ro_log(RO_LOG_NAME_NWN,"Opening NWN input pipe %s.", RO_PIPE_SRV_SEND);
    fd = open(RO_PIPE_SRV_SEND, O_RDONLY | O_NONBLOCK);
    if(fd < 0 ) {
      ro_log(RO_LOG_NAME_NWN,"Cannot open NWN input pipe %s. Retuned %d.",
               RO_PIPE_SRV_SEND, fd);
      return -1;
    }
  }

  while(rcv == -1 && wait < RO_MAX_WAIT) {
    rcv = read(fd, logdata, sizeof(ro_logind_t));
    
    if(rcv == -1) {
      wait++;
      usleep(50000);
    }
  }
  /* ret:
   *  0 - no process opened pipe 
   * -1 - pipe is opened, but empty
   * >0 - size of data
   */
  if(rcv != sizeof(ro_logind_t)) {
    return -1;
  } 

  return 0;
}

/***** Login checker side */

void ro_sendToNwn(ro_logind_p logdata) {

  static int fd = -1;
  int ret;

  if(fd < 0) {
    ro_log(RO_LOG_NAME_CHECKER,"Opening LC output pipe %s. \n", RO_PIPE_SRV_SEND);
    fd = open(RO_PIPE_SRV_SEND, O_WRONLY);
    if(fd < 0 ) {
      ro_log(RO_LOG_NAME_CHECKER,"Cannot open LC output pipe %s. returned %d", 
             RO_PIPE_SRV_SEND, fd); 
      return;
    }
  }

  ret = write(fd, logdata, sizeof(ro_logind_t));
  if(ret != sizeof(ro_logind_t)) {
    ro_log(RO_LOG_NAME_CHECKER,"Cannot  send data to NWN. returned %d",
             ret); 
  }

}

int ro_recvFromNwn(int fd, ro_logind_p logdata) {
  int ret;

  if(fd < 0) {
    return -1;
  }
  memset(logdata,'\0',sizeof(ro_logind_t));
  ret = read(fd, logdata, sizeof(ro_logind_t));
 
  return ret;

}


int ro_openSrvInPipe() {

  int fd;
  int ret;
  /* If Pipe does not exist */
  if(access(RO_PIPE_SRV_RCV, F_OK) == -1) {
    ret = mkfifo(RO_PIPE_SRV_RCV, 0777);
    if( ret != 0) {
      ro_log(RO_LOG_NAME_CHECKER,"Cannot create LC in  pipe %s. Retuned %d.",
              RO_PIPE_SRV_RCV, ret);
      /*exit(1);*/
      return -1;
    }
  }

  ro_log(RO_LOG_NAME_CHECKER,"Opening LC input pipe %s.", RO_PIPE_SRV_RCV);
  fd = open(RO_PIPE_SRV_RCV, O_RDONLY);
  if(fd < 0 ) {
    /*exit(1); */
    ro_log(RO_LOG_NAME_CHECKER,"Cannot open LC input pipe %s. returned %d", RO_PIPE_SRV_RCV, fd);
    return -1;
  }

  return fd;
}

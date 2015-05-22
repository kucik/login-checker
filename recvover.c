/***************************************************************************
    netoverride.c - Network function overrides for NWN
    Copyright (C) 2007 Doug Swarin (zac@intertex.net)
    Modified kucik 2010

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdarg.h>

#include <fcntl.h>
#include <time.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>

#define __USE_GNU
#include <dlfcn.h>

#include "ro_logger.h"
#include "ro_comm.h"

static char *ro_extract_pstring (unsigned char *pstring) {
  static char buf[256];

  /* pstring is guaranteed to be no more than 255 bytes */
  memcpy(buf, (pstring + 1), *pstring);
  buf[*pstring] = 0;

  return buf;
}


static int ro_verify_login(const char *login, const char *key, 
                           const char *key2, const char *key3,
                           const struct sockaddr_in *sin) { 

   static ro_logind_t send, recv;
 
   /* Check player login length */
   if(strlen(login) > RO_NWN_LOGIN_LEN) {
     ro_log(RO_LOG_NAME_NWN,"Player login '%s' (%d) exceeded limit %d.",
            login, (int)strlen(login), RO_NWN_LOGIN_LEN);
     return RO_LOGIN_ERROR_UNKNOWN;
   }

   ro_log(RO_LOG_NAME_NWN,"Checking player '%s' with keys '%s' '%s' '%s' from"
                           " %s",
                    login, key, key2, key3, inet_ntoa(sin->sin_addr));
  
   /* Prepare send struct */
   memset(send.login, '\0', sizeof(send.login));
   memset(send.key2, '\0', sizeof(send.key2));
   memset(send.key3, '\0', sizeof(send.key3));


   strcpy(send.login, login);
   strcpy(send.key, key);
   strcpy(send.ip, inet_ntoa(sin->sin_addr));
   send.permission = 0;
   
   /* key check */
   strcpy(send.key2, key2);
   strcpy(send.key3, key3);

   /* Send data */
   if(ro_sendToLC(&send) != 0) { 
     ro_log(RO_LOG_NAME_NWN,"Error. Cannot open write pipe to LC."
            "player login is disallowed.");
     return RO_COMMUNICATION_ERROR;
   }

   /* Wait for answer */
   while(ro_readFromLC(&recv) == 0) {
     if( strcmp(recv.login, send.login)==0 ) {
       /* Check permission */
       ro_log(RO_LOG_NAME_NWN,"Login %s for player %s (%s).",
              (recv.permission == RO_LOGIN_OK) ? "allowed":"disallowed",
              recv.login, recv.key);
       return recv.permission;
     }
     /* Bad answer */
     ro_log(RO_LOG_NAME_NWN,"Error. Expected data (%s|%s) but "
            "received (%s|%s)",
             send.login, send.key, recv.login, recv.key);
   }
   ro_log(RO_LOG_NAME_NWN,"Error. Cannot read data from LC");
   
   return RO_COMMUNICATION_ERROR;
}


ssize_t recvfrom (int fd, void *buf, size_t len, int flags,
                  struct sockaddr *addr, socklen_t *addrlen) {

  ssize_t ret;
  static ssize_t (*real_recvfrom)(int, void *, size_t, int,
                  struct sockaddr *, socklen_t *) = NULL;
  static char name[256], cdkey[256];
/*  static char tmp[256];
  int i;*/

  /* Get link to original recvfrom function */
  if (real_recvfrom == NULL)
    real_recvfrom = dlsym(RTLD_NEXT, "recvfrom");

  
  /* Use original recvfrom and get return value */
  ret = real_recvfrom(fd, buf, len, flags, addr, addrlen);

  if (ret > 18 && memcmp(buf, "BNVSV", 5) == 0) {
    struct sockaddr_in *sin = (struct sockaddr_in *)addr;
    char key1[8+1];
    char key2[8+1];
    char key3[8+1];
    int permission = 0;
    memset(key1,'\0',9);
    memset(key2,'\0',9);
    memset(key3,'\0',9);
     
    memcpy(key1, buf + 7 , 8);
    memcpy(key2, buf + 48, 8);
    memcpy(key3, buf + 89, 8);

    ro_log(RO_LOG_NAME_NWN,"Check keys: %s:%d (%s) (%s) (%s).",
           inet_ntoa(sin->sin_addr), ntohs(sin->sin_port), key1, key2, key3);
    permission = ro_verify_login("" ,key1, key2, key3, sin);
     
     
  } else if (ret > 18 && memcmp(buf, "BNCS", 4) == 0) {


    unsigned char *p = (unsigned char*)buf;
    int ver = *(p + 5);
    int cl_port = p[5] +( 256 * p [4]);
    char cl_ct = p[6];
    struct sockaddr_in *sin = (struct sockaddr_in *)addr;
    int permission = 0;

/*    if (ver < 20 || *(p + 18) < 1 || *(p + 19 + *(p + 18)) < 1) { */
    if (*(p + 18) < 1 || *(p + 19 + *(p + 18)) < 1) {
      ro_log(RO_LOG_NAME_NWN,"rejected invalid login type %xd from %s:%d [v%d] cl_port(%d)",
        cl_ct, inet_ntoa(sin->sin_addr), ntohs(sin->sin_port), ver, cl_port);

      memcpy(buf, "BNLM\x00\x12\x00\x00\x00\x00\x00", 11);
      return 11;
    }

    strcpy(name, ro_extract_pstring(p + 18));
    strcpy(cdkey, ro_extract_pstring(p + 19 + *(p + 18)));
    

    permission = ro_verify_login(name,cdkey,"","",sin); 
    switch(permission) {
      /* Login OK */
      case RO_LOGIN_OK:
        break;
      /* Server not ready */
      case RO_LOGIN_ERROR_SERVER_NOT_READY:
        ro_log(RO_LOG_NAME_NWN,"Server is not ready for login %s:%d by %s (%s) [v%d]",
        inet_ntoa(sin->sin_addr), ntohs(sin->sin_port), name, cdkey, ver);
        memcpy(buf, "BNLM\x00\x12\x00\x00\x00\x00\x00", 11);
        return 11;
      /* Bad login data */
      case RO_LOGIN_ERROR_NO_REG:
      case RO_LOGIN_ERROR_UNKNOWN:
      case RO_LOGIN_ERROR_CDKEY:
      case RO_LOGIN_ERROR_IP:
        ro_log(RO_LOG_NAME_NWN,"reject invalid login + cdkey combination %s:%d by %s (%s) [v%d]",
        inet_ntoa(sin->sin_addr), ntohs(sin->sin_port), name, cdkey, ver);
        memcpy(buf, "BNLM\x00\x12\x00\x00\x00\x00\x00", 11);
        return 11;
      /* Some other error */
      default:
        break;
    }

    ro_log(RO_LOG_NAME_NWN,"Acces granted type %xd %s:%d to %s (%s) [v%d] cl_port(%d)",
      cl_ct, inet_ntoa(sin->sin_addr), ntohs(sin->sin_port), name, cdkey, ver, cl_port);
  }

  return ret;
}


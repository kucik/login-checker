#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#include <mysql.h>
#include "ro_logger.h"
#include "ro_comm.h"

#define __USE_GNU

#define __LC_SERVER_STATUS_OK 1
#define __LC_SERVER_STATUS_NOT_RUNNING 0

static int __checkServerStatus() {
  static int status = __LC_SERVER_STATUS_NOT_RUNNING;
  int ret = 0;

  if(status == __LC_SERVER_STATUS_NOT_RUNNING) {
    ro_log(RO_LOG_NAME_CHECKER,"Checking nwserver status ");
    ret = system("grep \"INIT([SO])\" \"/home/nwn/logs.0/nwnx2.txt\" ");
    if(ret == 0) {
      status = __LC_SERVER_STATUS_OK;
      ro_log(RO_LOG_NAME_CHECKER,"Server is ready for incoming connections ");
    } 
    else {
      ro_log(RO_LOG_NAME_CHECKER,"Server is not ready. ");
    }
  }
  return status;
}

static MYSQL* __mysql_init(const char *server, const char *user, 
                                      const char *password, const char *db) {
  MYSQL *conn;

  conn = mysql_init(NULL);

  /* Connect to database */
  if (!mysql_real_connect(conn, server,
      user, password, db, 0, NULL, 0)) {
     ro_log(RO_LOG_NAME_CHECKER,"Error when connecting to database: server(%s) user(%s) "
            "password (*****) database(%s)",
        server,user,db);
     conn=NULL;
     return conn;
  }

  ro_log(RO_LOG_NAME_CHECKER,"Connected to database: server(%s) user(%s) "
            "password (*****) database(%s)",
             server,user,db);

  return conn;
}

static int __checkKeys(ro_logind_p logdata, MYSQL *conn) {
  char sql[1024];
  MYSQL_RES *res;
  MYSQL_ROW row;
  int iret = 0;
  /* Check if DB is initialized */
  if(conn == NULL) {
    return RO_LOGIN_MYSQL_ERROR;
  }

  /* Compose SQL request */
  memset(sql,'\0',1024);
  sprintf(sql, "SELECT count(*) FROM pwplayers "
               "WHERE cdkey='%s' AND cdkey2='%s' AND CDKEY3='%s' AND"
               "(IP = '%s' OR noipcheck = '1')",
               logdata->key, logdata->key2, logdata->key3, logdata->ip);
  /* Process SQL request */
  if (mysql_query(conn, sql)) {
     ro_log(RO_LOG_NAME_CHECKER,"Error when reading from database: '%s'. "
            "Got error: %s ", sql,mysql_error(conn));
     return RO_LOGIN_MYSQL_ERROR;
  }

  /* Check result */
  res = mysql_use_result(conn);
  if( (row = mysql_fetch_row(res)) == NULL) {
    ro_log(RO_LOG_NAME_CHECKER,"Error when reading from database: '%s'. "
            "Got error: %s ", sql,mysql_error(conn));
    mysql_free_result(res);
    return RO_LOGIN_MYSQL_ERROR;
  }

  /* Count of records */
  if(atoi(row[0]) > 0) {
    logdata->errorcode = RO_LOGIN_OK;
    iret = RO_LOGIN_OK;
  }
  else {
    logdata->errorcode = RO_LOGIN_ERROR_CDKEY;
    ro_log(RO_LOG_NAME_CHECKER,"Invalid CDKEY combination '%s' '%s' '%s' "
           "from IP '%s'.", logdata->key, logdata->key2, logdata->key3, 
           logdata->ip);
    iret = RO_LOGIN_ERROR_CDKEY;
  }
 
  mysql_free_result(res);
  return iret;
}

static int __checkLogin(ro_logind_p logdata, MYSQL *conn) {

  char sql[1024];
  MYSQL_RES *res;
  MYSQL_ROW row; 
  /* Default state */
  int iret = RO_LOGIN_ERROR_NO_REG;

  /* Check if DB is initialized */
  if(conn == NULL) {
    return RO_LOGIN_MYSQL_ERROR;
  }
  
  /* Compose SQL request */
  memset(sql,'\0',1024);
  sprintf(sql, "SELECT login, cdkey, ip, noipcheck, privilegies FROM pwplayers WHERE login='%s'",
                 logdata->login);
  /* Process SQL request */
  if (mysql_query(conn, sql)) {
     ro_log(RO_LOG_NAME_CHECKER,"Error when reading from database: '%s'. "
            "Got error: %s ", sql,mysql_error(conn));
     return RO_LOGIN_MYSQL_ERROR;
  }

  /* Check result */
  res = mysql_use_result(conn);
  logdata->errorcode = RO_LOGIN_ERROR_NO_REG;

  /* Check result */ 
  while ((row = mysql_fetch_row(res)) != NULL) {
      logdata->errorcode = RO_LOGIN_OK;
      iret = RO_LOGIN_OK;
      /* Impossible to get here */
      if( strcmp(row[0], logdata->login) != 0) {
        logdata->errorcode = RO_LOGIN_ERROR_UNKNOWN;
        ro_log(RO_LOG_NAME_CHECKER,"Unknown user '%s'.",logdata->login);
        iret = RO_LOGIN_ERROR_NO_REG;
        break;
      }
      /* Invalid CD KEY */
      if( strcmp(row[1], logdata->key) != 0) {
        logdata->errorcode = RO_LOGIN_ERROR_CDKEY;
        ro_log(RO_LOG_NAME_CHECKER,"Invalid CDKEY '%s' for user '%s'. "
               "Should be '%s'", logdata->key, logdata->login, row[1]);
        strncpy(logdata->key, row[1], RO_NWN_CDKEY_LEN);
        iret = RO_LOGIN_ERROR_CDKEY;
        break;
      }
      /* IP adress differ */
      if( atoi(row[3]) == 0 && strcmp(row[2], logdata->ip) != 0) {
        logdata->errorcode = RO_LOGIN_ERROR_IP;
        ro_log(RO_LOG_NAME_CHECKER,"Invalid IP '%s' for user '%s'. Should be '%s'", logdata->ip, logdata->login, row[2]);
        strncpy(logdata->ip, row[2], RO_IP_LEN);
        iret = RO_LOGIN_ERROR_IP;
        break;
      }
      /* Check privilegies */
      if( atoi(row[4]) < 0 ) {
        logdata->errorcode = RO_LOGIN_ERROR_IP;
        ro_log(RO_LOG_NAME_CHECKER,"Not valid privilegies '%d' for user '%s'. ",  atoi(row[4]), logdata->login );
        strncpy(logdata->ip, row[2], RO_IP_LEN);
        iret = RO_LOGIN_ERROR_IP;
        break;
      }
  }
  mysql_free_result(res);

  /* No rows found */
  if(iret == RO_LOGIN_ERROR_NO_REG) {
    logdata->errorcode = RO_LOGIN_ERROR_UNKNOWN;
    ro_log(RO_LOG_NAME_CHECKER,"Unknown user '%s'.",logdata->login);
    iret = RO_LOGIN_ERROR_NO_REG;
  }

  return iret;

}


int main(int argc, char **argv) {
  int fd_in;
  MYSQL *conn;
  int run = 1;
  int ret;
  ro_logind_t dtin;
  int permission;
  char host[16];
  char user[32]; 
  char passwd[32];
  char dbname[32];
  int i;

  /* read input params */
  for(i=1; i < argc; i++) {
    switch(i) {
      case 1:
        strncpy(host, argv[i], sizeof(host));
        break;
      case 2:
        strncpy(user, argv[i], sizeof(user));
        break;
      case 3:
        strncpy(passwd, argv[i], sizeof(passwd));
        break;
      case 4:
        strncpy(dbname, argv[i], sizeof(dbname));
        break;
    }
  }
  conn = __mysql_init(host, user, passwd, dbname);

  /* Open pipe */  
  fd_in = -1;
  while(fd_in == -1) {
    sleep(1);
    fd_in = ro_openSrvInPipe();
  }

  /* main loop */
  while(run != 0) {
    /* Get incoming data */    

    ret = ro_recvFromNwn(fd_in, &dtin);

    if(__checkServerStatus() != __LC_SERVER_STATUS_OK) {
      permission = RO_LOGIN_ERROR_SERVER_NOT_READY;
    }
    else if(strlen(dtin.login) > 0){
      permission = __checkLogin(&dtin, conn);
    }
    else {
      permission = __checkKeys(&dtin, conn);
    }
    dtin.permission = permission;

    ro_sendToNwn(&dtin);
  }
  
  return 0;
}

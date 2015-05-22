

#define RO_PIPE_SRV_RCV "/tmp/nwn_lc_rcv"
#define RO_PIPE_SRV_SEND "/tmp/nwn_lc_send"
#define RO_MAX_WAIT 6
#define RO_NWN_LOGIN_LEN 50
#define RO_NWN_CDKEY_LEN 10
#define RO_IP_LEN 16

#define RO_LOGIN_OK 0x0
#define RO_COMMUNICATION_ERROR 0x1
#define RO_LOGIN_MYSQL_ERROR 0x2
#define RO_LOGIN_ERROR_SERVER_NOT_READY 0x4
#define RO_LOGIN_ERROR_NO_REG 0x8
#define RO_LOGIN_ERROR_UNKNOWN 0x10
#define RO_LOGIN_ERROR_CDKEY 0x20
#define RO_LOGIN_ERROR_IP 0x40


typedef struct{
  char login[RO_NWN_LOGIN_LEN + 1];
  char key[RO_NWN_CDKEY_LEN + 1 ];
  char key2[RO_NWN_CDKEY_LEN + 1 ];
  char key3[RO_NWN_CDKEY_LEN + 1 ];
  char ip[RO_IP_LEN + 1];
  int permission;
  int errorcode;
} ro_logind_s;
typedef ro_logind_s  ro_logind_t;
typedef ro_logind_s *ro_logind_p;

int  ro_sendToLC(ro_logind_p logdata);

int ro_readFromLC(ro_logind_p logdata);

void ro_sendToNwn(ro_logind_p logdata);

int ro_recvFromNwn(int fd, ro_logind_p logdata);

int ro_openSrvInPipe();





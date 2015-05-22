
#define RO_LOG_NAME_NWN "NWN"
#define RO_LOG_NAME_CHECKER "LOGCHECKER"

/***************** Function declaration ******************/
/**
 * Log text
 * 
 * @param[in]   srvclnt - Server/client identification
 * @param[in]   fmt     - Iput text format
 * @param[in]   ...     - Params
 */
void ro_log (const char *srvclnt,const char *fmt, ...)  __attribute__(( format (printf, 2, 3) ));



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <lua5.2/lua.h>
#include <lua5.2/lualib.h>
#include <lua5.2/lauxlib.h>

#if LIBCURL_VERSION_NUM >= 0x073d00
/* In libcurl 7.61.0, support was added for extracting the time in plain
   microseconds. Older libcurl versions are stuck in using 'double' for this
   information so we complicate this example a bit by supporting either
   approach. */ 
#define TIME_IN_US 1  
#define TIMETYPE curl_off_t
#define TIMEOPT CURLINFO_TOTAL_TIME_T
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     1000000
#else
#define TIMETYPE double
#define TIMEOPT CURLINFO_TOTAL_TIME
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     0.5
#endif
 
#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         600000000000

static int curl(lua_State *L);
static int get_body(lua_State *L);
static int xferinfo(void *p,curl_off_t dltotal, curl_off_t dlnow,curl_off_t ultotal, curl_off_t ulnow);
static int older_progress(void *p,double dltotal, double dlnow,double ultotal, double ulnow);
int luaopen_libspeedtest(lua_State *L);
int testSpeed(const char *link, lua_State *L, int upload);
const char* getBody(const char *link);
void createfile(int size);
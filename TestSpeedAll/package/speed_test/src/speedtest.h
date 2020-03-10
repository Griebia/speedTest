#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <math.h>
#include <time.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

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
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     0.1
#endif
 
#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         600000000000

#ifndef CURL_OFF_T_MAX
#if (CURL_SIZEOF_CURL_OFF_T == 4)
#define CURL_OFF_T_MAX CURL_OFF_T_C(0x7FFFFFFF)
#else /* assume CURL_SIZEOF_CURL_OFF_T == 8 */
#define CURL_OFF_T_MAX CURL_OFF_T_C(0x7FFFFFFFFFFFFFFF)
#endif
#define CURL_OFF_T_MIN (-CURL_OFF_T_MAX - CURL_OFF_T_C(1))
#endif

struct string {
  char *ptr;
  size_t len;
};

struct speedtestprogress {
  TIMETYPE lastruntime; /* type depends on version, see above */
  TIMETYPE testtime; 
  CURL *curl;
  lua_State *L;
};

static int testspeed_wrapper(lua_State *L);
static int getbody_wrapper(lua_State *L);
static int call_back(void *p,curl_off_t dltotal, curl_off_t dlnow,curl_off_t ultotal, curl_off_t ulnow);
int luaopen_libspeedtest(lua_State *L);
int test_internet_speed(const char *link, double testTime, int upload, lua_State *L);
const char* get_body(const char *link);
void init_string(struct string *s);
size_t write_string(void *ptr, size_t size, size_t nmemb, struct string *s);
size_t write_empty(void *buffer, size_t size, size_t nmemb, void *userp);
void handle_sigint(int sig);
void send_signals();
void create_pid();
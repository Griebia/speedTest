#include <stdio.h>
#include <curl/curl.h>
#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>
 


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
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     1
#endif
 
#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         600000000000
 
struct myprogress {
  TIMETYPE lastruntime; /* type depends on version, see above */ 
  CURL *curl;
  lua_State *L;
};
 
/* this is how the CURLOPT_XFERINFOFUNCTION callback works */ 
static int xferinfo(void *p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
{
  struct myprogress *myp = (struct myprogress *)p;
  CURL *curl = myp->curl;
  TIMETYPE curtime = 0;
  lua_State *L = myp->L;
  curl_off_t avarageSpeed;
  curl_off_t currentDownloaded = dlnow;
  curl_easy_getinfo(curl, TIMEOPT, &curtime);
  curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD_T, &avarageSpeed);
  
  /* under certain circumstances it may be desirable for certain functionality
     to only run every N seconds, in order to do this the transaction time can
     be used */ 
   
  if((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) {
    myp->lastruntime = curtime;
#ifdef TIME_IN_US
    
    fprintf(stderr, "TOTAL TIME: %" CURL_FORMAT_CURL_OFF_T ".%06ld\r\n",
            (curtime / 1000000), (long)(curtime % 1000000));
#else

  double x = (double) avarageSpeed;
  double y = (double) currentDownloaded;
  lua_getglobal(L, "printSpeeds");  /* function to be called */
  lua_pushnumber(L, x);   /* push 1st argument */
  lua_pushnumber(L, y);   /* push 2nd argument */
  
  /* do the call (2 arguments, 1 result) */
      if (lua_pcall(L, 2, 1, 0) != 0)
       //perror("error running function `f': %s",
         //        lua_tostring(L, -1));
    
      /* retrieve result */
      if (!lua_isnumber(L, -1))
        perror("function `f' must return a number");
      double z = lua_tonumber(L, -1);
      lua_pop(L, 1);  /* pop returned value */

    // printf("Avarage speed %" CURL_FORMAT_CURL_OFF_T ".%06ld\r\n", (avarageSpeed/1000000),(long)(avarageSpeed % 1000000));
    // printf("Current downloaded speed %" CURL_FORMAT_CURL_OFF_T ".%06ld\r\n", (currentDownloaded/1000000),(long)(currentDownloaded % 1000000));
    // fprintf(stderr, "TOTAL TIME: %f \r\n", curtime);
#endif
  }
 
//   fprintf(stderr, "UP: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
//           "  DOWN: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
//           "\r\n",
//           ulnow, ultotal, dlnow, dltotal);
 
  if(dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
    return 1;
  return 0;
}
 
#if LIBCURL_VERSION_NUM < 0x072000
/* for libcurl older than 7.32.0 (CURLOPT_PROGRESSFUNCTION) */ 
static int older_progress(void *p,
                          double dltotal, double dlnow,
                          double ultotal, double ulnow)
{
  return xferinfo(p,
                  (curl_off_t)dltotal,
                  (curl_off_t)dlnow,
                  (curl_off_t)ultotal,
                  (curl_off_t)ulnow);
}
#endif
 
int testSpeed(const char *link, lua_State *L)
{
  CURL *curl;
  CURLcode res = CURLE_OK;
  struct myprogress prog;
 
  curl = curl_easy_init();
  if(curl) {
    prog.lastruntime = 0;
    prog.curl = curl;
    prog.L = L;
 
    curl_easy_setopt(curl, CURLOPT_URL, link);
 
#if LIBCURL_VERSION_NUM >= 0x072000
    /* xferinfo was introduced in 7.32.0, no earlier libcurl versions will
       compile as they won't have the symbols around.
 
       If built with a newer libcurl, but running with an older libcurl:
       curl_easy_setopt() will fail in run-time trying to set the new
       callback, making the older callback get used.
 
       New libcurls will prefer the new callback and instead use that one even
       if both callbacks are set. */ 
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
    /* pass the struct pointer into the xferinfo function, note that this is
       an alias to CURLOPT_PROGRESSDATA */ 
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
#else
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, older_progress);
    /* pass the struct pointer into the progress function */ 
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &prog);
#endif
 
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    res = curl_easy_perform(curl);
 
    if(res != CURLE_OK)
      fprintf(stderr, "%s\n", curl_easy_strerror(res));
 
    /* always cleanup */ 
    curl_easy_cleanup(curl);
  }
  return (int)res;
}
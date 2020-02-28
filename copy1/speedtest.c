#include "speedtest.h"


struct string {
  char *ptr;
  size_t len;
};

//library to be registered
static const struct luaL_Reg mylib [] = {
      {"curl", curl},
      {"get_body", get_body},
      {NULL, NULL}  /* sentinel */
    };

static int curl(lua_State *L){
    const char *link = lua_tostring(L, 1);
    int upload = lua_toboolean(L, 2);
    testSpeed(link,L,upload);
    lua_pushstring(L,link);
    return 3;
}

static int get_body(lua_State *L){
  const char *link = lua_tostring(L, 1);
  const char *body = getBody(link);
  printf("%s\n", body);
  lua_pushstring(L,body);
  return 1;
}

//name of this function is not flexible
int luaopen_libspeedtest(lua_State *L){
	luaL_newlib(L, mylib);
	return 1;
}

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}

const char* getBody(const char *link){
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if(curl) {
    struct string s;
    init_string(&s);

    curl_easy_setopt(curl, CURLOPT_URL, link);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
  
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    
    res = curl_easy_perform(curl);

    curl_easy_cleanup(curl); 

    printf("%s\n", s.ptr);

    return s.ptr;
  }

  curl_easy_cleanup(curl); 
}
 
struct myprogress {
  TIMETYPE lastruntime; /* type depends on version, see above */ 
  CURL *curl;
  lua_State *L;
};
 
/* this is how the CURLOPT_XFERINFOFUNCTION callback works */ 
static int xferinfo(void *p,curl_off_t dltotal, curl_off_t dlnow,curl_off_t ultotal, curl_off_t ulnow)
{
  struct myprogress *myp = (struct myprogress *)p;
  CURL *curl = myp->curl;
  TIMETYPE curtime = 0;
  lua_State *L = myp->L;
  curl_off_t downloadSpeed;
  curl_off_t uploadSpeed;
  curl_off_t currentDownloaded = dlnow;
  curl_easy_getinfo(curl, TIMEOPT, &curtime);
  curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD_T, &downloadSpeed);
  curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD_T, &uploadSpeed);

  //printf("%f %f\n",(double)downloadSpeed,(double)uploadSpeed);
  
  /* under certain circumstances it may be desirable for certain functionality
     to only run every N seconds, in order to do this the transaction time can
     be used */ 
   
  if((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) {
    myp->lastruntime = curtime;
#ifdef TIME_IN_US
    
    fprintf(stderr, "TOTAL TIME: %" CURL_FORMAT_CURL_OFF_T ".%06ld\r\n",
            (curtime / 1000000), (long)(curtime % 1000000));
#else

  lua_getglobal(L, "printSpeeds");  /* function to be called */
  lua_pushnumber(L, (double)downloadSpeed);   /* push 1st argument */
  lua_pushnumber(L, (double)currentDownloaded);   /* push 2nd argument */
  lua_pushnumber(L, (double)ultotal);   /* push 1st argument */
  lua_pushnumber(L, (double)uploadSpeed);   /* push 2nd argument */
  
  /* do the call (4 arguments, 1 result) */
  if (!lua_pcall(L, 4, 1, 0))
    perror("error running function `f': %s");
    
#endif
  }
 
  if(dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
    return 1;
  return 0;
}
 
#if LIBCURL_VERSION_NUM < 0x072000
/* for libcurl older than 7.32.0 (CURLOPT_PROGRESSFUNCTION) */ 
static int older_progress(void *p,double dltotal, double dlnow,double ultotal, double ulnow){
  return xferinfo(p,(curl_off_t)dltotal,(curl_off_t)dlnow,(curl_off_t)ultotal,(curl_off_t)ulnow);
}
#endif
 
void createfile(int size){
  FILE *fp = fopen("testFile", "w");
  fseek(fp, size, SEEK_SET);
  fputc('\0', fp);
  fclose(fp);
}

int testSpeed(const char *link, lua_State *L, int upload){
  CURL *curl;
  CURLcode res = CURLE_OK;
  struct myprogress prog;
  struct stat file_info;

  int hd;

  
 
  curl = curl_easy_init();
  if(curl) {
    prog.lastruntime = 0;
    prog.curl = curl;
    prog.L = L;
    FILE *fp;
    curl_easy_setopt(curl, CURLOPT_URL, link);


 
#if LIBCURL_VERSION_NUM >= 0x072000
    /* xferinfo was introduced in 7.32.0, no earlier libcurl versions will
       compile as they won't have the symbols around.
 
       If built with a newer libcurl, but running with an older libcurl:
       curl_easy_setopt() will fail in srun-time trying to set the new
       callback, making the older callback get used.
 
       New libcurls will prefer the new callback and instead use that one even
       if both callbacks are set. */ 
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
    /* pass the struct pointer into the xferinfo function, note that this is
       an alias to CURLOPT_PROGRESSDATA */ 
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
    if(upload == 1){
      fp = fopen("10MB.zip","rb");
      hd = open("10MB.zip", O_RDONLY);
      fstat(hd, &file_info);
       /* tell it to "upload" to the URL */
      curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

      /* set where to read from (on Windows you need to use READFUNCTION too) */
      curl_easy_setopt(curl, CURLOPT_READDATA, fp);

          /* and give the size of the upload, this supports large file sizes
       on systems that have general support for it */ 
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                     (curl_off_t)file_info.st_size);
    }
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

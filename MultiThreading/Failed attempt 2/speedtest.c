#include "speedtest.h"

lua_State *stateL;
int p[2];

//Libary of commands for lua to recognise.
static const struct luaL_Reg mylib[] = {
    {"testspeed", testspeed_wrapper},
    {"getbody", getbody_wrapper},
    {NULL, NULL} /* sentinel */
};

//Curl command wrapper for testing the speed of upload or download.
static int testspeed_wrapper(lua_State *L)
{
  printf("Gone in the wrapper \n");
  const char *url = lua_tostring(L, 1);
  int timetest = lua_tonumber(L, 2);
  int upload = lua_toboolean(L, 3);
  int res = test_internet_speed(url, timetest, upload, L);
  if (res > 0)
  {
    res = 0;
  }
  lua_pushboolean(L, res);
  lua_pushstring(L, url);
  return 2;
}

//Body get wrapper from a url.
static int getbody_wrapper(lua_State *L)
{
  const char *url = lua_tostring(L, 1);
  const char *body = get_body(url);
  lua_pushstring(L, body);
  return 1;
}

//Setting the libary to lua to recognise (The naming scheme should be the same as the made .so files).
int luaopen_libspeedtest(lua_State *L)
{
  stateL = L;
  // signal(SIGINT, handle_sigint);
  // send_signals();
  // create_pid();
  luaL_newlib(L, mylib);
  return 1;
}

//Initialising a string.
void init_string(struct string *s)
{
  s->len = 0;
  s->ptr = malloc(s->len + 1);
  if (s->ptr == NULL)
  {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

//Writes to string struct the gotten data.
size_t write_string(void *ptr, size_t size, size_t nmemb, struct string *s)
{
  size_t new_len = s->len + size * nmemb;
  s->ptr = realloc(s->ptr, new_len + 1);
  if (s->ptr == NULL)
  {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr + s->len, ptr, size * nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size * nmemb;
}

//Empty write for download speed test.
size_t write_empty(void *buffer, size_t size, size_t nmemb, void *userp)
{
  return size * nmemb;
}

//Get the body response form an URL
const char *get_body(const char *URL)
{
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  if (curl)
  {
    struct string s;
    init_string(&s);
    curl_easy_setopt(curl, CURLOPT_URL, URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return s.ptr;
  }
  curl_easy_cleanup(curl);
}

static int wait_for_results()
{
  int inbuf;
  printf("Waiting for results\n");
  int copy = dup(p[0]);
  read(copy, inbuf, sizeof(int));
  printf("%d\n", inbuf);

  //   Stops the function if the set time limit is reached.
  // if (curtime >= myp->testtime)
  //   return 1;
  // //Stops the function if a certain byte limit is reached.
  // if (dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
  //   return 1;
  return 0;
}

/* this is how the CURLOPT_XFERINFOFUNCTION  works */
//This is the  function for every call in download or upload
static int call_back(void *pa, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
  //Gets all of the information from the struct
  struct speedtestprogress *myp = (struct speedtestprogress *)pa;
  CURL *curl = myp->curl;
  TIMETYPE curtime = 0;
  lua_State *L = myp->L;
  curl_easy_getinfo(curl, TIMEOPT, &curtime);
  if ((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL)
  {
    printf("%d\n",getpid());
    int copy = dup(p[1]);
    write(copy, getpid(), sizeof(int));
    close(copy);
    // myp->lastruntime = curtime;
    // curl_off_t downloadSpeed;
    // curl_off_t uploadSpeed;
    // curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD_T, &downloadSpeed);
    // curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD_T, &uploadSpeed);
    // //The function in lua that should be called.
    // lua_getglobal(L, "writeData");
    // //Return the arguments to the function.
    // lua_pushnumber(L, (double)downloadSpeed); /* push 1st argument */
    // lua_pushnumber(L, (double)dlnow);         /* push 2nd argument */
    // lua_pushnumber(L, (double)uploadSpeed);   /* push 3st argument */
    // lua_pushnumber(L, (double)ulnow);         /* push 4nd argument */

    // /* do the call (4 arguments, 1 result) */
    // if (lua_pcall(L, 4, 1, 0))
    //   perror("error running function `f': %s");
  }

  //Stops the function if the set time limit is reached.
  if (curtime >= myp->testtime)
    return 1;
  //Stops the function if a certain byte limit is reached.
  if (dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
    return 1;
  return 0;
}

//Tests the internet speed by the given link adn if it is a download or an upload.
int test_internet_speed(const char *link, double testTime, int upload, lua_State *L)
{
  if (pipe(p) < 0)
  {
    exit(0);
  }

  for (int i = 0; i < 5; i++) // loop will run n times (n=5)
  {
    int i = fork();
    if (i == 0)
    {
      CURL *curl = NULL;
      CURLcode res = CURLE_FAILED_INIT;
      FILE *fp = NULL;
      struct curl_slist *header_list = NULL;
      struct speedtestprogress prog;
      //Initiating the curl.
      curl = curl_easy_init();
      if (curl)
      {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        //Sets the xferinfo callback.
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, call_back);
        /* pass the struct pointer into the xferinfo function, note that this is
       an alias to CURLOPT_PROGRESSDATA */
        //Sets the given data to the function.
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
        //If it is an upload test does also this.
        if (upload == 1)
        {
          fp = fopen("/dev/zero", "r");
          if (!fp)
            goto cleanup;
          double sz = 92233720368547758;
          //Adds POST to the curl.
          curl_easy_setopt(curl, CURLOPT_POST, 1L);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE,
                           (curl_off_t)sz);
          //Selects the file form it is read.
          curl_easy_setopt(curl, CURLOPT_READDATA, fp);

          //Set the header
          header_list = curl_slist_append(header_list, "Expect:");
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
          curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
          /* Include server headers in the output */
          curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_empty);
        prog.lastruntime = 0;
        prog.curl = curl;
        prog.L = L;
        prog.testtime = testTime;

        curl_easy_setopt(curl, CURLOPT_URL, link);

        res = curl_easy_perform(curl);

        TIMETYPE curtime = 0;
        curl_easy_getinfo(curl, TIMEOPT, &curtime);

        curl_easy_cleanup(curl);

        if (curtime < prog.testtime)
        {
          test_internet_speed(link, prog.testtime - curtime, upload, L);
        }
      }

    cleanup:
      if (fp != NULL)
      {
        fclose(fp);
        fp = NULL;
      }
      printf("[son] pid %d from [parent] pid %d\n", getpid(), getppid());
      exit(0);
    }
  }
  while(wait_for_results() != 1)
    printf("Printed \n");

  for (int i = 0; i < 5; i++) // loop will run n times (n=5)
    wait(NULL);

  // printf("Starting to wait for results");
  // wait_for_results();
  return (int)1;
}

void handle_sigint(int sig)
{
  lua_getglobal(stateL, "close");
  /* do the call (4 arguments, 1 result) */
  lua_pcall(stateL, 0, 0, 0);

  exit(1);
}

void send_signals()
{
  FILE *fp;
  if ((fp = fopen("/var/run/speedtest.pid", "r")))
  {
    char str[60];
    fgets(str, 60, fp);
    int num = atoi(str);
    kill(num, SIGINT);
    fclose(fp);
  }
}

void create_pid()
{
  FILE *fp;
  fp = fopen("/var/run/speedtest.pid", "w");
  fprintf(fp, "%d", getpid());
  fclose(fp);
}

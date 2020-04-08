/* curl_multi_test.c

   Clemens Gruber, 2013
   <clemens.gruber@pqgruber.com>

   Code description:
    Requests 4 Web pages via the CURL multi interface
    and checks if the HTTP status code is 200.

   Update: Fixed! The check for !numfds was the problem.
*/

#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <curl/multi.h>

#define MAX_WAIT_MSECS 30*1000 /* Wait max. 30 seconds */
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
};


static const char *urls[] = {
  "http://www.microsoft.com",
  "http://www.yahoo.com",
  "http://www.wikipedia.org",
  "http://slashdot.org"
};
#define CNT 4

static size_t cb(char *d, size_t n, size_t l, void *p)
{
  /* take care of the data here, ignored in this example */
  (void)d;
  (void)p;
  return n*l;
}

//Empty write for download speed test.
size_t write_empty(void *buffer, size_t size, size_t nmemb, void *userp)
{
   return size * nmemb;
}

/* this is how the CURLOPT_XFERINFOFUNCTION  works */ 
//This is the  function for every call in download or upload
static int call_back(void *p,curl_off_t dltotal, curl_off_t dlnow,curl_off_t ultotal, curl_off_t ulnow)
{
  //Gets all of the information from the struct
  struct speedtestprogress *myp = (struct speedtestprogress *)p;
  CURL *curl = myp->curl;
  TIMETYPE curtime = 0;
  curl_easy_getinfo(curl, TIMEOPT, &curtime);
  if((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) {
    myp->lastruntime = curtime;
    curl_off_t downloadSpeed;
    curl_off_t uploadSpeed;
    curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD_T, &downloadSpeed);
    curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD_T, &uploadSpeed);
    //The function in lua that should be called.
    printf("%ld",uploadSpeed);
  }
  
  //Stops the function if the set time limit is reached.
  if(curtime >= myp->testtime)
    return 1;
  //Stops the function if a certain byte limit is reached.
  if(dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
    return 1;
  return 0;
}

static void init(CURLM *cm, const char *link, double testTime, int upload)
{
  CURL *curl = NULL;
  CURLcode res = CURLE_FAILED_INIT;
  FILE *fp = NULL;
  struct curl_slist *header_list = NULL;
  struct speedtestprogress prog;
  //Initiating the curl.
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    //Sets the xferinfo callback.
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, call_back);
    /* pass the struct pointer into the xferinfo function, note that this is
       an alias to CURLOPT_PROGRESSDATA */
    //Sets the given data to the function.
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
    //If it is an upload test does also this.
    if(upload == 1){
      fp = fopen("/dev/zero","r");
      // if(!fp)
      //   goto cleanup;
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
    prog.testtime = testTime;

    curl_easy_setopt(curl, CURLOPT_URL, link);
  }
  curl_multi_add_handle(cm, curl);
}

// static void init(CURLM *cm, int i)
// {
//   CURL *eh = curl_easy_init();
//   curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, cb);
//   curl_easy_setopt(eh, CURLOPT_HEADER, 0L);
//   curl_easy_setopt(eh, CURLOPT_URL, urls[i]);
//   curl_easy_setopt(eh, CURLOPT_PRIVATE, urls[i]);
//   curl_easy_setopt(eh, CURLOPT_VERBOSE, 0L);
//   curl_multi_add_handle(cm, eh);
// }

int main(void)
{
  printf("No error");
    CURLM *cm=NULL;
    CURL *eh=NULL;
    CURLMsg *msg=NULL;
    CURLcode return_code=0;
    int still_running=0, i=0, msgs_left=0;
    int http_status_code;
    const char *szUrl;

    curl_global_init(CURL_GLOBAL_ALL);

    cm = curl_multi_init();

    for (i = 0; i < CNT; ++i) {
        init(cm, "http://speed-kaunas.telia.lt:8080/speedtest/upload.php",10,1);
    }

    curl_multi_perform(cm, &still_running);

    do {
        int numfds=0;
        int res = curl_multi_wait(cm, NULL, 0, MAX_WAIT_MSECS, &numfds);
        if(res != CURLM_OK) {
            fprintf(stderr, "error: curl_multi_wait() returned %d\n", res);
            return EXIT_FAILURE;
        }
        /*
         if(!numfds) {
            fprintf(stderr, "error: curl_multi_wait() numfds=%d\n", numfds);
            return EXIT_FAILURE;
         }
        */
        curl_multi_perform(cm, &still_running);

    } while(still_running);

    while ((msg = curl_multi_info_read(cm, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            eh = msg->easy_handle;

            return_code = msg->data.result;
            if(return_code!=CURLE_OK) {
                fprintf(stderr, "CURL error code: %d\n", msg->data.result);
                continue;
            }

            // Get HTTP status code
            http_status_code=0;
            szUrl = NULL;

            curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_status_code);
            curl_easy_getinfo(eh, CURLINFO_PRIVATE, &szUrl);

            if(http_status_code==200) {
                printf("200 OK for %s\n", szUrl);
            } else {
                fprintf(stderr, "GET of %s returned http status code %d\n", szUrl, http_status_code);
            }

            curl_multi_remove_handle(cm, eh);
            curl_easy_cleanup(eh);
        }
        else {
            fprintf(stderr, "error: after curl_multi_info_read(), CURLMsg=%d\n", msg->msg);
        }
    }

    curl_multi_cleanup(cm);

    return EXIT_SUCCESS;
}

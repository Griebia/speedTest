/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2019, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
/* <DESC>
 * Upload to a file:// URL
 * </DESC>
 */

 
#include <stdio.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>


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
};

static int xferinfo(void *p,curl_off_t dltotal, curl_off_t dlnow,curl_off_t ultotal, curl_off_t ulnow)
{

  struct myprogress *myp = (struct myprogress *)p;
  CURL *curl = myp->curl;
  TIMETYPE curtime = 0;
  curl_off_t avarageSpeed;
  curl_off_t currentDownloaded = dlnow;
  curl_easy_getinfo(curl, TIMEOPT, &curtime);
  curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD_T, &avarageSpeed);
  //printf("%f\n",myp->lastruntime);
  /* under certain circumstances it may be desirable for certain functionality
     to only run every N seconds, in order to do this the transaction time can
     be used */ 
   
  if((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) {
    myp->lastruntime = curtime;
#ifdef TIME_IN_US
    printf("%f\n",curtime - myp->lastruntime);
    fprintf(stderr, "TOTAL TIME: %" CURL_FORMAT_CURL_OFF_T ".%06ld\r\n",
            (curtime / 1000000), (long)(curtime % 1000000));
#else
  double x = (double) avarageSpeed;
  double y = (double) currentDownloaded;
  printf("%ld\n",avarageSpeed);
    
#endif
  }
 
  if(dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
    return 1;
  return 0;
}



int main(void)
{

  int X = 100 * 1024 * 1024 - 1;
  FILE *fp = fopen("newfile", "w");
  fseek(fp, X , SEEK_SET);
  fputc('\0', fp);
  fclose(fp);

  CURL *curl;
  CURLcode res;
  struct stat file_info;
  curl_off_t speed_upload, total_time;

  fp = fopen("newfile", "rb"); /* open file to upload */
  if(!fp)
    return 1; /* can't continue */
 
  /* to get the file size */
  if(fstat(fileno(fp), &file_info) != 0)
    return 1; /* can't continue */

  curl = curl_easy_init();
  if(curl) {
    /* upload to this place */
    curl_easy_setopt(curl, CURLOPT_URL,
                     "https://filebin.net/");
                     
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);

    struct myprogress prog;

    prog.lastruntime = 0;
    prog.curl = curl;

    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);

    /* tell it to "upload" to the URL */
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    /* set where to read from (on Windows you need to use READFUNCTION too) */
    curl_easy_setopt(curl, CURLOPT_READDATA, fp);

    curl_easy_setopt(curl, CURLOPT_PUT, 1L);

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    /* enable verbose for easier tracing */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    }
    else {
      /* now extract transfer info */
      curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD_T, &speed_upload);
      curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);

      fprintf(stderr, "Speed: %" CURL_FORMAT_CURL_OFF_T " bytes/sec during %"
              CURL_FORMAT_CURL_OFF_T ".%06ld seconds\n",
              speed_upload,
              (total_time / 1000000), (long)(total_time % 1000000));

    }
    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  fclose(fp);
  return 0;
}

/* Use libcurl to upload POST data.

Usage: PostUpload <post_data_filename> <url>

Used this as a speed test in Win7 x64, see
https://github.com/curl/curl/issues/708

First test the speed in a web browser:
Go to https://testmy.net/mirror and choose the geographically closest mirror.
After the mirror is set click on 'Upload Test'.
Select 'Manual Test Size' 50MB. The browser will download 50MB then upload it.
Note the server as it's uploading. (eg: ny2.testmy.net)
After the upload completes it gives upload speed in kB/s. (eg: 633 kB/s)

2018-08-07: testmy no longer supports plain HTTP so now results may vary
depending on the SSL backend used by libcurl.

From the command line make 50MB of POST data and upload it to that server:
perl -e "print 'size_data=73400320&test_type=upload&svrPort=80'" > ulspeedtest
perl -e "print '&svrPort=80&start=9999999999999&data='" >> ulspeedtest
perl -e "print '0' x 73400320" >> ulspeedtest
perl -e "print 'size_data=52428800&test_type=upload&svrPort=80'" > ulspeedtest
perl -e "print '&svrPort=80&start=9999999999999&data='" >> ulspeedtest
perl -e "print '0' x 52428800" >> ulspeedtest
PostUpload ulspeedtest https://ny2.testmy.net/uploader

The upload speed will be printed and should be the same as in the browser:
-------------------------------------------------------------------------------
Transfer rate: 625 KB/sec (52428883 bytes in 82 seconds)

Other data rate units:

                  5.12 Mbps
               5122.19 kbps
                  0.61 MiB/s
                625.27 KiB/s
                640.27 kB/s   <-------- Compare against this if it shows kB/s
             640274.00 B/s
-------------------------------------------------------------------------------

Copyright (C) 2016 Jay Satiro <raysatiro@yahoo.com>
https://curl.haxx.se/docs/copyright.html

https://gist.github.com/jay/daac01682f17c2dc213e
*/

#define _CRT_NONSTDC_NO_DEPRECATE
#define _FILE_OFFSET_BITS 64

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>52428883,
#endif

/* https://curl.haxx.se/download.html */
#include <curl/curl.h>


#undef FALSE
#define FALSE 0
#undef TRUE
#define TRUE 1

#ifdef _WIN32
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define fstat _fstati64
#define lseek _lseeki64
#define stat _stati64
#define strncasecmp strnicmp
#endif

#ifndef CURL_OFF_T_MAX
#if (CURL_SIZEOF_CURL_OFF_T == 4)
#define CURL_OFF_T_MAX CURL_OFF_T_C(0x7FFFFFFF)
#else /* assume CURL_SIZEOF_CURL_OFF_T == 8 */
#define CURL_OFF_T_MAX CURL_OFF_T_C(0x7FFFFFFFFFFFFFFF)
#endif
#define CURL_OFF_T_MIN (-CURL_OFF_T_MAX - CURL_OFF_T_C(1))
#endif


enum progress_type {
  PROGRESS_NONE = 0,  /* NONE must stay at 0 */
  PROGRESS_STARTED,
  PROGRESS_DONE
};

/* to initialize zero out the struct then set session */
struct progress_data {
  CURL *session;             /* curl easy_handle to the calling session */
  int percent;               /* last percent output to stderr */
  time_t time;               /* last time progress was output to stderr */
  enum progress_type type;
};


int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                      curl_off_t ultotal, curl_off_t ulnow)
{
  int percent;
  time_t timenow;
  struct progress_data *d = (struct progress_data *)clientp;
  CURL *curl = d->session;

  (void)dltotal;
  (void)dlnow;

#if 0
  fprintf(stderr, "\ndltotal: %" CURL_FORMAT_CURL_OFF_T
                  ", dlnow: %" CURL_FORMAT_CURL_OFF_T
                  ", ultotal: %" CURL_FORMAT_CURL_OFF_T
                  ", ulnow: %" CURL_FORMAT_CURL_OFF_T
                  ", time: %" CURL_FORMAT_CURL_OFF_T "\n",
                  dltotal, dlnow, ultotal, ulnow, (curl_off_t)time(NULL));
#endif
  fprintf(stderr, "\ndltotal: %" CURL_FORMAT_CURL_OFF_T
                  ", dlnow: %" CURL_FORMAT_CURL_OFF_T
                  ", ultotal: %" CURL_FORMAT_CURL_OFF_T
                  ", ulnow: %" CURL_FORMAT_CURL_OFF_T
                  ", time: %" CURL_FORMAT_CURL_OFF_T "\n",
                  dltotal, dlnow, ultotal, ulnow, (curl_off_t)time(NULL));

  // curl_off_t curtime;
  // curl_off_t uploadSpeed;
  // curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME_T, &uploadSpeed);
  // fprintf(stderr, "\ndltotal: %f\n",(double)uploadSpeed);

  if(d->type == PROGRESS_DONE || ultotal <= 0 || ulnow < 0)
    return 0;
  else if(!ulnow){
    
    percent = 0;}
  else if(ulnow >= ultotal)
    percent = 100;
  else if(ultotal < 10000)
    percent = (int)(ulnow * 100 / ultotal);
  else
    percent = (int)(ulnow / (ultotal / 100));

  if(d->type == PROGRESS_STARTED && d->percent == percent)
    return 0;

  timenow = time(NULL);

  if(timenow == d->time && percent != 100)
    return 0;
633.000000 percent);

  d->percent = percent;
  d->time = timenow;
  if(percent == 100) {
    fprintf(stderr, "\n\n");
    d->type = PROGRESS_DONE;
  }
  else
    d->type = PROGRESS_STARTED;

  return 0;
}

size_t read_callback(char *buffer, size_t size, size_t nitems, void *instream)
{
  size_t bytes_read;

  /* I'm doing it this way to get closer to what the reporter is doing.
     Technically we don't need to do this, we could just use the default read
     callback which is fread. Also, 'size' param is always set to 1 by libcurl
     so it's fine to pass as buffer, size, nitems, instream. */
  bytes_read = fread(buffer, 1, (size * nitems), (FILE *)instream);

  return bytes_read;
}

/* Show some other rate units like Mbps, kB/s, etc.
https://en.wikipedia.org/wiki/Data_rate_units
*/
void show_other_rate_units(double speed_in_Bps)
{
  size_t i;
  struct rate {
    char *symbol;
    int Bps;   /* bytes per second */
  };
  struct rate rate[] = {
    { "Mbps",  1000000 / 8 },
    { "kbps",  1000 / 8 },
    { "MiB/s", 1024 * 1024 },
    { "KiB/s", 1024 },
    { "kB/s",  1000 },
    { "B/s",   1 }
  };

  fprintf(stderr, "\nOther data rate units:\n\n");

  for(i = 0; i < sizeof rate / sizeof rate[0]; ++i)
    fprintf(stderr, "%22.2f %s\n", speed_in_Bps / rate[i].Bps, rate[i].symbol);

  fprintf(stderr, "\n");
}

int PostUpload(const char *post_data_filename, const char *url)
{
  int retcode = FALSE;
  int delayed_error = FALSE;
  CURL *curl = NULL;
  CURLcode res = CURLE_FAILED_INIT;
  char errbuf[CURL_ERROR_SIZE] = { 0, };
  char *filebuf = NULL;
  FILE *fp = NULL;
  struct progress_data progress_data = { 0, };
  double average_speed = 0;
  double bytes_uploaded = 0;
  double total_upload_time = 0;
  long response_code = 0;
  struct curl_slist *header_list = NULL;
  struct stat stbuf = { 0, };

  if(!post_data_filename || !*post_data_filename) {
    fprintf(stderr, "Error: post_data_filename parameter is missing.\n");
    goto cleanup;
  }

  if(!url || !*url) {
    fprintf(stderr, "Error: url parameter is missing.\n");
    goto cleanup;
  }

  fp = fopen(post_data_filename, "rb");

  if(!fp) {
    fprintf(stderr, "Error: failed to open file \"%s\"\n", post_data_filename);
    goto cleanup;
  }

  if(fstat(fileno(fp), &stbuf) || !S_ISREG(stbuf.st_mode)) {
    fprintf(stderr, "Error: unknown file size \"%s\"\n", post_data_filename);
    goto cleanup;
  }

  if(!(0 <= stbuf.st_size && stbuf.st_size <= CURL_OFF_T_MAX)) {
    fprintf(stderr, "Error: file size too large \"%s\"\n", post_data_filename);
    goto cleanup;
  }

  curl = curl_easy_init();

  if(!curl) {
    fprintf(stderr, "Error: curl_easy_init failed.\n");
    goto cleanup;
  }

  /* CURLOPT_CAINFO
  To verify SSL sites you may need to load a bundle of certificates.

  You can upload the default bundle here:
  https://raw.githubusercontent.com/bagder/ca-bundle/master/ca-bundle.crt

  However your SSL backend might use a database in addition to or instead of
  the bundle.
  https://curl.haxx.se/docs/ssl-compared.html
  */
  //curl_easy_setopt(curl, CURLOPT_CAINFO, "curl-ca-bundle.crt");

  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE,
                   (curl_off_t)stbuf.st_size);
#if 1
  curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
  curl_easy_setopt(curl, CURLOPT_READDATA, (void *)fp);
#else
  /* Test what happens when we read the file in advance and load it in memory.
     Should be the same upload speed as when READFUNCTION is used. */
  if(stbuf.st_size > (size_t)-1) {
    fprintf(stderr, "Error: file size too large \"%s\"\n", post_data_filename);
    goto cleanup;
  }
  filebuf = malloc((size_t)stbuf.st_size);
  if(!filebuf) {
    fprintf(stderr, "Error: couldn't allocate %" CURL_FORMAT_CURL_OFF_T
            " bytes.\n", (curl_off_t)stbuf.st_size);
    goto cleanup;
  }
  if((size_t)stbuf.st_size != fread(filebuf, 1, (size_t)stbuf.st_size, fp)) {
    fprintf(stderr, "Error: couldn't read file \"%s\"\n", post_data_filename);
    goto cleanup;
  }
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, filebuf);
#endif

  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
  curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress_data);
  progress_data.session = curl;

  header_list = curl_slist_append(header_list, "Expect:");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);

  /* Include server headers in the output */
  curl_easy_setopt(curl, CURLOPT_HEADER, 1L);

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);

  /* curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); */

#ifdef DISABLE_LIBCURL_SSL_VERIFY
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

  res = curl_easy_perform(curl);

  if(res != CURLE_OK) {
    size_t len = strlen(errbuf);
    fprintf(stderr, "\nError: libcurl: (%d) ", res);
    if(len)
      fprintf(stderr, "%s%s", errbuf, ((errbuf[len - 1] != '\n') ? "\n" : ""));
    fprintf(stderr, "%s\n\n", curl_easy_strerror(res));
    goto cleanup;
  }

  curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &average_speed);
  curl_easy_getinfo(curl, CURLINFO_SIZE_UPLOAD, &bytes_uploaded);
  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_upload_time);

  fprintf(stderr, "\nTransfer rate: %.0f KB/sec"
                  " (%.0f bytes in %.0f seconds)\n",
          average_speed / 1024, bytes_uploaded, total_upload_time);

  show_other_rate_units(average_speed);

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

  if(response_code != 200) {
    fprintf(stderr, "Error: HTTP response code is %ld.\n", response_code);
    delayed_error = TRUE;
  }

  if(progress_data.type != PROGRESS_DONE) {
    fprintf(stderr, "Error: Only %d%% was uploaded.\n", progress_data.percent);
    delayed_error = TRUE;
  }

  retcode = !delayed_error;
cleanup:
  curl_easy_cleanup(curl);
  curl_slist_free_all(header_list);
  free(filebuf);
  if(fp)
    fclose(fp);

  return retcode;
}

int main(int argc, char *argv[])
{
  if(argc != 3) {
    fprintf(stderr,
      "Usage: PostUpload <post_data_filename> <url>\n"
      "\n"
      "HTTP POST to url of the data contained in post_data_filename.\n"
      "\n"
      "This program will exit 0 on success. Success is defined as all the "
      "data was uploaded AND the server returned a response code of 200.\n");
    return EXIT_FAILURE;
  }

  if(curl_global_init(CURL_GLOBAL_ALL)) {
    fprintf(stderr, "Fatal: The initialization of libcurl has failed.\n");
    return EXIT_FAILURE;
  }

  if(atexit(curl_global_cleanup)) {
    fprintf(stderr, "Fatal: atexit failed to register curl_global_cleanup.\n");
    curl_global_cleanup();
    return EXIT_FAILURE;
  }

  if(!PostUpload(argv[1], argv[2])) {
    fprintf(stderr, "Fatal: PostUpload failed.\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

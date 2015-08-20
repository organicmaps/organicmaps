// osmupdate 2015-04-15 10:00
#define VERSION "0.4.1"
//
// compile this file:
// gcc osmupdate.c -o osmupdate
//
// (c) 2011..2015 Markus Weber, Nuernberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Affero General Public License
// version 3 as published by the Free Software Foundation.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Affero General Public License for more details.
// You should have received a copy of this license along
// with this program; if not, see http://www.gnu.org/licenses/.
//
// Other licenses are available on request; please ask the author.

#define MAXLOGLEVEL 2
const char* helptext=
"\nosmupdate " VERSION "\n"
"\n"
"This program cares about updating an .osm, .o5m or .pbf file. It\n"
"will download and apply OSM Change files (.osc) from the servers of\n"
"\"planet.openstreetmap.org\".\n"
"It also can assemble a new .osc or .o5c file which can be used to\n"
"update your OSM data file at a later time.\n"
"\n"
"Prequesites\n"
"\n"
"To run this program, please download and install two other programs\n"
"first: \"osmconvert\" and \"wget\".\n"
"\n"
"Usage\n"
"\n"
"Two command line arguments are mandatory: the name of the old and the\n"
"name of the new OSM data file. If the old data file does not have a\n"
"file timestamp, you may want to specify this timestamp manually on\n"
"the command line. If you do not, the program will try to determine\n"
"the timestamp by examining the whole old data file.\n"
"Instead of the second parameter, you alternatively may specify the\n"
"name of a change file (.osc or .o5c). In this case, you also may\n"
"replace the name of the old OSM data file by a timestamp.\n"
"Command line arguments which are not recognized by osmupdate will be\n"
"passed to osmconvert. Use this opportunity to supply a bounding box\n"
"or a bounding polygon if you are going to update a regional change\n"
"file. You also may exclude unneeded meta data from your file by\n"
"specifying this osmconvert option: --drop-author\n"
"\n"
"Usage Examples\n"
"\n"
"  ./osmupdate old_file.o5m new_file.o5m\n"
"  ./osmupdate old_file.pbf new_file.pbf\n"
"  ./osmupdate old_file.osm new_file.osm\n"
"        The old OSM data will be updated and written as new_file.o5m\n"
"        or new_file.o5m. For safety reasons osmupdate will not delete\n"
"        the old file. If you do not need it as backup file, please\n"
"        delete it by yourself.\n"
"\n"
"  ./osmupdate old_file.osm 2011-07-15T23:30:00Z new_file.osm\n"
"  ./osmupdate old_file.osm NOW-86400 new_file.osm\n"
"        If your old OSM data file does not contain a file timestamp,\n"
"        or you do not want to rely on this timestamp, it can be\n"
"        specified manually. Relative times are in seconds to NOW.\n"
"\n"
"  ./osmupdate old_file.o5m change_file.o5c\n"
"  ./osmupdate old_file.osm change_file.osc\n"
"  ./osmupdate 2011-07-15T23:30:00Z change_file.o5c\n"
"  ./osmupdate 2011-07-15T23:30:00Z change_file.osc.gz\n"
"  ./osmupdate NOW-3600 change_file.osc.gz\n"
"        Here, the old OSM data file is not updated directly. An OSM\n"
"        changefile is written instead. This changefile can be used to\n"
"        update the OSM data file afterwards.\n"
"        You will have recognized the extension .gz in the last\n"
"        example. In this case, the OSM Change file will be written\n"
"        with gzip compression. To accomplish this, you need to have\n"
"        the program gzip installed on your system.\n"
"\n"
"  ./osmupdate london_old.o5m london_new.o5m -B=london.poly\n"
"        The OSM data file london_old.o5m will be updated. Hence the\n"
"        downloaded OSM changefiles contain not only London, but the\n"
"        whole planet, a lot of unneeded data will be added to this\n"
"        regional file. The -B= argument will clip these superfluous\n"
"        data.\n"
"\n"
"The program osmupdate recognizes a few command line options:\n"
"\n"
"--max-days=UPDATE_RANGE\n"
"        By default, the maximum time range for to assemble a\n"
"        cumulated changefile is 250 days. You can change this by\n"
"        giving a different maximum number of days, for example 300.\n"
"        If you do, please ensure that there are daily change files\n"
"        available for such a wide range of time.\n"
"\n"
"--minute\n"
"--hour\n"
"--day\n"
"--sporadic\n"
"        By default, osmupdate uses a combination of minutely, hourly\n"
"        and daily changefiles. If you want to limit these changefile\n"
"        categories, use one or two of these options and choose that\n"
"        category/ies you want to be used.\n"
"        The option --sporadic allows processing changefile sources\n"
"        which do not have the usual \"minute\", \"hour\" and \"day\"\n"
"        subdirectories.\n"
"\n"
"--max-merge=COUNT\n"
"        The subprogram osmconvert is able to merge more than two\n"
"        changefiles in one run. This ability increases merging speed.\n"
"        Unfortunately, every changefile consumes about 200 MB of main\n"
"        memory while being processed. For this reason, the number of\n"
"        parallely processable changefiles is limited.\n"
"        Use this commandline argument to determine the maximum number\n"
"        of parallely processed changefiles. The default value is 7.\n"
"\n"
"-t=TEMPPATH\n"
"--tempfiles=TEMPPATH\n"
"        On order to cache changefiles, osmupdate needs a separate\n"
"        directory. This parameter defines the name of this directory,\n"
"        including the prefix of the tempfiles' names.\n"
"        The default value is \"osmupdate_temp/temp\".\n"
"\n"
"--keep-tempfiles\n"
"        Use this option if you want to keep local copies of every\n"
"        downloaded file. This is strongly recommended if you are\n"
"        going to assemble different changefiles which overlap in\n"
"        time ranges. Your data traffic will be minimized.\n"
"        Do not invoke this option if you are going to use different\n"
"        change file sources (option --base-url). This would cause\n"
"        severe data corruption.\n"
"\n"
"--compression-level=LEVEL\n"
"        Define level for gzip compression. Values between 1 (low\n"
"        compression, but fast) and 9 (high compression, but slow).\n"
"\n"
"--base-url=BASE_URL\n"
"        To accelerate downloads or to get regional file updates you\n"
"        may specify an alternative download location. Please enter\n"
"        its URL, or simply the word \"mirror\" if you want to use\n"
"        gwdg's planet server.\n"
"\n"
"--base-url-suffix=BASE_URL_SUFFIX\n"
"        To use old planet URLs, you may need to add the suffix\n"
"        \"-replicate\" because it was custom to have this word in the\n"
"        URL, right after the period identifier \"day\" etc.\n"
"\n"
"-v\n"
"--verbose\n"
"        With activated \'verbose\' mode, some statistical data and\n"
"        diagnosis data will be displayed.\n"
"        If -v resp. --verbose is the first parameter in the line,\n"
"        osmupdate will display all input parameters.\n"
"\n"
"This program is for experimental use. Expect malfunctions and data\n"
"loss. Do not use the program in productive or commercial systems.\n"
"\n"
"There is NO WARRANTY, to the extent permitted by law.\n"
"Please send any bug reports to markus.weber@gmx.com\n\n";

#define _FILE_OFFSET_BITS 64
#include <zlib.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

typedef enum {false= 0,true= 1} bool;
typedef uint8_t byte;
typedef unsigned int uint;
#define isdig(x) isdigit((unsigned char)(x))
static int loglevel= 0;  // logging to stderr;
  // 0: no logging; 1: small logging; 2: normal logging;
  // 3: extended logging;
#define DP(f) fprintf(stderr,"Debug: " #f "\n");
#define DPv(f,...) fprintf(stderr,"Debug: " #f "\n",__VA_ARGS__);
#define DPM(f,p,m) { byte* pp; int i,mm; static int msgn= 3; \
  if(--msgn>=0) { fprintf(stderr,"Debug memory: " #f); \
  pp= (byte*)(p); mm= (m); if(pp==NULL) fprintf(stderr,"\n  (null)"); \
  else for(i= 0; i<mm; i++) { \
  if((i%16)==0) fprintf(stderr,"\n "); \
  fprintf(stderr," %02x",*pp++); } \
  fprintf(stderr,"\n"); } }
#if __WIN32__
  #define NL "\r\n"  // use CR/LF as new-line sequence
  #define DIRSEP '\\'
  #define DIRSEPS "\\"
  #define DELFILE "del"
  #define off_t off64_t
  #define lseek lseek64
#else
  #define NL "\n"  // use LF as new-line sequence
  #define DIRSEP '/'
  #define DIRSEPS "/"
  #define DELFILE "rm"
  #define O_BINARY 0
#endif



static inline char *strmcpy(char *dest, const char *src, size_t maxlen) {
  // similar to strcpy(), this procedure copies a character string;
  // here, the length is cared about, i.e. the target string will
  // be limited in case it is too long;
  // src[]: source string which is to be copied;
  // maxlen: maximum length of the destination string
  //         (including terminator null);
  // return:
  // dest[]: destination string of the copy; this is the
  //         function's return value too;
  char* d;

  if(maxlen==0)
return dest;
  d= dest;
  while(--maxlen>0 && *src!=0)
    *d++= *src++;
  *d= 0;
  return dest;
  }  // end   strmcpy()
#define strMcpy(d,s) strmcpy((d),(s),sizeof(d))

static inline char *stecpy(char** destp, char* destend,
    const char* src) {
  // same as stppcpy(), but you may define a pointer which the
  // destination will not be allowed to cross, whether or not the
  // string will be completed at that moment;
  // in either case, the destination string will be terminated with 0;
  char* dest;

  dest= *destp;
  if(dest>=destend)
return dest;
  destend--;
  while(*src!=0 && dest<destend)
    *dest++= *src++;
  *dest= 0;
  *destp= dest;
  return dest;
  }  // end stecpy()

static inline char *stpesccpy(char *dest, const char *src) {
  // same as C99's stpcpy(), but all quotation marks, apostrophes
  // and backslashes will be escaped (i.e., preceded) by backslashes;
  // for windows, backslashes will not be escaped;
  while(*src!=0) {
    if(*src=='\'' || *src=='\"'
        #if !__WIN32__
          || *src=='\\'
        #endif
        )
      *dest++= '\\';
    *dest++= *src++;
    }
  *dest= 0;
  return dest;
  }  // stpesccpy()

static inline char *steesccpy(char **destp,char *destend,
    const char *src) {
  // same as stppesccpy(), but you may define a pointer which the
  // destination will not be allowed to cross, whether or not the
  // string will be completed at that moment;
  // in either case, the destination string will be terminated with 0;
  char* dest;

  dest= *destp;
  if(dest>=destend)
return dest;
  destend-= 2;
  while(*src!=0 && dest<destend) {
    if(*src=='\'' || *src=='\"'
        #if !__WIN32__
          || *src=='\\'
        #endif
        )
      *dest++= '\\';
    *dest++= *src++;
    }
  *dest= 0;
  *destp= dest;
  return dest;
  }  // steesccpy()

static inline char *stpmcpy(char *dest, const char *src, size_t maxlen) {
  // similar to strmcpy(), this procedure copies a character string;
  // however, it returns the address of the destination string's
  // terminating zero character;
  // this makes it easier to concatenate strings;
  char* d;

  if(maxlen==0)
return dest;
  d= dest;
  while(--maxlen>0 && *src!=0)
    *d++= *src++;
  *d= 0;
  return d;
  }  // end stpmcpy()
#define stpMcpy(d,s) stpmcpy(d,s,sizeof(d))

static inline int strzcmp(const char* s1,const char* s2) {
  // similar to strcmp(), this procedure compares two character strings;
  // here, the number of characters which are to be compared is limited
  // to the length of the second string;
  // i.e., this procedure can be used to identify a short string s2
  // within a long string s1;
  // s1[]: first string;
  // s2[]: string to compare with the first string;
  // return:
  // 0: both strings are identical; the first string may be longer than
  //    the second;
  // -1: the first string is alphabetical smaller than the second;
  // 1: the first string is alphabetical greater than the second;
  while(*s1==*s2 && *s1!=0) { s1++; s2++; }
  if(*s2==0)
    return 0;
  return *(unsigned char*)s1 < *(unsigned char*)s2? -1: 1;
  }  // end   strzcmp()

static inline int strycmp(const char* s1,const char* s2) {
  // similar to strcmp(), this procedure compares two character strings;
  // here, both strings are end-aligned;
  // not more characters will be compared than are existing in string s2;
  // i.e., this procedure can be used to identify a file name extension;
  const char* s1e;
  int l;

  l= strchr(s2,0)-s2;
  s1e= strchr(s1,0);
  if(s1e-s1<l)
return 1;
  s1= s1e-l;
  while(*s1==*s2 && *s1!=0) { s1++; s2++; }
  if(*s2==0)
    return 0;
  return *(unsigned char*)s1 < *(unsigned char*)s2? -1: 1;
  }  // end   strycmp()

static inline uint32_t strtouint32(const char* s) {
  // read a number and convert it to an unsigned 32-bit integer;
  // return: number;
  int32_t i;
  byte b;

  i= 0;
  for(;;) {
    b= (byte)(*s++ -'0');
    if(b>=10)
  break;
    i= i*10+b;
    }
  return i;
  }  // end   strtouint32()

static inline int64_t strtosint64(const char* s) {
  // read a number and convert it to a signed 64-bit integer;
  // return: number;
  int sign;
  int64_t i;
  byte b;

  if(*s=='-') { s++; sign= -1; } else sign= 1;
  i= 0;
  for(;;) {
    b= (byte)(*s++ -'0');
    if(b>=10)
  break;
    i= i*10+b;
    }
  return i*sign;
  }  // end   strtosint64()

static inline int64_t strtimetosint64(const char* s) {
  // read a timestamp in OSM format, e.g.: "2010-09-30T19:23:30Z",
  // and convert it to a signed 64-bit integer;
  // also allowed: relative time to NOW, e.g.: "NOW-86400",
  // which means '24 hours ago';
  // return: time as a number (seconds since 1970);
  //         ==0: syntax error;
  if(s[0]=='N') {  // presumably a relative time to 'now'
    if(s[1]!='O' || s[2]!='W' || (s[3]!='+' && s[3]!='-') ||
        !isdig(s[4]))  // wrong syntax
return 0;
    s+= 3;  // jump over "NOW"
    if(*s=='+') s++;  // jump over '+', if any
return time(NULL)+strtosint64(s);
    }  // presumably a relative time to 'now'
  if((s[0]!='1' && s[0]!='2') ||
      !isdig(s[1]) || !isdig(s[2]) || !isdig(s[3]) ||
      s[4]!='-' || !isdig(s[5]) || !isdig(s[6]) ||
      s[7]!='-' || !isdig(s[8]) || !isdig(s[9]) ||
      s[10]!='T' || !isdig(s[11]) || !isdig(s[12]) ||
      s[13]!=':' || !isdig(s[14]) || !isdig(s[15]) ||
      s[16]!=':' || !isdig(s[17]) || !isdig(s[18]) ||
      s[19]!='Z')  // wrong syntax
return 0;
  /* regular timestamp */ {
    struct tm tm;

    tm.tm_isdst= 0;
    tm.tm_year=
      (s[0]-'0')*1000+(s[1]-'0')*100+(s[2]-'0')*10+(s[3]-'0')-1900;
    tm.tm_mon= (s[5]-'0')*10+s[6]-'0'-1;
    tm.tm_mday= (s[8]-'0')*10+s[9]-'0';
    tm.tm_hour= (s[11]-'0')*10+s[12]-'0';
    tm.tm_min= (s[14]-'0')*10+s[15]-'0';
    tm.tm_sec= (s[17]-'0')*10+s[18]-'0';
    #if __WIN32__
return mktime(&tm)-timezone;
    #else
return timegm(&tm);
    #endif
    }  // regular timestamp
  }  // end   strtimetosint64()

static inline void int64tostrtime(uint64_t v,char* sp) {
  // write a timestamp in OSM format, e.g.: "2010-09-30T19:23:30Z",
  // into a string;
  // v: value of the timestamp;
  // sp[21]: destination string;
  time_t vtime;
  struct tm tm;
  int i;

  vtime= v;
  #if __WIN32__
  memcpy(&tm,gmtime(&vtime),sizeof(tm));
  #else
  gmtime_r(&vtime,&tm);
  #endif
  i= tm.tm_year+1900;
  sp+= 3; *sp--= i%10+'0';
  i/=10; *sp--= i%10+'0';
  i/=10; *sp--= i%10+'0';
  i/=10; *sp= i%10+'0';
  sp+= 4; *sp++= '-';
  i= tm.tm_mon+1;
  *sp++= i/10+'0'; *sp++= i%10+'0'; *sp++= '-';
  i= tm.tm_mday;
  *sp++= i/10+'0'; *sp++= i%10+'0'; *sp++= 'T';
  i= tm.tm_hour;
  *sp++= i/10+'0'; *sp++= i%10+'0'; *sp++= ':';
  i= tm.tm_min;
  *sp++= i/10+'0'; *sp++= i%10+'0'; *sp++= ':';
  i= tm.tm_sec%60;
  *sp++= i/10+'0'; *sp++= i%10+'0'; *sp++= 'Z'; *sp= 0;
  }  // end   int64tostrtime()

static inline bool file_exists(const char* file_name) {
  // query if a file exists;
  // file_name[]: name of the file in question;
  // return: the file exists;
  return access(file_name,R_OK)==0;
  }  // file_exists()

static inline int64_t file_length(const char* file_name) {
  // retrieve length of a file;
  // file_name[]: file name;
  // return: number of bytes of this file;
  //         if the file could not be accessed, the return value is -1;
  struct stat s;
  int r;

  r= stat(file_name,&s);
  if(r==0)
return s.st_size;
  #if !__WIN32__
    if(errno==EOVERFLOW)  // size larger than 2^31
return 0x7fffffff;
  #endif
  return -1;
  }  // end   file_length()



//------------------------------------------------------------
// Module Global   global variables for this program
//------------------------------------------------------------

// to distinguish global variable from local or module global
// variables, they are preceded by 'global_';

static const char global_osmconvert_program_here_in_dir[]=
  "./osmconvert";
static const char* global_osmconvert_program=
  global_osmconvert_program_here_in_dir+2;
  // path to osmconvert program
static char global_tempfile_name[450]= "";
  // prefix of names for temporary files
static bool global_keep_tempfiles= false;
  // temporary files shall not be deleted at program end
static char global_osmconvert_arguments[2000]= "";
  // general command line arguments for osmconvert;
#define max_number_of_changefiles_in_cache 100
static int global_max_merge= 7;
  // maximum number of parallely processed changefiles
static const char* global_gzip_parameters= "";
  // parameters for gzip compression
static char global_base_url[400]=
  "http://planet.openstreetmap.org/replication";
static char global_base_url_suffix[100]="";
  // for old replication URL, to get "day-replication" instead of "day"

#define PERR(f) \
  fprintf(stderr,"osmupdate Error: " f "\n");
  // print error message
#define PERRv(f,...) \
  fprintf(stderr,"osmupdate Error: " f "\n",__VA_ARGS__);
  // print error message with value(s)
#define WARN(f) { static int msgn= 3; if(--msgn>=0) \
  fprintf(stderr,"osmupdate Warning: " f "\n"); }
  // print a warning message, do it maximal 3 times
#define WARNv(f,...) { static int msgn= 3; if(--msgn>=0) \
  fprintf(stderr,"osmupdate Warning: " f "\n",__VA_ARGS__); }
  // print a warning message with value(s), do it maximal 3 times
#define PINFO(f) \
  fprintf(stderr,"osmupdate: " f "\n"); // print info message
#define PINFOv(f,...) \
  fprintf(stderr,"osmupdate: " f "\n",__VA_ARGS__);
#define ONAME(i) \
  (i==0? "node": i==1? "way": i==2? "relation": "unknown object")

//------------------------------------------------------------
// end   Module Global   global variables for this program
//------------------------------------------------------------



static void shell_command(const char* command,char* result) {
  // execute a shell command;
  // command[]: shell command;
  // result[1000]: result of the command;
  FILE* fp;
  char* result_p;
  int maxlen;
  int r;

  if(loglevel>=2)
    PINFOv("Executing shell command:\n%s",command)
  fp= popen(command,"r");
  if(fp==NULL) {
    PERR("Could not execute shell command.")
    result[0]= 0;
exit(1);
    }
  result_p= result;
  maxlen= 1000-1;
  while(maxlen>0) {
    r= read(fileno(fp),result_p,maxlen);
    if(r==0)  // end of data
  break;
    if(r<0)
exit(errno);  // (thanks to Ben Konrath)
    result_p+= r;
    maxlen-= r;
    }
  *result_p= 0;
  if(pclose(fp)==-1)
exit(errno);  // (thanks to Ben Konrath)
  if(loglevel>=2)
    PINFOv("Got shell command result:\n%s",result)
  }  // end   shell_command()

typedef enum {cft_UNKNOWN,cft_MINUTELY,cft_HOURLY,cft_DAILY,cft_SPORADIC}
  changefile_type_t;
#define CFTNAME(i) \
  (i==cft_MINUTELY? "minutely": i==cft_HOURLY? "hourly": \
  i==cft_DAILY? "daily": i==cft_SPORADIC? "sporadic": "unknown")

static int64_t get_file_timestamp(const char* file_name) {
  // get the timestamp of a specific file;
  // if the file timestamp is not available, this procedure tries
  // to retrieve the timestamp from the file's statistics;
  // return: timestamp of the file (seconds from Jan 01 1970);
  //         ==0: no file timestamp available;
  char command[500],*command_p,result[1000];
  char* command_e= command+sizeof(command);
  int64_t file_timestamp;

  command_p= command;
  stecpy(&command_p,command_e,global_osmconvert_program);
  stecpy(&command_p,command_e," --out-timestamp \"");
  steesccpy(&command_p,command_e,file_name);
  stecpy(&command_p,command_e,"\" 2>&1");
  shell_command(command,result);
  if(result[0]!='(' && (result[0]<'1' || result[0]>'2')) {
      // command not found
    PERR("Please install program osmconvert first.")
exit(1);
    }  // command not found
  file_timestamp= strtimetosint64(result);
  if(file_timestamp==0) {  // the file has no file timestamp
    // try to get the timestamp from the file's statistics
    char* p;

    if(loglevel>0) {  // verbose mode
      PINFOv("file %s has no file timestamp.",file_name)
      PINFO("Running statistics to get the timestamp.")
      }
    command_p= command;
    stecpy(&command_p,command_e,global_osmconvert_program);
    stecpy(&command_p,command_e," --out-statistics \"");
    steesccpy(&command_p,command_e,file_name);
    stecpy(&command_p,command_e,"\" 2>&1");
    shell_command(command,result);
    p= strstr(result,"timestamp max: ");
    if(p!=NULL) {
      file_timestamp= strtimetosint64(p+15);
      PINFO("Aging the timestamp by 4 hours for safety reasons.")
      file_timestamp-= 4*3600;
      }
    }  // the file has no file timestamp
  if(loglevel>0) {  // verbose mode
    char ts[30];

    if(file_timestamp==0)
      strcpy(ts,"(no timestamp)");
    else
      int64tostrtime(file_timestamp,ts);
    PINFOv("timestamp of %s: %s",file_name,ts)
    }  // verbose mode
  return file_timestamp;
  }  // get_file_timestamp()

static int64_t get_newest_changefile_timestamp(
    changefile_type_t changefile_type,int32_t* file_sequence_number) {
  // get sequence number and timestamp of the newest changefile
  // of a specific changefile type;
  // changefile_type: minutely, hourly, daily, sporadic changefile;
  // return: timestamp of the file (seconds from Jan 01 1970);
  //         ==0: no file timestamp available;
  // *file_sequence_number: sequence number of the newest changefile;
  static bool firstrun= true;
  char command[1000],*command_p;
  char* command_e= command+sizeof(command);
  char result[1000];
  int64_t changefile_timestamp;

  #if __WIN32__
    static char newest_timestamp_file_name[400];
    if(firstrun) {
      // create the file name for the newest timestamp;
      // this is only needed for Windows as Windows' wget
      // cannot write the downloaded file to standard output;
      // usually: "osmupdate_temp/temp.7"
      strcpy(stpmcpy(newest_timestamp_file_name,global_tempfile_name,
        sizeof(newest_timestamp_file_name)-5),".7");
      }
  #endif
  command_p= command;
  stecpy(&command_p,command_e,
    "wget -q ");
  stecpy(&command_p,command_e,global_base_url);
  switch(changefile_type) {  // changefile type
  case cft_MINUTELY:
    stecpy(&command_p,command_e,"/minute");
    break;
  case cft_HOURLY:
    stecpy(&command_p,command_e,"/hour");
    break;
  case cft_DAILY:
    stecpy(&command_p,command_e,"/day");
    break;
  case cft_SPORADIC:
    break;
  default:  // invalid change file type
return 0;
    }  // changefile type
  stecpy(&command_p,command_e,global_base_url_suffix);
  stecpy(&command_p,command_e,"/state.txt");
  #if __WIN32__
    stecpy(&command_p,command_e," -O \"");
    steesccpy(&command_p,command_e,newest_timestamp_file_name);
    stecpy(&command_p,command_e,"\" 2>&1");
  #else
    stecpy(&command_p,command_e," -O - 2>&1");
  #endif
  shell_command(command,result);
  if(firstrun) {  // first run
    firstrun= false;
    if(result[0]!='#' && (result[0]<'1' || result[0]>'2') && (
        strstr(result,"not found")!=NULL ||
        strstr(result,"cannot find")!=NULL)) {  // command not found
      PERR("Please install program wget first.")
exit(1);
      }  // command not found
    }  // first run
  #if __WIN32__
    result[0]= 0;
    /* copy tempfile contents to result[] */ {
      int fd,r;

      fd= open(newest_timestamp_file_name,O_RDONLY);
      if(fd>0) {
        r= read(fd,result,sizeof(result)-1);
        if(r>=0)
          result[r]= 0;
        close(fd);
        }
      }
    if(loglevel<2)
      unlink(newest_timestamp_file_name);
  #endif
  if(result[0]=='#') {  // full status information
    // get sequence number
    char* sequence_number_p;
    sequence_number_p= strstr(result,"sequenceNumber=");
    if(sequence_number_p!=NULL)  // found sequence number
      *file_sequence_number= strtouint32(sequence_number_p+15);
    // get timestamp
    char* timestamp_p;
    timestamp_p= strstr(result,"timestamp=");
      // search timestamp line
    if(timestamp_p!=NULL && timestamp_p-result<sizeof(result)-30) {
        // found timestamp line
      // cpy timestam to begin of result[]
      timestamp_p+= 10;  // jump over text
      memcpy(result,timestamp_p,13);
      memcpy(result+13,timestamp_p+14,3);
      memcpy(result+16,timestamp_p+18,4);
      }  // found timestamp line
    }  // full status information
  changefile_timestamp= strtimetosint64(result);
  if(loglevel>0) {  // verbose mode
    char ts[30];

    if(changefile_timestamp==0)
      strcpy(ts,"(no timestamp)");
    else
      int64tostrtime(changefile_timestamp,ts);
    PINFOv("newest %s timestamp: %s",CFTNAME(changefile_type),ts)
    }  // verbose mode
return changefile_timestamp;
  }  // get_newest_changefile_timestamp

static int64_t get_changefile_timestamp(
    changefile_type_t changefile_type,int32_t file_sequence_number) {
  // download and inspect the timestamp of a specific changefile which
  // is available in the Internet;
  // a timestamp file will not be downloaded if it
  // already exists locally as temporary file;
  // changefile_type: minutely, hourly, daily, sporadic changefile;
  // file_sequence_number: sequence number of the file;
  // uses:
  // global_tempfile_name
  // return: timestamp of the changefile (seconds from Jan 01 1970);
  //         ==0: no file timestamp available;
  char command[2000]; char* command_p;
  char* command_e= command+sizeof(command);
  char result[1000];
  char timestamp_cachefile_name[400];
  int fd,r;  // file descriptor; number of bytes which have been read
  char timestamp_contents[1000];  // contents of the timestamp
  int64_t changefile_timestamp;
  char* sp;

  // create the file name for the cached timestamp; example:
  // "osmupdate_temp/temp.m000012345.txt"
  sp= stpmcpy(timestamp_cachefile_name,global_tempfile_name,
    sizeof(timestamp_cachefile_name[0])-20);
  *sp++= '.';
  *sp++= CFTNAME(changefile_type)[0];
    // 'm', 'h', 'd', 's': minutely, hourly, daily, sporadic timestamps
  sprintf(sp,"%09"PRIi32".txt",file_sequence_number);
    // add sequence number and file name extension

  // download the timestamp into a cache file
  if(file_length(timestamp_cachefile_name)<10) {
      // timestamp has not been downloaded yet
    command_p= command;
    stecpy(&command_p,command_e,"wget -nv -c ");
    stecpy(&command_p,command_e,global_base_url);
    if(changefile_type==cft_MINUTELY)
      stecpy(&command_p,command_e,"/minute");
    else if(changefile_type==cft_HOURLY)
      stecpy(&command_p,command_e,"/hour");
    else if(changefile_type==cft_DAILY)
      stecpy(&command_p,command_e,"/day");
    else if(changefile_type==cft_SPORADIC)
      ;
    else  // invalid change file type
      return 0;
    stecpy(&command_p,command_e,global_base_url_suffix);
    stecpy(&command_p,command_e,"/");
    /* assemble Sequence path */ {
      int l;
      l= sprintf(command_p,"%03i/%03i/%03i",
        file_sequence_number/1000000,file_sequence_number/1000%1000,
        file_sequence_number%1000);
      command_p+= l;
      }
    stecpy(&command_p,command_e,".state.txt -O \"");
    steesccpy(&command_p,command_e,timestamp_cachefile_name);
    stecpy(&command_p,command_e,"\" 2>&1");
    shell_command(command,result);
    }  // timestamp has not been downloaded yet

  // read the timestamp cache file
  fd= open(timestamp_cachefile_name,O_RDONLY|O_BINARY);
  if(fd<=0)  // could not open the file
    timestamp_contents[0]= 0;  // hence we did not read anything
  else {  // could open the file
    r= read(fd,timestamp_contents,sizeof(timestamp_contents)-1);
    if(r<0) r= 0;
    timestamp_contents[r]= 0;  // set string terminator
    close(fd);
    }  // could open the file

  // parse the timestamp information
  if(timestamp_contents[0]=='#') {  // full status information
    // get timestamp
    char* timestamp_p;
    timestamp_p= strstr(timestamp_contents,"timestamp=");
      // search timestamp line
    if(timestamp_p!=NULL &&
        timestamp_p-timestamp_contents<sizeof(timestamp_contents)-30) {
        // found timestamp line
      // copy timestamp to begin of timestamp_contents[]
      timestamp_p+= 10;  // jump over text
      memcpy(timestamp_contents,timestamp_p,13);
      memcpy(timestamp_contents+13,timestamp_p+14,3);
      memcpy(timestamp_contents+16,timestamp_p+18,4);
      }  // found timestamp line
    }  // full status information
  changefile_timestamp= strtimetosint64(timestamp_contents);

  if(loglevel>0) {  // verbose mode
    char ts[30];

    if(changefile_timestamp==0)
      strcpy(ts,"(no timestamp)");
    else
      int64tostrtime(changefile_timestamp,ts);
    PINFOv("%s changefile %i: %s",
      CFTNAME(changefile_type),file_sequence_number,ts)
    }  // verbose mode
  if(changefile_timestamp==0) {  // no timestamp
    if(file_sequence_number==0)  // first file in repository
      changefile_timestamp= 1;  // set virtual very old timestamp
    else {
      PERRv("no timestamp for %s changefile %i.",
        CFTNAME(changefile_type),file_sequence_number)
exit(1);
      }
    }  // no timestamp
return changefile_timestamp;
  }  // get_changefile_timestamp

static void process_changefile(
    changefile_type_t changefile_type,int32_t file_sequence_number,
    int64_t new_timestamp) {
  // download and process a change file;
  // change files will not be processed one by one, but cumulated
  // until some files have been downloaded and then processed in a group;
  // a file will not be downloaded if it already exists locally as
  // temporary file;
  // changefile_type: minutely, hourly, daily, sporadic changefile;
  // file_sequence_number: sequence number of the file;
  //                       ==0: process the remaining files which
  //                            are waiting in the cache; cleanup;
  // new_timestamp: timestamp of the new file which is to be created;
  //            ==0: the procedure will assume the newest of all
  //                 timestamps which has been passed since the
  //                 program has been started;
  // uses:
  // global_max_merge
  // global_tempfile_name
  static bool firstrun= true;
  static int number_of_changefiles_in_cache= 0;
  static int64_t newest_new_timestamp= 0;
  static char master_cachefile_name[400];
  static char master_cachefile_name_temp[400];
  static char cachefile_name[max_number_of_changefiles_in_cache][400];
  char command[4000+200*max_number_of_changefiles_in_cache];
  char* command_e= command+sizeof(command);
  char* command_p;
  char result[1000];

  if(firstrun) {
    firstrun= false;
    // create the file name for the cached master changefile;
    // usually: "osmupdate_temp/temp.8"
    strcpy(stpmcpy(master_cachefile_name,global_tempfile_name,
      sizeof(master_cachefile_name)-5),".8");
    strcpy(stpmcpy(master_cachefile_name_temp,global_tempfile_name,
      sizeof(master_cachefile_name_temp)-5),".9");
    unlink(master_cachefile_name);
    unlink(master_cachefile_name_temp);
    }
  if(new_timestamp>newest_new_timestamp)
    newest_new_timestamp= new_timestamp;
  if(file_sequence_number!=0) {  // changefile download requested
    char* this_cachefile_name=
      cachefile_name[number_of_changefiles_in_cache];
    int64_t old_file_length;
    char* sp;

    // create the file name for the cached changefile; example:
    // "osmupdate_temp/temp.m000012345.osc.gz"
    sp= stpmcpy(this_cachefile_name,global_tempfile_name,
      sizeof(cachefile_name[0])-20);
    *sp++= '.';
    *sp++= CFTNAME(changefile_type)[0];
      // 'm', 'h', 'd', 's': minutely, hourly, daily,
      //                     sporadic changefiles
    sprintf(sp,"%09"PRIi32".osc.gz",file_sequence_number);
      // add sequence number and file name extension

    // assemble the URL and download the changefile
    old_file_length= file_length(this_cachefile_name);
    if(loglevel>0 && old_file_length<10)
        // verbose mode AND file not downloaded yet
      PINFOv("%s changefile %i: downloading",
        CFTNAME(changefile_type),file_sequence_number)
    command_p= command;
    stecpy(&command_p,command_e,"wget -nv -c ");
    stecpy(&command_p,command_e,global_base_url);
    switch(changefile_type) {  // changefile type
    case cft_MINUTELY:
      stecpy(&command_p,command_e,"/minute");
      break;
    case cft_HOURLY:
      stecpy(&command_p,command_e,"/hour");
      break;
    case cft_DAILY:
      stecpy(&command_p,command_e,"/day");
      break;
    case cft_SPORADIC:
      break;
    default:  // invalid change file type
      return;
      }  // changefile type
    stecpy(&command_p,command_e,global_base_url_suffix);
    stecpy(&command_p,command_e,"/");

    /* process sequence number */ {
      int l;
      l= sprintf(command_p,"%03i/%03i/%03i.osc.gz",
        file_sequence_number/1000000,file_sequence_number/1000%1000,
        file_sequence_number%1000);
      command_p+= l;
      }  // process sequence number

    stecpy(&command_p,command_e," -O \"");
    steesccpy(&command_p,command_e,this_cachefile_name);
    stecpy(&command_p,command_e,"\" 2>&1 && echo \"Wget Command Ok\"");
    shell_command(command,result);
    if(strstr(result,"Wget Command Ok")==NULL) {  // download error
      PERRv("Could not download %s changefile %i",
        CFTNAME(changefile_type),file_sequence_number)
      PINFOv("wget Error message:\n%s",result)
exit(1);
      }
    if(loglevel>0 && old_file_length>=10) {
        // verbose mode AND file was already in cache
      if(file_length(this_cachefile_name)!=old_file_length)
        PINFOv("%s changefile %i: download completed",
          CFTNAME(changefile_type),file_sequence_number)
      else
        PINFOv("%s changefile %i: already in cache",
          CFTNAME(changefile_type),file_sequence_number)
      }  // verbose mode
    number_of_changefiles_in_cache++;
    }  // changefile download requested

  if(number_of_changefiles_in_cache>=global_max_merge
      || (file_sequence_number==0 && number_of_changefiles_in_cache>0)) {
      // at least one change files must be merged
    // merge all changefiles which are waiting in cache
    if(loglevel>0)
      PINFO("Merging changefiles.")
    command_p= command;
    stecpy(&command_p,command_e,global_osmconvert_program);
    stecpy(&command_p,command_e," --merge-versions ");
    stecpy(&command_p,command_e,global_osmconvert_arguments);
    while(number_of_changefiles_in_cache>0) {
        // for all changefiles in cache
      number_of_changefiles_in_cache--;
      stecpy(&command_p,command_e," \"");
      steesccpy(&command_p,command_e,
        cachefile_name[number_of_changefiles_in_cache]);
      stecpy(&command_p,command_e,"\"");
      }  // for all changefiles in cache
    if(file_exists(master_cachefile_name)) {
      stecpy(&command_p,command_e," \"");
      steesccpy(&command_p,command_e,master_cachefile_name);
      stecpy(&command_p,command_e,"\"");
      }
    if(newest_new_timestamp!=0) {
      stecpy(&command_p,command_e," --timestamp=");
      if(command_e-command_p>=30)
        int64tostrtime(newest_new_timestamp,command_p);
      command_p= strchr(command_p,0);
      }
    stecpy(&command_p,command_e," --out-o5c >\"");
    steesccpy(&command_p,command_e,master_cachefile_name_temp);
    stecpy(&command_p,command_e,"\"");
    shell_command(command,result);
    if(file_length(master_cachefile_name_temp)<10 ||
        strstr(result,"Error")!=NULL ||
        strstr(result,"error")!=NULL) {  // merging failed
      PERRv("Merging of changefiles failed:\n%s",command)
      if(result[0]!=0)
        PERRv("%s",result)
exit(1);
      }  // merging failed
    unlink(master_cachefile_name);
    rename(master_cachefile_name_temp,master_cachefile_name);
    }  // at lease one change files must be merged
  }  // process_changefile()

#if !__WIN32__
void sigcatcher(int sig) {
  fprintf(stderr,"osmupdate: Output has been terminated.\n");
  exit(1);
  }  // end   sigchatcher()
#endif

int main(int argc,const char** argv) {
  // main procedure;
  // for the meaning of the calling line parameters please look at the
  // contents of helptext[];

  // variables for command line arguments
  int main_return_value;
  const char* a;  // command line argument
  char* osmconvert_arguments_p;
    // pointer in global_osmconvert_arguments[]
  char final_osmconvert_arguments[2000];
    // command line arguments for the final run of osmconvert;
  char* final_osmconvert_arguments_p;
    // pointer in final_osmconvert_arguments[]
  const char* old_file;  // name of the old OSM file
  int64_t old_timestamp;  // timestamp of the old OSM file
  const char* new_file;  // name of the new OSM file or OSM Change file
  bool new_file_is_o5;  // the new file is type .o5m or .o5c
  bool new_file_is_pbf;  // the new file is type .pbf
  bool new_file_is_changefile;  // the new file is a changefile
  bool new_file_is_gz;  // the new file is a gzip compressed
  int64_t max_update_range;  // maximum range for cumulating changefiles
    // in order to update an OSM file; unit: seconds;
  char tempfile_directory[400];  // directory for temporary files
  bool process_minutely,process_hourly,process_daily,process_sporadic;
    // if one of these variables is true, then only the chosen categories
    // shall be processed;
  bool no_minutely,no_hourly,no_daily;
    // the category shall not be processed;

  // regular variables
  int64_t minutely_timestamp,hourly_timestamp,daily_timestamp,
    sporadic_timestamp;
    // timestamps for changefiles which are available in the Internet;
    // unit: seconds after Jan 01 1970;
  int32_t minutely_sequence_number,hourly_sequence_number,
    daily_sequence_number,sporadic_sequence_number;
  int64_t timestamp;
  int64_t next_timestamp;

  // care about clean-up procedures
  #if !__WIN32__
  /* care about signal handler */ {
    static struct sigaction siga;

    siga.sa_handler= sigcatcher;
    sigemptyset(&siga.sa_mask);
    siga.sa_flags= 0;
    sigaction(SIGPIPE,&siga,NULL);
    }
  #endif

  // initializations
  main_return_value= 0;  // (default)
  #if __WIN32__
    setmode(fileno(stdout),O_BINARY);
    setmode(fileno(stdin),O_BINARY);
  #endif
  osmconvert_arguments_p= global_osmconvert_arguments;
  final_osmconvert_arguments[0]= 0;
  final_osmconvert_arguments_p= final_osmconvert_arguments;
  old_file= NULL;
  old_timestamp= 0;
  new_file= NULL;
  new_file_is_o5= false;
  new_file_is_pbf= false;
  new_file_is_changefile= false;
  new_file_is_gz= false;
  max_update_range= 250*86400;
  process_minutely= process_hourly= process_daily= process_sporadic=
    false;
  no_minutely= no_hourly= no_daily= false;
  if(file_exists(global_osmconvert_program))
      // must be Linux (no ".exe" at the end) AND
      // osmconvert program seems to be in this directory
    global_osmconvert_program= global_osmconvert_program_here_in_dir;

  // read command line parameters
  if(argc<=1) {  // no command line parameters given
    fprintf(stderr,"osmupdate " VERSION "\n"
      "Updates .osm and .o5m files, downloads .osc and o5c files.\n"
      "To get detailed help, please enter: ./osmupdate -h\n");
return 0;  // end the program, because without having parameters
      // we do not know what to do;
    }
  while(--argc>0) {  // for every parameter in command line
    argv++;  // switch to next parameter; as the first one is just
      // the program name, we must do this prior reading the
      // first 'real' parameter;
    a= argv[0];
    if(loglevel>0)  // verbose mode
      fprintf(stderr,"osmupdate Parameter: %.2000s\n",a);
    if(strcmp(a,"-h")==0 || strcmp(a,"-help")==0 ||
        strcmp(a,"--help")==0) {  // user wants help text
      fprintf(stdout,"%s",helptext);  // print help text
        // (took "%s", to prevent oversensitive compiler reactions)
return 0;
      }
    if((strcmp(a,"-v")==0 || strcmp(a,"--verbose")==0 ||
        strzcmp(a,"-v=")==0 || strzcmp(a,"--verbose=")==0) &&
        loglevel==0) {  // test mode - if not given already
      char* sp;

      sp= strchr(a,'=');
      if(sp!=NULL) loglevel= sp[1]-'0'; else loglevel= 1;
      if(loglevel<1) loglevel= 1;
      if(loglevel>MAXLOGLEVEL) loglevel= MAXLOGLEVEL;
      if(a[1]=='-') {  // must be "--verbose" and not "-v"
        if(loglevel==1)
          fprintf(stderr,"osmupdate: Verbose mode.\n");
        else
          fprintf(stderr,"osmupdate: Verbose mode %i.\n",loglevel);
        }
  continue;  // take next parameter
      }
    if(strzcmp(a,"--max-days=")==0) {  // maximum update range
      max_update_range= (int64_t)strtouint32(a+11)*86400;
  continue;  // take next parameter
      }
    if((strzcmp(a,"-t=")==0 || strzcmp(a,"--tempfiles=")==0) &&
        global_tempfile_name[0]==0) {
        // user-defined prefix for names of temorary files
      strmcpy(global_tempfile_name,strchr(a,'=')+1,
        sizeof(global_tempfile_name)-50);
  continue;  // take next parameter
      }
    if(strzcmp(a,"--keep-tempfiles")==0) {
        // temporary files shall not be deleted at program end
      global_keep_tempfiles= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--compression-level=")==0) {
        // gzip compression level
      static char gzip_par[3]= "";

      if(a[20]<'1' || a[20]>'9' || a[21]!=0) {
        PINFO("Range error. Changed to --compression-level=3")
        gzip_par[0]= '-'; gzip_par[1]= '3'; gzip_par[2]= 0;
        }
      else {
        gzip_par[0]= '-'; gzip_par[1]= a[20]; gzip_par[2]= 0;
        global_gzip_parameters= gzip_par;
        }
  continue;  // take next parameter
      }
    if(strzcmp(a,"--max-merge=")==0) {
        // maximum number of parallely processed changefiles
      global_max_merge= strtouint32(a+12);
      if(global_max_merge<2) {
        global_max_merge= 2;
        PINFO("Range error. Increased to --max-merge=2")
        }
      if(global_max_merge>max_number_of_changefiles_in_cache) {
        global_max_merge= max_number_of_changefiles_in_cache;
        PINFOv("Range error. Decreased to --max-merge=%i",
          max_number_of_changefiles_in_cache)
        }
  continue;  // take next parameter
      }
    if(strzcmp(a,"--minute")==0) {  // process minutely data
        // accept "--minute" as well as old syntax "--minutely"
      process_minutely= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--hour")==0) {  // process hourly data
        // accept "--hour" as well as old syntax "--hourly"
      process_hourly= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--day")==0 || strzcmp(a,"--daily")==0) {
        // process daily data;
        // accept "--day" as well as old syntax "--daily"
      process_daily= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--sporadic")==0) {
        // process sporadic data;
      process_sporadic= true;
  continue;  // take next parameter
      }
    if((strzcmp(a,"--base-url=")==0 && a[11]!=0) ||
        (strzcmp(a,"--planet-url=")==0 && a[13]!=0)) {
        // change base url
        // the second option keyword is deprecated but still supported
      const char* ap;
      char* sp;

      ap= a+11;
      if(a[2]=='p') ap+= 2;
      if(strcmp(ap,"mirror")==0)
        strcpy(global_base_url,"ftp://ftp5.gwdg.de/pub/misc/"
          "openstreetmap/planet.openstreetmap.org/replication");
      else if(strstr(ap,"://")!=NULL)
        strmcpy(global_base_url,ap,sizeof(global_base_url)-1);
      else {
        strcpy(global_base_url,"http://");
        strmcpy(global_base_url+7,ap,sizeof(global_base_url)-8);
        }
      sp= strchr(global_base_url,0);
      if(sp>global_base_url && sp[-1]=='/')
        sp[-1]= 0;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--base-url-suffix=")==0 && a[20]!=0) {
        // change base url suffix
      strMcpy(global_base_url_suffix,a+18);
  continue;  // take next parameter
      }
    if(strzcmp(a,"--planet-url-suffix=")==0 && a[20]!=0) {
        // change base url suffix (this option keyword is deprecated)
      strMcpy(global_base_url_suffix,a+20);
  continue;  // take next parameter
      }
    if(a[0]=='-') {
        // command line argument not recognized by osmupdate
      // store it so we can pass it to osmconvert later
      int len;

      len= strlen(a)+3;
      if(osmconvert_arguments_p-global_osmconvert_arguments+len
          >=sizeof(global_osmconvert_arguments) ||
          final_osmconvert_arguments_p-final_osmconvert_arguments+len
          >=sizeof(final_osmconvert_arguments)) {
        PERR("too many command line arguments for osmconvert.")
return 1;
        }
      if(strcmp(a,"--complete-ways")==0 ||
          strcmp(a,"--complex-ways")==0 ||
          strcmp(a,"--drop-brokenrefs")==0 ||
          strcmp(a,"--drop-broken-refs")==0) {
        WARNv("option %.80s does not work with updates.",a)
  continue;  // take next parameter
        }
      if(strzcmp(a,"-b=")!=0 && strzcmp(a,"-B=")!=0) {
          // not a bounding box and not a bounding polygon
        *osmconvert_arguments_p++= ' ';
        *osmconvert_arguments_p++= '\"';
        osmconvert_arguments_p= stpesccpy(osmconvert_arguments_p,a);
        *osmconvert_arguments_p++= '\"';
        *osmconvert_arguments_p= 0;
        }
      *final_osmconvert_arguments_p++= ' ';
      *final_osmconvert_arguments_p++= '\"';
      final_osmconvert_arguments_p=
        stpesccpy(final_osmconvert_arguments_p,a);
      *final_osmconvert_arguments_p++= '\"';
      *final_osmconvert_arguments_p= 0;
  continue;  // take next parameter
      }
    if(old_timestamp==0) {
      old_timestamp= strtimetosint64(a);
      if(old_timestamp!=0)  // this is a valid timestamp
  continue;  // take next parameter
      }
    // here: parameter must be a file name
    if(old_file==NULL && old_timestamp==0) {  // name of the old file
      old_file= a;
  continue;  // take next parameter
      }
    if(new_file==NULL) {  // name of the new file
      new_file= a;
      new_file_is_o5=
        strycmp(new_file,".o5m")==0 || strycmp(new_file,".o5c")==0 ||
        strycmp(new_file,".o5m.gz")==0 ||
        strycmp(new_file,".o5c.gz")==0;
      new_file_is_pbf=
        strycmp(new_file,".pbf")==0;
      new_file_is_changefile=
        strycmp(new_file,".osc")==0 || strycmp(new_file,".o5c")==0 ||
        strycmp(new_file,".osc.gz")==0 ||
        strycmp(new_file,".o5c.gz")==0;
      new_file_is_gz= strycmp(new_file,".gz")==0;
  continue;  // take next parameter
      }
    }  // end   for every parameter in command line

  /* create tempfile directory for cached timestamps and changefiles */ {
    char *sp;

    if(strlen(global_tempfile_name)<2)  // not set yet
      strcpy(global_tempfile_name,"osmupdate_temp"DIRSEPS"temp");
        // take default
    sp= strchr(global_tempfile_name,0);
    if(sp[-1]==DIRSEP)  // it's a bare directory
      strcpy(sp,"temp");  // add a file name prefix
    strMcpy(tempfile_directory,global_tempfile_name);
    sp= strrchr(tempfile_directory,DIRSEP);
      // get last directory separator
    if(sp!=NULL) *sp= 0;  // if found any, cut the string here
    #if __WIN32__
      mkdir(tempfile_directory);
    #else
      mkdir(tempfile_directory,0700);
    #endif
    }

  // get file timestamp of OSM input file
  if(old_timestamp==0) {  // no timestamp given by the user
    if(old_file==NULL) {  // no file name given for the old OSM file
      PERR("Specify at least the old OSM file's name or its timestamp.")
return 1;
      }
    if(!file_exists(old_file)) {  // old OSM file does not exist
      PERRv("Old OSM file does not exist: %.80s",old_file);
return 1;
      }
    old_timestamp= get_file_timestamp(old_file);
    if(old_timestamp==0) {
      PERRv("Old OSM file does not contain a timestamp: %.80s",old_file);
      PERR("Please specify the timestamp manually, e.g.: "
        "2011-07-15T23:30:00Z");
return 1;
      }
    }  // end   no timestamp given by the user

  // parameter consistency check
  if(new_file==NULL) {
    PERR("No output file was specified.");
return 1;
    }
  if(old_file!=NULL && strcmp(old_file,new_file)==0) {
    PERR("Input file and output file are identical.");
return 1;
    }
  if(old_file==NULL && !new_file_is_changefile) {
    PERR("If no old OSM file is specified, osmupdate can only "
      "generate a changefile.");
return 1;
    }

  // initialize sequence numbers and timestamps
  minutely_sequence_number= hourly_sequence_number=
    daily_sequence_number= sporadic_sequence_number= 0;
  minutely_timestamp= hourly_timestamp=
    daily_timestamp= sporadic_timestamp= 0;

  // care about user defined processing categories
  if(process_minutely || process_hourly ||
      process_daily || process_sporadic) {
      // user wants specific type(s) of chancefiles to be processed
    if(!process_minutely) no_minutely= true;
    if(!process_hourly) no_hourly= true;
    if(!process_daily) no_daily= true;
    }
  else {
    // try to get sporadic timestamp
    sporadic_timestamp= get_newest_changefile_timestamp(
      cft_SPORADIC,&sporadic_sequence_number);
    if(sporadic_timestamp!=0) {
        // there is a timestamp at the highest directory level,
        // this must be a so-called sporadic timestamp
      if(loglevel>0) {
        PINFO("Found status information in base URL root.")
        PINFO("Ignoring subdirectories \"minute\", \"hour\", \"day\".")
        }
      process_sporadic= true;  // let's take it
      no_minutely= no_hourly= no_daily= true;
      }
    }

  // get last timestamp for each, minutely, hourly, daily,
  // and sporadic diff files
  if(!no_minutely) {
    minutely_timestamp= get_newest_changefile_timestamp(
      cft_MINUTELY,&minutely_sequence_number);
    if(minutely_timestamp==0) {
      PERR("Could not get the newest minutely timestamp from the Internet.")
return 1;
      }
    }
  if(!no_hourly) {
    hourly_timestamp= get_newest_changefile_timestamp(
      cft_HOURLY,&hourly_sequence_number);
    if(hourly_timestamp==0) {
      PERR("Could not get the newest hourly timestamp from the Internet.")
  return 1;
      }
    }
  if(!no_daily) {
    daily_timestamp= get_newest_changefile_timestamp(
      cft_DAILY,&daily_sequence_number);
    if(daily_timestamp==0) {
      PERR("Could not get the newest daily timestamp from the Internet.")
  return 1;
      }
    }
  if(process_sporadic && sporadic_timestamp==0) {
    sporadic_timestamp= get_newest_changefile_timestamp(
      cft_SPORADIC,&sporadic_sequence_number);
    if(sporadic_timestamp==0) {
      PERR("Could not get the newest sporadic timestamp "
        "from the Internet.")
  return 1;
      }
    }

  // check maximum update range
  if(minutely_timestamp-old_timestamp>max_update_range) {
      // update range too large
    int days;
    days= (int)((minutely_timestamp-old_timestamp+86399)/86400);
    PERRv("Update range too large: %i days.",days)
    PINFOv("To allow such a wide range, add: --max-days=%i",days)
return 1;
    }  // update range too large

  // clear last hourly timestamp if
  // OSM old file's timestamp > latest hourly timestamp - 30 minutes
  if(old_timestamp>hourly_timestamp-30*60 && !no_minutely)
    hourly_timestamp= 0;  // (let's take minutely updates)

  // clear last daily timestamp if
  // OSM file timestamp > latest daily timestamp - 16 hours
  if(old_timestamp>daily_timestamp-16*3600 &&
      !(no_hourly && no_minutely))
    daily_timestamp= 0;  // (let's take hourly and minutely updates)

  // initialize start timestamp
  timestamp= 0;
  if(timestamp<minutely_timestamp) timestamp= minutely_timestamp;
  if(timestamp<hourly_timestamp) timestamp= hourly_timestamp;
  if(timestamp<daily_timestamp) timestamp= daily_timestamp;
  if(timestamp<sporadic_timestamp) timestamp= sporadic_timestamp;

  // get and process minutely diff files from last minutely timestamp
  // backward; stop just before latest hourly timestamp or OSM
  // file timestamp has been reached;
  if(minutely_timestamp!=0) {
    next_timestamp= timestamp;
    while(next_timestamp>hourly_timestamp &&
        next_timestamp>old_timestamp) {
      timestamp= next_timestamp;
      process_changefile(cft_MINUTELY,minutely_sequence_number,timestamp);
      minutely_sequence_number--;
      next_timestamp= get_changefile_timestamp(
        cft_MINUTELY,minutely_sequence_number);
      }
    }

  // get and process hourly diff files from last hourly timestamp
  // backward; stop just before last daily timestamp or OSM
  // file timestamp has been reached;
  if(hourly_timestamp!=0) {
    next_timestamp= timestamp;
    while(next_timestamp>daily_timestamp &&
        next_timestamp>old_timestamp) {
      timestamp= next_timestamp;
      process_changefile(cft_HOURLY,hourly_sequence_number,timestamp);
      hourly_sequence_number--;
      next_timestamp= get_changefile_timestamp(
        cft_HOURLY,hourly_sequence_number);
      }
    }

  // get and process daily diff files from last daily timestamp
  // backward; stop just before OSM file timestamp has been reached;
  if(daily_timestamp!=0) {
    next_timestamp= timestamp;
    while(next_timestamp>old_timestamp) {
      timestamp= next_timestamp;
      process_changefile(cft_DAILY,daily_sequence_number,timestamp);
      daily_sequence_number--;
      next_timestamp= get_changefile_timestamp(
        cft_DAILY,daily_sequence_number);
      }
    }

  // get and process sporadic diff files from last sporadic timestamp
  // backward; stop just before OSM file timestamp has been reached;
  if(sporadic_timestamp!=0) {
    next_timestamp= timestamp;
    while(next_timestamp>old_timestamp) {
      timestamp= next_timestamp;
      process_changefile(
        cft_SPORADIC,sporadic_sequence_number,timestamp);
      sporadic_sequence_number--;
      next_timestamp= get_changefile_timestamp(
        cft_SPORADIC,sporadic_sequence_number);
      }
    }

  // process remaining files which may still wait in the cache;
  process_changefile(0,0,0);

  /* create requested output file */ {
    char master_cachefile_name[400];
    char command[2000],*command_p;
    char* command_e= command+sizeof(command);
    char result[1000];

    if(loglevel>0)
      PINFO("Creating output file.")
    strcpy(stpmcpy(master_cachefile_name,global_tempfile_name,
      sizeof(master_cachefile_name)-5),".8");
    if(!file_exists(master_cachefile_name)) {
      if(old_file==NULL)
        PINFO("There is no changefile since this timestamp.")
      else 
        PINFO("Your OSM file is already up-to-date.")
return 21;
      }
    command_p= command;
    if(new_file_is_changefile) {  // changefile
      if(new_file_is_gz) {  // compressed
        if(new_file_is_o5) {  // .o5c.gz
          stecpy(&command_p,command_e,"gzip ");
          stecpy(&command_p,command_e,global_gzip_parameters);
          stecpy(&command_p,command_e," <\"");
          steesccpy(&command_p,command_e,master_cachefile_name);
          stecpy(&command_p,command_e,"\" >\"");
          steesccpy(&command_p,command_e,new_file);
          stecpy(&command_p,command_e,"\"");
          }
        else {  // .osc.gz
          stecpy(&command_p,command_e,global_osmconvert_program);
          stecpy(&command_p,command_e," ");
          stecpy(&command_p,command_e,global_osmconvert_arguments);
          stecpy(&command_p,command_e," \"");
          steesccpy(&command_p,command_e,master_cachefile_name);
          stecpy(&command_p,command_e,"\" --out-osc |gzip ");
          stecpy(&command_p,command_e,global_gzip_parameters);
          stecpy(&command_p,command_e," >\"");
          steesccpy(&command_p,command_e,new_file);
          stecpy(&command_p,command_e,"\"");
          }
        shell_command(command,result);
        }  // compressed
      else {  // uncompressed
        if(new_file_is_o5)  // .o5c
          rename(master_cachefile_name,new_file);
        else {  // .osc
          stecpy(&command_p,command_e,global_osmconvert_program);
          stecpy(&command_p,command_e," ");
          stecpy(&command_p,command_e,global_osmconvert_arguments);
          stecpy(&command_p,command_e," \"");
          steesccpy(&command_p,command_e,master_cachefile_name);
          stecpy(&command_p,command_e,"\" --out-osc >\"");
          steesccpy(&command_p,command_e,new_file);
          stecpy(&command_p,command_e,"\"");
          shell_command(command,result);
          }
        }  // uncompressed
      }  // changefile
    else {  // OSM file
      #if 0
      if(loglevel>=2) {
        PINFOv("oc %s",global_osmconvert_program)
        PINFOv("fa %s",final_osmconvert_arguments)
        PINFOv("of %s",old_file)
        PINFOv("mc %s",master_cachefile_name)
        }
      #endif
      stecpy(&command_p,command_e,global_osmconvert_program);
      stecpy(&command_p,command_e," ");
      stecpy(&command_p,command_e,final_osmconvert_arguments);
      stecpy(&command_p,command_e," \"");
      steesccpy(&command_p,command_e,old_file);
      stecpy(&command_p,command_e,"\" \"");
      steesccpy(&command_p,command_e,master_cachefile_name);
      if(new_file_is_gz) {  // compressed
        if(new_file_is_o5) {  // .o5m.gz
          stecpy(&command_p,command_e,"\" --out-o5m |gzip ");
          stecpy(&command_p,command_e,global_gzip_parameters);
          stecpy(&command_p,command_e," >\"");
          steesccpy(&command_p,command_e,new_file);
          stecpy(&command_p,command_e,"\"");
          }
        else {  // .osm.gz
          stecpy(&command_p,command_e,"\" --out-osm |gzip ");
          stecpy(&command_p,command_e,global_gzip_parameters);
          stecpy(&command_p,command_e," >\"");
          steesccpy(&command_p,command_e,new_file);
          stecpy(&command_p,command_e,"\"");
          }
        }  // compressed
      else {  // uncompressed
        if(new_file_is_pbf) {  // .pbf
          stecpy(&command_p,command_e,"\" --out-pbf >\"");
          steesccpy(&command_p,command_e,new_file);
          stecpy(&command_p,command_e,"\"");
          }
        else if(new_file_is_o5) {  // .o5m
          stecpy(&command_p,command_e,"\" --out-o5m >\"");
          steesccpy(&command_p,command_e,new_file);
          stecpy(&command_p,command_e,"\"");
          }
        else {  // .osm
          stecpy(&command_p,command_e,"\" --out-osm >\"");
          steesccpy(&command_p,command_e,new_file);
          stecpy(&command_p,command_e,"\"");
          }
        }  // uncompressed
      shell_command(command,result);
      }  // OSM file
    if(loglevel<2)
      unlink(master_cachefile_name);
    }  // create requested output file 

  // delete tempfiles
  if(global_keep_tempfiles) {  // tempfiles shall be kept
    if(loglevel>0)
      PINFO("Keeping temporary files.")
    }  // tempfiles shall be kept
  else {  // tempfiles shall be deleted
    char command[500],*command_p,result[1000];
    char* command_e= command+sizeof(command);

    if(loglevel>0)
      PINFO("Deleting temporary files.")
    command_p= command;
    stecpy(&command_p,command_e,DELFILE" \"");
    steesccpy(&command_p,command_e,global_tempfile_name);
    stecpy(&command_p,command_e,"\".*");
    shell_command(command,result);
    rmdir(tempfile_directory);
    }  // tempfiles shall be deleted

  if(main_return_value==0 && loglevel>0)
    PINFO("Completed successfully.")

  return main_return_value;
  }  // end   main()


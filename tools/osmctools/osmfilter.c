// osmfilter 2015-04-14 19:50
#define VERSION "1.4.0"
//
// compile this file:
// gcc osmfilter.c -O3 -o osmfilter
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
// Other licenses are available on request; please ask the author.

#define MAXLOGLEVEL 2
const char* shorthelptext=
"\nosmfilter " VERSION "  Parameter Overview\n"
"(Please use  --help  to get more information.)\n"
"\n"
"<file>                    file to filter; (.o5m faster than .osm)\n"
"--keep=                   define which objects are to be kept\n"
"--keep-nodes=             same as above, but applies to nodes only,\n"
"--keep-ways=              etc.\n"
"--keep-relations=              Examples:\n"
"--keep-nodes-ways=             --keep=\"amenity=pub =bar\"\n"
"--keep-nodes-relations=        --keep=\"tunnel=yes and lit=yes\"\n"
"--keep-ways-relations=\n"
"--drop=                   define which objects are to be dropped\n"
"--drop-...(see above)=    similar to --keep-...= (see above)\n"
"--keep-tags=              define which tags are to be kept\n"
"--keep-node-tags=         same as above, but applies to nodes only,\n"
"--keep-way-tags=          etc.\n"
"--keep-relation-tags=\n"
"--keep-node-way-tags=\n"
"--keep-node-relation-tags=\n"
"--keep-way-relation-tags=\n"
"--drop-tags=              define which tags are to be dropped\n"
"--drop-...-tags=          similar to --keep-...-tags= (see above)\n"
"--drop-author             delete changeset and user information\n"
"--drop-version            same as before, but delete version as well\n"
"--drop-nodes              delete all nodes\n"
"--drop-ways               delete all ways\n"
"--drop-relations          delete all relations\n"
"--emulate-osmosis         emulate Osmosis XML output format\n"
"--emulate-pbf2osm         emulate pbf2osm output format\n"
"--fake-author             set changeset to 1 and timestamp to 1970\n"
"--fake-version            set version number to 1\n"
"--fake-lonlat             set lon to 0 and lat to 0\n"
"-h                        display this parameter overview\n"
"--help                    display a more detailed help\n"
"--ignore-dependencies     ignore dependencies between OSM objects\n"
"--out-key=                write statistics (for the key, if supplied)\n"
"--out-count=              same as before, but sorted by occurrence\n"
"--out-osm                 write output in .osm format (default)\n"
"--out-osc                 write output in .osc format (OSMChangefile)\n"
"--out-osh                 write output in .osh format (visible-tags)\n"
"--out-o5m                 write output in .o5m format (fast binary)\n"
"--out-o5c                 write output in .o5c format (bin. Changef.)\n"
"-o=<outfile>              reroute standard output to a file\n"
"-t=<tempfile>             define tempfile prefix\n"
"--parameter-file=<file>   param. in file, separated by empty lines\n"
"--verbose                 activate verbose mode\n";
const char* helptext=
"\nosmfilter " VERSION "\n"
"\n"
"THIS PROGRAM IS FOR EXPERIMENTAL USE ONLY.\n"
"PLEASE EXPECT MALFUNCTION AND DATA LOSS.\n"
"SAVE YOUR DATA BEFORE STARTING THIS PROGRAM.\n"
"\n"
"This program filters OpenStreetMap data.\n"
"\n"
"The input file name must be supplied as command line argument. The\n"
"file must not be a stream. Redirections from standard input will not\n"
"work because the program needs random access to the file. You do not\n"
"need to specify the input format, osmfilter will recognize these\n"
"formats: .osm (XML), .osc (OSM Change File), .osh (OSM Full History),\n"
".o5m (speed-optimized) and .o5c (speed-optimized Change File).\n"
"\n"
"The output format is .osm by default. If you want a different format,\n"
"please specify it using the appropriate command line parameter.\n"
"\n"
"--keep=OBJECT_FILTER\n"
"        All object types (nodes, ways and relations) will be kept\n"
"        if they meet the filter criteria. Same applies to dependent\n"
"        objects, e.g. nodes in ways, ways in relations, relations in\n"
"        other relations.\n"
"        Please look below for a syntax description of OBJECT_FILTER.\n"
"\n"
"--keep-nodes=OBJECT_FILTER\n"
"--keep-ways=OBJECT_FILTER\n"
"--keep-relations=OBJECT_FILTER\n"
"--keep-nodes-ways=OBJECT_FILTER\n"
"--keep-nodes-relations=OBJECT_FILTER\n"
"--keep-ways-relations=OBJECT_FILTER\n"
"        Same as above, but just for the specified object types.\n"
"\n"
"--drop=OBJECT_FILTER\n"
"        All object types (nodes, ways and relations) which meet the\n"
"        supplied filter criteria will be dropped, regardless of\n"
"        meeting the criteria of a keep filter (see above).\n"
"        Please look below for a syntax description of OBJECT_FILTER.\n"
"\n"
"--drop-nodes=OBJECT_FILTER\n"
"--drop-ways=OBJECT_FILTER\n"
"--drop-relations=OBJECT_FILTER\n"
"--drop-nodes-ways=OBJECT_FILTER\n"
"--drop-nodes-relations=OBJECT_FILTER\n"
"--drop-ways-relations=OBJECT_FILTER\n"
"        Same as above, but just for the specified object types.\n"
"\n"
"--keep-tags=TAG_FILTER\n"
"        The in TAG_FILTER specified tags will be allowed on output.\n"
"        Please look below for a syntax description of TAG_FILTER.\n"
"\n"
"--keep-node-tags=TAG_FILTER\n"
"--keep-way-tags=TAG_FILTER\n"
"--keep-relation-tags=TAG_FILTER\n"
"--keep-node-way-tags=TAG_FILTER\n"
"--keep-node-relation-tags=TAG_FILTER\n"
"--keep-way-relation-tags=TAG_FILTER\n"
"        Same as above, but just for the specified object types.\n"
"\n"
"--drop-tags=TAG_FILTER\n"
"        The specified tags will be dropped. This overrules the\n"
"        previously described parameter --keep-tags.\n"
"        Please look below for a syntax description of TAG_FILTER.\n"
"\n"
"--drop-node-tags=TAG_FILTER\n"
"--drop-way-tags=TAG_FILTER\n"
"--drop-relation-tags=TAG_FILTER\n"
"--drop-node-way-tags=TAG_FILTER\n"
"--drop-node-relation-tags=TAG_FILTER\n"
"--drop-way-relation-tags=TAG_FILTER\n"
"        Same as above, but just for the specified object types.\n"
"\n"
"--drop-author\n"
"        For most applications the author tags are not needed. If you\n"
"        specify this option, no author information will be written:\n"
"        no changeset, user or timestamp.\n"
"\n"
"--drop-version\n"
"        If you want to exclude not only the author information but\n"
"        also the version number, specify this option.\n"
"\n"
"--drop-nodes\n"
"--drop-ways\n"
"--drop-relations\n"
"        According to the combination of these parameters, no members\n"
"        of the referred section will be written.\n"
  "\n"
"--emulate-osmosis\n"
"--emulate-pbf2osm\n"
"        In case of .osm output format, the program will try to use\n"
"        the same data syntax as Osmosis, resp. pbf2osm.\n"
"\n"
"--fake-author\n"
"        If you have dropped author information (--drop-author) that\n"
"        data will be lost, of course. Some programs however require\n"
"        author information on input although they do not need that\n"
"        data. For this purpose, you can fake the author information.\n"
"        o5mfiler will write changeset 1, timestamp 1970.\n"
"\n"
"--fake-version\n"
"        Same as --fake-author, but - if .osm xml is used as output\n"
"        format - only the version number will be written (version 1).\n"
"        This is useful if you want to inspect the data with JOSM.\n"
"\n"
"--fake-lonlat\n"
"        Some programs depend on getting longitude/latitude values,\n"
"        even when the object in question shall be deleted. With this\n"
"        option you can have osmfilter to fake these values:\n"
"           ... lat=\"0\" lon=\"0\" ...\n"
"        Note that this is for XML files only (.osc and .osh).\n"
"\n"
"-h\n"
"        Display a short parameter overview.\n"
"\n"
"--help\n"
"        Display this help.\n"
"\n"
"--ignore-dependencies\n"
"        Usually, all member nodes of a way which meets the filter\n"
"        criteria will be included as well. Same applies to members of\n"
"        included relations. If you activate this option, all these\n"
"        dependencies between OSM objects will be ignored.\n"
"\n"
"--out-key=KEYNAME\n"
"        The output will contain no regular OSM data but only\n"
"        statistics: a list of all used keys is assembled. Left to\n"
"        each key, the number of occurrences is printed.\n"
"        If KEYNAME is given, the program will list all values which\n"
"        are used in connections with this key.\n"
"        You may use wildcard characters for KEYNAME, but only at the\n"
"        beginning and/or at the end. For example:  --out-key=addr:*\n"
"\n"
"--out-count=KEYNAME\n"
"        Same as --out-key=, but the list is sorted by the number of\n"
"        occurrences of the keys resp. values.\n"
"\n"
"--out-osm\n"
"        Data will be written in .osm format. This is the default\n"
"        output format.\n"
"\n"
"--out-osc\n"
"        The OSM Change format will be used for output. Please note\n"
"        that OSM objects which are to be deleted are represented by\n"
"        their ids only.\n"
"\n"
"--out-osh\n"
"        For every OSM object, the appropriate \'visible\' tag will be\n" "        added to meet \'full planet history\' specification.\n"
"\n"
"--out-o5m\n"
"        The .o5m format will be used. This format has the same\n"
"        structure as the conventional .osm format, but the data are\n"
"        stored as binary numbers and are therefore much more compact\n"
"        than in .osm format. No packing is used, so you can pack .o5m\n"
"        files using every file packer you want, e.g. lzo, bz2, etc.\n"
"\n"
"--out-o5c\n"
"        This is the change file format of .o5m data format. All\n"
"        <delete> tags will not be performed as delete actions but\n"
"        converted into .o5c data format.\n"
"\n"
"-o=<outfile>\n"
"        Standard output will be rerouted to the specified file.\n"
"        If no output format has been specified, the program will\n"
"        proceed according to the file name extension.\n"
"\n"
"-t=<tempfile>\n"
"        osmfilter uses a temporary file to process interrelational\n"
"        dependencies. This parameter defines the name prefix. The\n"
"        default value is \"osmfilter_tempfile\".\n"
"\n"
"--parameter-file=FILE\n"
"        If you want to supply one ore more command line arguments\n"
"        by a parameter file, please use this option and specify the\n"
"        file name. Within the parameter file, parameters must be\n"
"        separated by empty lines. Line feeds inside a parameter will\n"
"        be converted to spaces.\n"
"        Lines starting with \"// \" will be treated as comments.\n"
"\n"
"-v\n"
"--verbose\n"
"        With activated \'verbose\' mode, some statistical data and\n"
"        diagnosis data will be displayed.\n"
"        If -v resp. --verbose is the first parameter in the line,\n"
"        osmfilter will display all input parameters.\n"
"\n"
"OBJECT_FILTER\n"
"        Some of the command line arguments need a filter to be\n"
"        specified. This filter definition consists of key/val pairs\n"
"        and uses the following syntax:\n"
"          \"KEY1=VAL1 OP KEY2=VAL2 OP KEY3=VAL3 ...\"\n"
"        OP is the Boolean operator, it must be either \"and\" or \"or\".\n"
"        As usual, \"and\" will be processed prior to \"or\". If you\n"
"        want to influence the sequence of processing, you may use\n"
"        brackets to do so. Please note that brackets always must be\n"
"        padded by spaces. Example: lit=yes and ( note=a or source=b )\n"
"        Instead of each \"=\" you may enter one of these comparison\n"
"        operators: != (not equal), <, >, <=, >=\n"
"        The program will use ASCII-alphabetic comparison unless you\n" "        compare against a value which is starting with a digit.\n"
"        If there are different possible values for the same key, you\n"
"        need to write the key only once. For example:\n"
"          \"amenity=restaurant =pub =bar\"\n"
"        It is allowed to omit the value. In this case, the program\n"
"        will accept every value for the defined key. For example:\n"
"          \"all highway= lit=yes\"\n"
"        You may use wildcard characters for key or value, but only at\n"
"        the beginning and/or at the end. For example:\n"
"          wikipedia:*=  highway=*ary  ref_name=*central*\n"
"        Please be careful with wildcards in keys since only the first\n"
"        key which meets the pattern will be processed.\n"
"        There are three special keys which represent object id, user\n"
"        id and user name: @id, @uid and @user. They allow you to\n"
"        search for certain objects or for edits of specific users.\n"
"\n"
"TAG_FILTER\n"
"        The tag filter determines which tags will be kept and which\n"
"        will be not. The example\n"
"          --keep-tags=\"highway=motorway =primary\"\n"
"        will not accept \"highway\" tags other than \"motorway\" or\n"
"        \"primary\". Note that neither the object itself will be\n"
"        deleted, nor the remaining tags. If you want to drop every\n"
"        tag which is not mentioned in a list, use this example:\n"
"          all highway= amenity= name=\n"
"\n"
"Examples\n"
"\n"
"./osmfilter europe.o5m --keep=amenity=bar -o=new.o5m\n"
"./osmfilter a.osm --keep-nodes=lit=yes --drop-ways -o=light.osm\n"
"./osmfilter a.osm --keep=\"\n"
"    place=city or ( place=town and population>=10000 )\" -o=b.osm\n"
"./osmfilter region.o5m --keep=\"bridge=yes and layer>=2\" -o=r.o5m\n"
"\n"
"Tuning\n"
"\n"
"To speed-up the process, the program uses some main memory for a\n"
"hash table. By default, it uses 900 MB for storing a flag for every\n"
"possible node, 90 for the way flags, and 10 relation flags.\n"
"Every byte holds the flags for 8 ID numbers, i.e., in 900 MB the\n"
"program can store 7200 million flags. As there are less than 3200\n"
"million IDs for nodes at present (Oct 2014), 400 MB would suffice.\n"
"So, for example, you can decrease the hash sizes to e.g. 400, 50 and\n"
"2 MB (for relations, 2 flags are needed each) using this option:\n"
"\n"
"  --hash-memory=400-50-2\n"
"\n"
"But keep in mind that the OSM database is continuously expanding. For\n"
"this reason the program-own default value is higher than shown in the\n"
"example, and it may be appropriate to increase it in the future.\n"
"If you do not want to bother with the details, you can enter the\n"
"amount of memory as a sum, and the program will divide it by itself.\n"
"For example:\n"
"\n"
"  --hash-memory=1500\n"
"\n"
"These 1500 MB will be split in three parts: 1350 for nodes, 135 for\n"
"ways, and 15 for relations.\n"
"\n"
"Because we are taking hashes, it is not necessary to provide all the\n"
"suggested memory; the program will operate with less hash memory too.\n"
"But, in this case, the border filter will be less effective, i.e.,\n"
"some ways and some relations will be left in the output file although\n"
"they should have been excluded.\n"
"The maximum value the program accepts for the hash size is 4000 MiB;\n"
"If you exceed the maximum amount of memory available on your system,\n"
"the program will try to reduce this amount and display a warning\n"
"message.\n"
"\n"
"Limitations\n"
"\n"
"When filtering whole OSM objects (--keep...=, --drop...=), the input\n"
"file must contain the objects ordered by their type: first, all nodes\n"
"nodes, next, all ways, followed by all relations.\n"
"\n"
"Usual .osm, .osc, .o5m and o5c files adhere to this condition. This\n"
"means that you do not have to worry about this limitation. osmfilter\n"
"will display an error message if this sequence is broken.\n"
"\n"
"The number of key/val pairs in each filter parameter is limited to\n"
"1000, the length of each key or val is limited to 100.\n"
"\n"
"There is NO WARRANTY, to the extent permitted by law.\n"
"Please send any bug reports to markus.weber@gmx.com\n\n";

#define _FILE_OFFSET_BITS 64
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

typedef enum {false= 0,true= 1} bool;
typedef uint8_t byte;
typedef unsigned int uint;
#define isdig(x) isdigit((unsigned char)(x))
static byte isdigi_tab[]= {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#define isdigi(c) (isdigi_tab[(c)])  // digit
static byte digival_tab[]= {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  1,2,3,4,5,6,7,8,9,10,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#define digival(c) (digival_tab[(c)])
  // value of a digit, starting with 1, for comparisons only

static int loglevel= 0;  // logging to stderr;
  // 0: no logging; 1: small logging; 2: normal logging;
  // 3: extended logging;
#define UR(x) if(x){}  // result value intentionally ignored
#define DP(f) fprintf(stderr,"- Debug: " #f "\n");
#define DPv(f,...) fprintf(stderr,"- Debug: " #f "\n",__VA_ARGS__);
#if __WIN32__
  #define NL "\r\n"  // use CR/LF as new-line sequence
  #define off_t off64_t
  #define lseek lseek64
#else
  #define NL "\n"  // use LF as new-line sequence
  #define O_BINARY 0
#endif



//------------------------------------------------------------
// Module Global   global variables for this program
//------------------------------------------------------------

// to distinguish global variable from local or module global
// variables, they are preceded by 'global_';

static bool global_dropversion= false;  // exclude version
static bool global_dropauthor= false;  // exclude author information
static bool global_fakeauthor= false;  // fake author information
static bool global_fakeversion= false;  // fake just the version number
static bool global_fakelonlat= false;
  // fake longitude and latitude in case of delete actions (.osc);
static bool global_dropnodes= false;  // exclude nodes section
static bool global_dropways= false;  // exclude ways section
static bool global_droprelations= false;  // exclude relations section
static bool global_outo5m= false;  // output shall have .o5m format
static bool global_outo5c= false;  // output shall have .o5c format
static bool global_outosm= false;  // output shall have .osm format
static bool global_outosc= false;  // output shall have .osc format
static bool global_outosh= false;  // output shall have .osh format
static const char* global_outkey= NULL;
  // =="": do not write osm data, write a list of keys instead;
  // !=NULL && !="": write a list of vals to the key this variable
  //                 points to;
static bool global_outsort= false;  // sort item list by count;
static bool global_emulatepbf2osm= false;
  // emulate pbf2osm compatible output
static bool global_emulateosmosis= false;
  // emulate Osmosis compatible output
static bool global_emulateosmium= false;
  // emulate Osmium compatible output
static char global_tempfilename[350]= "osmfilter_tempfile";
  // prefix of names for temporary files
static bool global_recursive= false;  // recursive processing necessary
static bool global_ignoredependencies= false;
  // user wants interobject dependencies to be ignored
#define PERR(f) { static int msgn= 3; if(--msgn>=0) \
  fprintf(stderr,"osmfilter Error: " f "\n"); }
  // print error message
#define PERRv(f,...) { static int msgn= 3; if(--msgn>=0) \
  fprintf(stderr,"osmfilter Error: " f "\n",__VA_ARGS__); }
  // print error message with value(s)
#define WARN(f) { static int msgn= 3; if(--msgn>=0) \
  fprintf(stderr,"osmfilter Warning: " f "\n"); }
  // print a warning message, do it maximal 3 times
#define WARNv(f,...) { static int msgn= 3; if(--msgn>=0) \
  fprintf(stderr,"osmfilter Warning: " f "\n",__VA_ARGS__); }
  // print a warning message with value(s), do it maximal 3 times
#define PINFO(f) \
  fprintf(stderr,"osmfilter: " f "\n"); // print info message
#define PINFOv(f,...) \
  fprintf(stderr,"osmfilter: " f "\n",__VA_ARGS__);
#define ONAME(i) \
  (i==0? "node": i==1? "way": i==2? "relation": "unknown object")
#define global_fileM 1  // maximum number of input files

//------------------------------------------------------------
// end   Module Global   global variables for this program
//------------------------------------------------------------



static inline char* int32toa(int32_t v,char* s) {
  // convert int32_t integer into string;
  // v: long integer value to convert;
  // return: s;
  // s[]: digit string;
  char* s1,*s2;
  char c;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  else if(v==0)
    *s1++= '0';
  s2= s1;
  while(v>0)
    { *s2++= "0123456789"[v%10]; v/= 10; }
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  return s;
  }  // end   int32toa()

static inline char* uint32toa(uint32_t v,char* s) {
  // convert uint32_t integer into string;
  // v: long integer value to convert;
  // return: s;
  // s[]: digit string;
  char* s1,*s2;
  char c;

  s1= s;
  if(v==0)
    *s1++= '0';
  s2= s1;
  while(v>0)
    { *s2++= "0123456789"[v%10]; v/= 10; }
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  return s;
  }  // end   uint32toa()

static inline char* int64toa(int64_t v,char* s) {
  // convert int64_t integer into string;
  // v: long integer value to convert;
  // return: s;
  // s[]: digit string;
  char* s1,*s2;
  char c;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  else if(v==0)
    *s1++= '0';
  s2= s1;
  while(v>0)
    { *s2++= "0123456789"[v%10]; v/= 10; }
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  return s;
  }  // end   int64toa()

static inline char *stpcpy0(char *dest, const char *src) {
  // redefinition of C99's stpcpy() because it's missing in MinGW,
  // and declaration in Linux seems to be wrong;
  while(*src!=0)
    *dest++= *src++;
  *dest= 0;
  return dest;
  }  // end stpcpy0()

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

static char *stpmcpy(char *dest, const char *src, size_t maxlen) {
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

static inline int strzlcmp(const char* s1,const char* s2) {
  // similar to strzcmp(), this procedure compares two character strings;
  // and accepts the first string to be longer than the second;
  // other than strzcmp(), this procedure returns the length of s2[] in
  // case both string contents are identical, and returns 0 otherwise;
  // s1[]: first string;
  // s2[]: string to compare with the first string;
  // return:
  // >0: both strings are identical, the length of the second string is
  //     returned; the first string may be longer than the second;
  // 0: the string contents are not identical;
  const char* s2a;

  s2a= s2;
  while(*s1==*s2 && *s1!=0) { s1++; s2++; }
  if(*s2==0)
    return s2-s2a;
  return 0;
  }  // end   strzlcmp()

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



//------------------------------------------------------------
// Module pbf_   protobuf conversions module
//------------------------------------------------------------

// this module provides procedures for conversions from
// protobuf formats to regular numbers;
// as usual, all identifiers of a module have the same prefix,
// in this case 'pbf'; one underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----
// many procedures have a parameter 'pp'; here, the address of
// a buffer pointer is expected; this pointer will be incremented
// by the number of bytes the converted protobuf element consumes;

//------------------------------------------------------------

static inline uint32_t pbf_uint32(byte** pp) {
  // get the value of an unsigned integer;
  // pp: see module header;
  byte* p;
  uint32_t i;
  uint32_t fac;

  p= *pp;
  i= *p;
  if((*p & 0x80)==0) {  // just one byte
    (*pp)++;
return i;
    }
  i&= 0x7f;
  fac= 0x80;
  while(*++p & 0x80) {  // more byte(s) will follow
    i+= (*p & 0x7f)*fac;
    fac<<= 7;
    }
  i+= *p++ *fac;
  *pp= p;
  return i;
  }  // end   pbf_uint32()

static inline int32_t pbf_sint32(byte** pp) {
  // get the value of an unsigned integer;
  // pp: see module header;
  byte* p;
  int32_t i;
  int32_t fac;
  int sig;

  p= *pp;
  i= *p;
  if((*p & 0x80)==0) {  // just one byte
    (*pp)++;
    if(i & 1)  // negative
return -1-(i>>1);
    else
return i>>1;
    }
  sig= i & 1;
  i= (i & 0x7e)>>1;
  fac= 0x40;
  while(*++p & 0x80) {  // more byte(s) will follow
    i+= (*p & 0x7f)*fac;
    fac<<= 7;
    }
  i+= *p++ *fac;
  *pp= p;
    if(sig)  // negative
return -1-i;
    else
return i;
  }  // end   pbf_sint32()

static inline uint64_t pbf_uint64(byte** pp) {
  // get the value of an unsigned integer;
  // pp: see module header;
  byte* p;
  uint64_t i;
  uint64_t fac;

  p= *pp;
  i= *p;
  if((*p & 0x80)==0) {  // just one byte
    (*pp)++;
return i;
    }
  i&= 0x7f;
  fac= 0x80;
  while(*++p & 0x80) {  // more byte(s) will follow
    i+= (*p & 0x7f)*fac;
    fac<<= 7;
    }
  i+= *p++ *fac;
  *pp= p;
  return i;
  }  // end   pbf_uint64()

static inline int64_t pbf_sint64(byte** pp) {
  // get the value of a signed integer;
  // pp: see module header;
  byte* p;
  int64_t i;
  int64_t fac;
  int sig;

  p= *pp;
  i= *p;
  if((*p & 0x80)==0) {  // just one byte
    (*pp)++;
    if(i & 1)  // negative
return -1-(i>>1);
    else
return i>>1;
    }
  sig= i & 1;
  i= (i & 0x7e)>>1;
  fac= 0x40;
  while(*++p & 0x80) {  // more byte(s) will follow
    i+= (*p & 0x7f)*fac;
    fac<<= 7;
    }
  i+= *p++ *fac;
  *pp= p;
    if(sig)  // negative
return -1-i;
    else
return i;
  }  // end   pbf_sint64()

#if 0  // not used at present
static inline void pbf_intjump(byte** pp) {
  // jump over a protobuf formatted integer;
  // pp: see module header;
  // we do not care about a possibly existing identifier,
  // therefore as the start address *pp the address of the
  // integer value is expected;
  byte* p;

  p= *pp;
  while(*p & 0x80) p++; p++;
  *pp= p;
  }  // end   pbf_intjump()
#endif

//------------------------------------------------------------
// end   Module pbf_   protobuf conversions module
//------------------------------------------------------------



//------------------------------------------------------------
// Module hash_   OSM hash module
//------------------------------------------------------------

// this module provides three hash tables with default sizes
// of 320, 60 and 20 MB;
// the procedures hash_seti() and hash_geti() allow bitwise
// access to these tables;
// as usual, all identifiers of a module have the same prefix,
// in this case 'hash'; one underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static bool hash__initialized= false;
#define hash__M 4
static unsigned char* hash__mem[hash__M]= {NULL,NULL,NULL,NULL};
  // start of the hash fields for each object type (node, way, relation);
static uint32_t hash__max[hash__M]= {0,0,0,0};
  // size of the hash fields for each object type
  // (node, way, positive relation, negative relation);
static int hash__errornumber= 0;
  // 1: object too large

static void hash__end() {
  // clean-up for hash module;
  // will be called at program's end;
  int o;  // object type

  for(o= 0;o<hash__M;o++) {
    hash__max[o]= 0;
    if(hash__mem[o]!=NULL) {
      free(hash__mem[o]); hash__mem[o]= NULL; }
    }
  hash__initialized= false;
  }  // end   hash__end()

//------------------------------------------------------------

static int hash_ini(int n,int w,int r) {
  // initializes the hash module;
  // n: amount of memory which is to be allocated for nodes;
  // w: amount of memory which is to be allocated for ways;
  // r: amount of memory which is to be allocated for relations;
  //    this will be divided for the two relation has fields:
  //    one field for positive relations and one for negative relations;
  // range for all input parameters: 1..4000, unit: MiB;
  // the second and any further call of this procedure will be ignored;
  // return: 0: initialization has been successful (enough memory);
  //         1: memory request had to been reduced to fit the system's
  //            resources (warning);
  //         2: memory request was unsuccessful (error);
  // general note concerning OSM database:
  // number of objects at Oct 2010: 950M nodes, 82M ways, 1.3M relations;
  // number of objects at May 2011: 1.3G nodes, 114M ways, 1.6M relations;
  int o;  // object type
  bool warning,error;

  warning= error= false;
  if(hash__initialized)  // already initialized
    return 0;  // ignore the call of this procedure
  // check parameters and store the values
  #define D(x,o) if(x<1) x= 1; else if(x>4000) x= 4000; \
    hash__max[o]= x*(1024*1024);
  D(n,0) D(w,1) D(r,2) D(r,3)
  #undef D
  // allocate memory for each hash table
  for(o= 0;o<hash__M;o++) {  // for each hash table
    do {
      hash__mem[o]= (unsigned char*)malloc(hash__max[o]);
      if(hash__mem[o]!=NULL) {  // allocation successful
        memset(hash__mem[o],0,hash__max[o]);  // clear all flags
    break;
        }
      // here: allocation unsuccessful
      // reduce amount by 50%
      hash__max[o]/=2;
      warning= true;
        // memorize that the user should be warned about this reduction
      // try to allocate the reduced amount of memory
      } while(hash__max[o]>=1024);
    if(hash__mem[o]==NULL)  // allocation unsuccessful at all
      error= true;  // memorize that the program should be aborted
    }  // end   for each hash table
  atexit(hash__end);  // chain-in the clean-up procedure
  if(!error) hash__initialized= true;
  return error? 2: warning? 1: 0;
  }  // end   hash_ini()

static void hash_seti(int o,int64_t idi) {
  // set a flag for a specific object type and ID;
  // o: object type; 0: node; 1: way; 2: relation;
  //    caution: due to performance reasons the boundaries
  //    are not checked;
  // id: id of the object;
  unsigned char* mem;  // address of byte in hash table
  unsigned int ido;  // bit offset to idi;

  if(!hash__initialized) return;  // ignore this call
  idi+= ((int64_t)hash__max[o])<<3;  // consider small negative numbers
  ido= idi&0x7;  // extract bit number (0..7)
  idi>>=3;  // calculate byte offset
  idi%= hash__max[o];  // consider length of hash table
  mem= hash__mem[o];  // get start address of hash table
  mem+= idi;  // calculate address of the byte
  *mem|= (1<<ido);  // set bit
  }  // end   hash_seti()

static bool hash_relseti(int64_t idi) {
  // set the status of a flag for a relation of a specific ID;
  // the flag is set only if this relations does not have a set flag
  // in the 'negative relations' hash field;
  // id: id of the object;
  // return: the flag has been set by this call of this procedure;
  // this procedure assumes that both, the hash field for positive
  // relations and the hash field for negative relations, have the
  // same size;
  unsigned char* mem;
  unsigned int ido;  // bit offset to idi;
  unsigned char bitmask;
  bool r;

  if(!hash__initialized) return true;  // ignore this call
  idi+= ((int64_t)hash__max[2])<<3;  // consider small negative numbers
  ido= idi&0x7;  // extract bit number (0..7)
  idi>>=3;  // calculate byte offset
  idi%= hash__max[2];  // consider length of hash table
  mem= hash__mem[3];  // get start address of negative hash table
  mem+= idi;  // calculate address of the byte
  if((*mem&(1<<ido))!=0)  // the relation's negative flag is set
return false;  // end processing here because we do not want to
    // set the relation's positive flag
  mem= hash__mem[2];  // get start address of positive hash table
  mem+= idi;  // calculate address of the byte
  bitmask= 1<<ido;  // determine the bitmask
  r= (*mem&bitmask)==0;  // the addressed bit has not been set until now
  *mem|= bitmask;  // set bit
  return r;
  }  // end   hash_relseti();

static bool hash_geti(int o,int64_t idi) {
  // get the status of a flag for a specific object type and ID;
  // (same as previous procedure, but id must be given as number);
  // o: object type; 0: node; 1: way; 2: relation;  caution:
  //    due to performance reasons the boundaries are not checked;
  // id: id of the object; the id is given as a string of decimal digits;
  //     a specific string terminator is not necessary, it is assumed
  //     that the id number ends with the first non-digit character;
  unsigned char* mem;
  unsigned int ido;  // bit offset to idi;
  bool flag;

  if(!hash__initialized) return false;
  idi+= ((int64_t)hash__max[o])<<3;  // consider small negative numbers
  ido= idi&0x7;  // extract bit number (0..7)
  idi>>=3;  // calculate byte offset
  idi%= hash__max[o];  // consider length of hash table
  mem= hash__mem[o];  // get start address of hash table
  mem+= idi;  // calculate address of the byte
  flag= (*mem&(1<<ido))!=0;  // get status of the addressed bit
  return flag;
  }  // end   hash_geti();

static int hash_queryerror() {
  // determine if an error has occurred;
  return hash__errornumber;
  }  // end   hash_queryerror()

//------------------------------------------------------------
// end   Module hash_   OSM hash module
//------------------------------------------------------------



//------------------------------------------------------------
// Module read_   OSM file read module
//------------------------------------------------------------

// this module provides procedures for buffered reading of
// standard input;
// as usual, all identifiers of a module have the same prefix,
// in this case 'read'; one underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

#define read_PREFETCH ((32+3)*1024*1024)
  // number of bytes which will be available in the buffer after
  // every call of read_input();
  // (important for reading .pbf files:
  //  size must be greater than pb__blockM)
#define read__bufM (read_PREFETCH*5)  // length of the buffer;
typedef struct {  // members may not be accessed from external
  int fd;  // file descriptor
  bool eof;  // we are at the end of input file
  byte* bufp;  // pointer in buf[]
  byte* bufe;  // pointer to the end of valid input in buf[]
  int64_t read__counter;
    // byte counter to get the read position in input file;
  uint64_t bufferstart;
    // dummy variable which marks the start of the read buffer
    // concatenated  with this instance of read info structure;
  } read_info_t;

//------------------------------------------------------------

static read_info_t* read_infop= NULL;
  // presently used read info structure, i.e. file handle
#define read__buf ((byte*)&read_infop->bufferstart)
  // start address of the file's input buffer
static byte* read_bufp= NULL;  // may be incremented by external
  // up to the number of read_PREFETCH bytes before read_input() is
  // called again;
static byte* read_bufe= NULL;  // may not be changed from external

static int read_open(const char* filename) {
  // open an input file;
  // filename[]: path and name of input file;
  //             ==NULL: standard input;
  // return: 0: ok; !=0: error;
  // read_infop: handle of the file;
  // note that you should close ever opened file with read_close()
  // before the program ends;

  // save status of presently processed input file (if any)
  if(read_infop!=NULL) {
    read_infop->bufp= read_bufp;
    read_infop->bufp= read_bufe;
    }

  // get memory space for file information and input buffer
  read_infop= (read_info_t*)malloc(sizeof(read_info_t)+read__bufM);
  if(read_infop==NULL) {
    PERRv("could not get %i bytes of memory.",read__bufM)
return 1;
    }

  // initialize read info structure
  read_infop->fd= 0;  // (default) standard input
  read_infop->eof= false;  // we are at the end of input file
  read_infop->bufp= read_infop->bufe= read__buf;  // pointer in buf[]
    // pointer to the end of valid input in buf[]
  read_infop->read__counter= 0;

  // set modul-global variables which are associated with this file
  read_bufp= read_infop->bufp;
  read_bufe= read_infop->bufe;

  // open the file
  if(loglevel>=2)
    fprintf(stderr,"Read-opening: %s",
      filename==NULL? "stdin": filename);
  if(filename==NULL)  // stdin shall be opened
    read_infop->fd= 0;
  else if(filename!=NULL) {  // a real file shall be opened
    read_infop->fd= open(filename,O_RDONLY|O_BINARY);
    if(read_infop->fd<0) {
      if(loglevel>=2)
        fprintf(stderr," -> failed\n");
      PERRv("could not open input file: %.80s",
        filename==NULL? "standard input": filename)
      free(read_infop); read_infop= NULL;
      read_bufp= read_bufe= NULL;
return 1;
      }
    }  // end   a real file shall be opened
  if(loglevel>=2)
    fprintf(stderr," -> FD %i\n",read_infop->fd);
return 0;
  }  // end   read_open()

static void read_close() {
  // close an opened file;
  // read_infop: handle of the file which is to close;
  int fd;

  if(read_infop==NULL)  // handle not valid;
return;
  fd= read_infop->fd;
  if(loglevel>=1) {  // verbose
    fprintf(stderr,"osmfilter: Number of bytes read: %"PRIu64"\n",
      read_infop->read__counter);
    }
  if(loglevel>=2) {
    fprintf(stderr,"Read-closing FD: %i\n",fd);
    }
  if(fd>0)  // not standard input
    close(fd);
  free(read_infop); read_infop= NULL;
  read_bufp= read_bufe= NULL;
  }  // end   read_close()

static inline bool read_input() {
  // read data from standard input file, use an internal buffer;
  // make data available at read_bufp;
  // read_open() must have been called before calling this procedure;
  // return: there are no (more) bytes to read;
  // read_bufp: start of next bytes available;
  //            may be incremented by the caller, up to read_bufe;
  // read_bufe: end of bytes in buffer;
  //            must not be changed by the caller;
  // after having called this procedure, the caller may rely on
  // having available at least read_PREFETCH bytes at address
  // read_bufp - with one exception: if there are not enough bytes
  // left to read from standard input, every byte after the end of
  // the reminding part of the file in the buffer will be set to
  // 0x00 - up to read_bufp+read_PREFETCH;
  int l,r;

  if(read_bufp+read_PREFETCH>=read_bufe) {  // read buffer is too low
    if(!read_infop->eof) {  // still bytes in the file
      if(read_bufe>read_bufp) {  // bytes remaining in buffer
        memmove(read__buf,read_bufp,read_bufe-read_bufp);
          // move remaining bytes to start of buffer
        read_bufe= read__buf+(read_bufe-read_bufp);
          // protect the remaining bytes at buffer start
        }
      else  // no remaining bytes in buffer
        read_bufe= read__buf;  // no bytes remaining to protect
        // add read bytes to debug counter
      read_bufp= read__buf;
      do {  // while buffer has not been filled
        l= (read__buf+read__bufM)-read_bufe-4;
          // number of bytes to read
        r= read(read_infop->fd,read_bufe,l);
        if(r<=0) {  // no more bytes in the file
          read_infop->eof= true;
            // memorize that there we are at end of file
          l= (read__buf+read__bufM)-read_bufe;
            // reminding space in buffer
          if(l>read_PREFETCH) l= read_PREFETCH;
          memset(read_bufe,0,l);  // 2011-12-24
            // set reminding space up to prefetch bytes in buffer to 0
      break;
          }
        read_infop->read__counter+= r;
        read_bufe+= r;  // set new mark for end of data
        read_bufe[0]= 0; read_bufe[1]= 0;  // set 4 null-terminators
        read_bufe[2]= 0; read_bufe[3]= 0;
        } while(r<l);  // end   while buffer has not been filled
      }  // end   still bytes to read
    }  // end   read buffer is too low
  return read_infop->eof && read_bufp>=read_bufe;
  }  // end   read_input()

static void read_switch(read_info_t* filehandle) {
  // switch to another already opened file;
  // filehandle: handle of the file which shall be switched to;

  // first, save status of presently processed input file
  if(read_infop!=NULL) {
    read_infop->bufp= read_bufp;
    read_infop->bufe= read_bufe;
    }
  // switch to new file information
  read_infop= filehandle;
  read_bufp= read_infop->bufp;
  read_bufe= read_infop->bufe;
  read_input();
  }  // end   read_switch()

static inline int read_jump(int position,bool jump) {
  // memorize the current position in the file or jump to it;
  // position: 0..2; storage position;
  //           be careful, no boundary checking is done;
  // jump: jump to a previously stored position;
  // return: ==0: ok; !=0: error;
  static off_t pos[3]= {-1,-1,-1};

  if(jump) {
    if(pos[position]==-1 ||
        lseek(read_infop->fd,pos[position],SEEK_SET)<0) {
      PERRv("could not rewind input file to position %i.",position)
return 1;
      }
    read_infop->read__counter= pos[position];
    read_bufp= read_bufe;  // force refetch
    read_infop->eof= false;  // force retest for end of file
    read_input();  // ensure prefetch
    }
  else {
    pos[position]= read_infop->read__counter-(read_bufe-read_bufp);
      // get current position, take buffer pointer into account;
    }
return 0;
  }  // end   read_jump()

//------------------------------------------------------------
// end Module read_   OSM file read module
//------------------------------------------------------------



//------------------------------------------------------------
// Module write_   write module
//------------------------------------------------------------

// this module provides a procedure which writes a byte to
// standard output;
// as usual, all identifiers of a module have the same prefix,
// in this case 'write'; one underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static char write__buf[UINT64_C(16000000)];
static char* write__bufe= write__buf+sizeof(write__buf);
  // (const) water mark for buffer filled 100%
static char* write__bufp= write__buf;
static int write__fd= 1;  // (initially standard output)
static inline void write_flush();

static void write__end() {
  // terminate the services of this module;
  if(write__fd>1) {  // not standard output
    if(loglevel>=2)
      fprintf(stderr,"Write-closing FD: %i\n",write__fd);
    close(write__fd);
    write__fd= 1;
    }
  }  // end   write__end()

//------------------------------------------------------------

static bool write_testmode= false;  // no standard output
static bool write_error= false;  // an error has occurred

static inline void write_flush() {
  if(write__bufp>write__buf && !write_testmode)
      // at least one byte in buffer AND not test mode
    write_error|=
      write(write__fd,write__buf,write__bufp-write__buf)<0;
  write__bufp= write__buf;
  }  // end   write_flush();

static int write_open(const char* filename) {
  // open standard output file;
  // filename: name of the output file;
  //           this string must be accessible until program end;
  //           ==NULL: standard output;
  // this procedure must be called before any output is done;
  // return: 0: OK; !=0: error;
  static bool firstrun= true;

  if(loglevel>=2)
    fprintf(stderr,"Write-opening: %s\n",
      filename==NULL? "stdout": filename);
  if(filename!=NULL) {  // not standard output
    write__fd= open(filename,
      O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,00600);
    if(write__fd<1) {
      PERRv("could not open output file: %.80s\n",filename)
      write__fd= 1;
return 1;
      }
    }
  if(firstrun) {
    firstrun= false;
    atexit(write__end);
    }
  return 0;
  }  // end   write_open()

static inline void write_char(int c) {
  // write one byte to stdout, use a buffer;
  if(write__bufp>=write__bufe) {  // the write buffer is full
    if(!write_testmode)
      write_error|=
        write(write__fd,write__buf,write__bufp-write__buf)<0;
    write__bufp= write__buf;
    }
  *write__bufp++= (char)c;
  }  // end   write_char();

static inline void write_mem(const byte* b,int l) {
  // write a memory area to stdout, use a buffer;
  while(--l>=0) {
    if(write__bufp>=write__bufe) {  // the write buffer is full
      if(!write_testmode)
        write_error|=
          write(write__fd,write__buf,write__bufp-write__buf)<0;
      write__bufp= write__buf;
      }
    *write__bufp++= (char)(*b++);
    }
  }  // end   write_mem();

static inline void write_str(const char* s) {
  // write a string to stdout, use a buffer;
  while(*s!=0) {
    if(write__bufp>=write__bufe) {  // the write buffer is full
      if(!write_testmode)
        write_error|=
          write(write__fd,write__buf,write__bufp-write__buf)<0;
      write__bufp= write__buf;
      }
    *write__bufp++= (char)(*s++);
    }
  }  // end   write_str();

static inline void write_xmlstr(const char* s) {
  // write an XML string to stdout, use a buffer;
  // every character which is not allowed within an XML string
  // will be replaced by the appropriate decimal sequence;
  static byte allowedchar[]= {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,1,0,0,0,1,1,0,0,0,0,0,0,0,0,  // \"&'
    0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,  // <>
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,  // {}DEL
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    #if 1
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    #else
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0};
    #endif
  byte b0,b1,b2,b3;
  int i;
  uint32_t u;
  #define write__char_D(c) { \
    if(write__bufp>=write__bufe) { \
      if(!write_testmode) \
        write_error|= \
          write(write__fd,write__buf,write__bufp-write__buf)<0; \
      write__bufp= write__buf; \
      } \
    *write__bufp++= (char)(c); }
  #define D(i) ((byte)(s[i]))
  #define DD ((byte)c)

  for(;;) {
    b0= *s++;
    if(b0==0)
  break;
    i= allowedchar[b0];
    if(i==0)  // this character may be written as is
      write__char_D(b0)
    else {  // use numeric encoding
      if(--i<=0)  // one byte
        u= b0;
      else {
        b1= *s++;
        if(--i<=0 && b1>=128)  // two bytes
          u= ((b0&0x1f)<<6)+(b1&0x3f);
        else {
          b2= *s++;
          if(--i<=0 && b1>=128 && b2>=128)  // three bytes
            u= ((b0&0x0f)<<12)+((b1&0x3f)<<6)+(b2&0x3f);
          else {
            b3= *s++;
            if(--i<=0 && b1>=128 && b2>=128 && b3>=128)  // four bytes
              u= ((b0&0x07)<<18)+((b1&0x3f)<<12)+
                ((b1&0x3f)<<6)+(b2&0x3f);
            else
              u= (byte)'?';
            }
          }
        }
      write__char_D('&') write__char_D('#')
      if(u<100) {
        if(u>=10)
          write__char_D(u/10+'0')
        write__char_D(u%10+'0')
        }
      else if(u<1000) {
        write__char_D(u/100+'0')
        write__char_D((u/10)%10+'0')
        write__char_D(u%10+'0')
        }
      else {
        char st[30];

        uint32toa(u,st);
        write_str(st);
        }
      write__char_D(';')
      }  // use numeric encoding
    }
  #undef DD
  #undef D
  #undef write__char_D
  }  // end   write_xmlstr();

static inline void write_xmlmnstr(const char* s) {
  // write an XML string to stdout, use a buffer;
  // every character which is not allowed within an XML string
  // will be replaced by the appropriate mnemonic or decimal sequence;
  static byte allowedchar[]= {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,9,0,0,0,9,9,0,0,0,0,0,0,0,0,  // \"&'
    0,0,0,0,0,0,0,0,0,0,0,0,9,0,9,0,  // <>
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,  // {}DEL
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    #if 1
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    #else
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0};
    #endif
  byte b0,b1,b2,b3;
  int i;
  uint32_t u;
  #define write__char_D(c) { \
    if(write__bufp>=write__bufe) { \
      if(!write_testmode) \
        write_error|= \
          write(write__fd,write__buf,write__bufp-write__buf)<0; \
      write__bufp= write__buf; \
      } \
    *write__bufp++= (char)(c); }
  #define D(i) ((byte)(s[i]))
  #define DD ((byte)c)

  for(;;) {
    b0= *s++;
    if(b0==0)
  break;
    i= allowedchar[b0];
    if(i==0)  // this character may be written as is
      write__char_D(b0)
    else if(i==9) {  // there is a mnemonic for this character
      write__char_D('&')
      switch(b0) {
      case '\"':
        write__char_D('q') write__char_D('u') write__char_D('o')
        write__char_D('t')
        break;
      case '&':
        write__char_D('a') write__char_D('m') write__char_D('p')
        break;
      case '\'':
        write__char_D('a') write__char_D('p') write__char_D('o')
        write__char_D('s')
        break;
      case '<':
        write__char_D('l') write__char_D('t')
        break;
      case '>':
        write__char_D('g') write__char_D('t')
        break;
      default:
        write__char_D('?')  // (should never reach here)
        }
      write__char_D(';')
      }  // there is a mnemonic for this character
    else {  // use numeric encoding
      if(--i<=0)  // one byte
        u= b0;
      else {
        b1= *s++;
        if(--i<=0 && b1>=128)  // two bytes
          u= ((b0&0x1f)<<6)+(b1&0x3f);
        else {
          b2= *s++;
          if(--i<=0 && b1>=128 && b2>=128)  // three bytes
            u= ((b0&0x0f)<<12)+((b1&0x3f)<<6)+(b2&0x3f);
          else {
            b3= *s++;
            if(--i<=0 && b1>=128 && b2>=128 && b3>=128)  // four bytes
              u= ((b0&0x07)<<18)+((b1&0x3f)<<12)+
                ((b1&0x3f)<<6)+(b2&0x3f);
            else
              u= (byte)'?';
            }
          }
        }
      write__char_D('&') write__char_D('#')
      if(u<100) {
        if(u>=10)
          write__char_D(u/10+'0')
        write__char_D(u%10+'0')
        }
      else if(u<1000) {
        write__char_D(u/100+'0')
        write__char_D((u/10)%10+'0')
        write__char_D(u%10+'0')
        }
      else {
        char st[30];

        uint32toa(u,st);
        write_str(st);
        }
      write__char_D(';')
      }  // use numeric encoding
    }
  #undef DD
  #undef D
  #undef write__char_D
  }  // end   write_xmlmnstr();

static inline void write_uint32(uint32_t v) {
  // write an unsigned 32 bit integer number to standard output;
  char s[20],*s1,*s2,c;

  s1= s;
  if(v==0)
    *s1++= '0';
  s2= s1;
  while(v>0)
    { *s2++= (v%10)+'0'; v/= 10; }
  *s2--= 0;
  while(s2>s1)
      { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  write_str(s);
  }  // end write_uint32()

#if 0  // not used at present
static inline void write_sint32(int32_t v) {
  // write a signed 32 bit integer number to standard output;
  char s[20],*s1,*s2,c;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  else if(v==0)
    *s1++= '0';
  s2= s1;
  while(v>0)
    { *s2++= (v%10)+'0'; v/= 10; }
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  write_str(s);
  }  // end write_sint32()
#endif

static inline void write_uint64(uint64_t v) {
  // write an unsigned 64 bit integer number to standard output;
  char s[30],*s1,*s2,c;

  s1= s;
  if(v==0)
    *s1++= '0';
  s2= s1;
  while(v>0)
    { *s2++= (v%10)+'0'; v/= 10; }
  *s2--= 0;
  while(s2>s1)
      { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  write_str(s);
  }  // end write_uint64()

static inline void write_sint64(int64_t v) {
  // write a signed 64 bit integer number to standard output;
  static char s[30],*s1,*s2,c;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  else if(v==0)
    *s1++= '0';
  s2= s1;
  while(v>0)
    { *s2++= (v%10)+'0'; v/= 10; }
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  write_str(s);
  }  // end write_sint64()

static inline void write_sfix7(int32_t v) {
  // write a signed 7 decimals fixpoint value to standard output;
  char s[20],*s1,*s2,c;
  int i;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  s2= s1;
  i= 7;
  while((v%10)==0 && i>1)  // trailing zeros
    { v/= 10;  i--; }
  while(--i>=0)
    { *s2++= (v%10)+'0'; v/= 10; }
  *s2++= '.';
  do
    { *s2++= (v%10)+'0'; v/= 10; }
    while(v>0);
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  write_str(s);
  }  // end write_sfix7()

static inline void write_sfix7o(int32_t v) {
  // write a signed 7 decimals fixpoint value to standard output;
  // keep trailing zeros;
  char s[20],*s1,*s2,c;
  int i;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  s2= s1;
  i= 7;
  while(--i>=0)
    { *s2++= (v%10)+'0'; v/= 10; }
  *s2++= '.';
  do
    { *s2++= (v%10)+'0'; v/= 10; }
    while(v>0);
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  write_str(s);
  }  // end write_sfix7o()

static inline void write_sfix6o(int32_t v) {
  // write a signed 6 decimals fixpoint value to standard output;
  // keep trailing zeros;
  char s[20],*s1,*s2,c;
  int i;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  s2= s1;
  i= 6;
  while(--i>=0)
    { *s2++= (v%10)+'0'; v/= 10; }
  *s2++= '.';
  do
    { *s2++= (v%10)+'0'; v/= 10; }
    while(v>0);
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  write_str(s);
  }  // end write_sfix6o()

#if 0  // currently unused
static inline void write_sfix9(int64_t v) {
  // write a signed 9 decimals fixpoint value to standard output;
  char s[20],*s1,*s2,c;
  int i;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  s2= s1;
  i= 9;
  while(--i>=0)
    { *s2++= (v%10)+'0'; v/= 10; }
  *s2++= '.';
  do
    { *s2++= (v%10)+'0'; v/= 10; }
    while(v>0);
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  write_str(s);
  }  // end write_sfix9()
#endif

static void write_timestamp(uint64_t v) {
  // write a timestamp in OSM format, e.g.: "2010-09-30T19:23:30Z"
  time_t vtime;
  struct tm tm;
  char s[30],*sp;
  int i;

  vtime= v;
  #if __WIN32__
  memcpy(&tm,gmtime(&vtime),sizeof(tm));
  #else
  gmtime_r(&vtime,&tm);
  #endif
  i= tm.tm_year+1900;
  sp= s+3; *sp--= i%10+'0';
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
  write_str(s);
  }  // end   write_timestamp()

//------------------------------------------------------------
// end   Module write_   write module
//------------------------------------------------------------



//------------------------------------------------------------
// Module count_   tag count module
//------------------------------------------------------------

// this module contains procedures which are responsible for
// counting the keys or the values of OSM tags;
// as usual, all identifiers of a module have the same prefix,
// in this case 'count_'; one underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

#define count__itemM 1000000
#define count__itemMs "1000000"
#define count__nameL 60
#define count__nameLs "60"
#define STR(s) #s

typedef struct {
  int32_t counter;
  char name[count__nameL];
  } count__item_t;
static count__item_t* count__item= NULL;
static count__item_t* count__iteme= NULL,*count__itemee= NULL;
  // last logical and last physical element (each exclusive);
static count__item_t** count__index= NULL;
  // index table of the item table
static count__item_t** count__indexe= NULL;
  // last logical element (exclusive);

static int count__qsortcount(const void* a,const void* b) {
  // count comparison for qsort()
  int32_t ax,bx;

  ax= (*(count__item_t**)a)->counter;
  bx= (*(count__item_t**)b)->counter;
  if(ax>bx)
return -1;
  if(ax<bx)
return 1;
  return 
    strcmp((*(count__item_t**)a)->name,(*(count__item_t**)b)->name);
  }  // end   count__qsortcount()

static void count__end() {
  // clean-up module's variables;
  if(count__item!=NULL) {
    free(count__item);
    count__item= NULL;
    }
  if(count__index!=NULL) {
    free(count__index);
    count__index= NULL;
    }
  }  // end   count__end()

//------------------------------------------------------------

static int count_ini() {
  // initialize this module;
  // return: ==0: ok; 1: could not get the memory;
  if(count__item!=NULL)  // already initialized
return 0;
  count__item=
    (count__item_t*)malloc(sizeof(count__item_t)*count__itemM);
  if(count__item==NULL)
    goto error;
  count__iteme= count__item;
  count__itemee= count__item+count__itemM;
  atexit(count__end);
  count__index=
    (count__item_t**)malloc(sizeof(count__item_t*)*count__itemM);
  if(count__index==NULL)
    goto error;
  count__indexe= count__index;
  return 0;
error:
  PERR("could not get memory for the counter.")
  return 1;
  }  // end   count_ini()

static inline void count_add(const char* name) {
  // add a new name to the item table;
  count__item_t** low,**mid,**high;
  const byte* np,*sp;
  static int compare= -1;
  int size;

  // determine if the name already exists in the table;
  low= count__index; high= count__indexe-1; mid= count__index;
  while(low<=high) {
    mid= low+(high-low)/2;
    np= (byte*)name;
    sp= (byte*)((*mid)->name);
    #define D if((compare= *np-*sp)==0 && *np!=0) {np++; sp++;
      // (just to speed-up the comparison a bit)
    D D D D D D D D D D D D D D D D
      compare= strncmp((char*)np,(char*)sp,count__nameL-1-16);
      }}}}}}}}}}}}}}}}
    #undef D
    if(compare==0)
  break;
    if(compare<0)
      high= mid-1;
    else
      low= mid+1;
    }
  if(compare==0) {  // found element match
    // increment this element's counter
    (*mid)->counter++;
return;
    }  // found element match
  // here: did not find a matching element

  // add a new element to the and insert new index into index list
  if(count__iteme>=count__itemee) {  // no space left in table
    WARN("too many items to count (maximum is "count__itemMs").")
return;
    }  // end   no space left in table
  count__iteme->counter= 1;
  strMcpy(count__iteme->name,name);
  if(compare>0) mid++;
  size= (char*)count__indexe-(char*)mid;
  if(size>0)
    memmove(mid+1,mid,size);
  *mid= count__iteme;
  count__iteme++;
  count__indexe++;
  }  // end   count_add()

static void count_sort() {
  // sort the list of items by the number of their occurrence
  qsort(count__index,count__indexe-count__index,sizeof(*count__index),
    count__qsortcount);
  }  // end   count_sort()

static void count_write() {
  // write the list of items to output stream
  char s[20+count__nameL+10];
  count__item_t** kpp,*kp;

  kpp= count__index;
  while(kpp<count__indexe) {
    kp= *kpp;
    sprintf(s,"%11i\t%."count__nameLs"s"NL,kp->counter,kp->name);
    write_str(s);
    kpp++;
    }
  }  // end   count_write()

//------------------------------------------------------------
// end   Module count_   tag count module
//------------------------------------------------------------



//------------------------------------------------------------
// Module fil_   osm filter module
//------------------------------------------------------------

// this module contains procedures which are responsible for
// filtering OSM data;
// as usual, all identifiers of a module have the same prefix,
// in this case 'fil_'; one underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static inline void fil_stresccpy(char *dest, const char *src,
    size_t len) {
  // similar as strmpy(), but remove every initial '\\' character;
  // len: length of the source string - without terminating zero;
  while(len>0) {
    if(*src=='\\') { src++; len--; }
    if(!(len>0) || *src==0)
  break;
    len--;
    *dest++= *src++;
    }
  *dest= 0;
  }  // end   fil_stresccpy()

static inline bool fil__cmp(const char* s1,const char* s2) {
  // this procedure compares two character strings;
  // s1[]: first string;
  // s2[0]: operator which shall be used for comparison;
  //         0: '=', and there are wildcards coded in s2[1]:
  //                 s2[1]==1: wildcard at start;
  //                 s2[1]==2: wildcard at end;
  //                 s2[1]==3: wildcard at both, start and end;
  //         1: '!=', and there are wildcards coded in s2[1];
  //         2: '='
  //         4: '<'
  //         5: '>='
  //         6: '>'
  //         7: '<='
  //         8: unused
  //         9: unused
  //        10: '=', numeric
  //        11: '!=', numeric
  //        12: '<', numeric
  //        13: '>=', numeric
  //        14: '>', numeric
  //        15: '<=', numeric
  // s2+1: string to compare with the first string;
  //       this string will start at s2+2 if wildcards are supplied;
  // return: condition is met;
  int op,wc;  // operator, wildcard flags
  int diff;  // (for numeric comparison)
  unsigned char s1v,s2v;  // (for numeric comparison)

  op= *s2++;
  if(op==2) { // '='
    // first we care about the 'equal' operator
    // because it's the most frequently used option
    while(*s1==*s2 && *s1!=0) { s1++; s2++; }
    return *s1==0 && *s2==0;
    }
  switch(op) {  // depending on comparison operator
  case 0:  // '=', and there are wildcards
    wc= *s2++;
    if(wc==2) {  // wildcard at end
      while(*s1==*s2 && *s1!=0) { s1++; s2++; }
      return *s2==0;
      }  // wildcard at end
    if(wc==1) {  // wildcard at start
      const char* s11,*s22;

      while(*s1!=0) {  // for all start positions in s1[]
        s11= s1; s22= s2;
        while(*s11==*s22 && *s11!=0) { s11++; s22++; }
        if(*s11==0 && *s22==0)
          return true;
        s1++;
        }  // for all start positions in s1[]
      return false;
      }  // wildcard at start
    /* wildcards at start and end */ {
      const char* s11,*s22;

      while(*s1!=0) {  // for all start positions in s1[]
        s11= s1; s22= s2;
        while(*s11==*s22 && *s11!=0) { s11++; s22++; }
        if(*s22==0)
          return true;
        s1++;
        }  // for all start positions in s1[]
      return false;
      }  // wildcards at start and end
  case 1:  // '!=', and there are wildcards
    wc= *s2++;
    if(wc==2) {  // wildcard at end
      while(*s1==*s2 && *s1!=0) { s1++; s2++; }
      return *s2!=0;
      }  // wildcard at end
    if(wc==1) {  // wildcard at start
      const char* s11,*s22;

      while(*s1!=0) {  // for all start positions in s1[]
        s11= s1; s22= s2;
        while(*s11==*s22 && *s11!=0) { s11++; s22++; }
        if(*s11==0 && *s22==0)
          return false;
        s1++;
        }  // for all start positions in s1[]
      return true;
      }  // wildcard at start
    /* wildcards at start and end */ {
      const char* s11,*s22;

      while(*s1!=0) {  // for all start positions in s1[]
        s11= s1; s22= s2;
        while(*s11==*s22 && *s11!=0) { s11++; s22++; }
        if(*s22==0)
          return false;
        s1++;
        }  // for all start positions in s1[]
      return true;
      }  // wildcards at start and end
  //case 2:  // '='  (we already cared about this)
  case 3:  // '!='
    while(*s1==*s2 && *s1!=0) { s1++; s2++; }
    return *s1!=0 || *s2!=0;
  case 4:  // '<'
    while(*s1==*s2 && *s1!=0) { s1++; s2++; }
    return *(unsigned char*)s1 < *(unsigned char*)s2;
  case 5:  // '>='
    while(*s1==*s2 && *s1!=0) { s1++; s2++; }
    return *(unsigned char*)s1 >= *(unsigned char*)s2;
  case 6:  // '>'
    while(*s1==*s2 && *s1!=0) { s1++; s2++; }
    return *(unsigned char*)s1 > *(unsigned char*)s2;
  case 7:  // '<='
    while(*s1==*s2 && *s1!=0) { s1++; s2++; }
    return *(unsigned char*)s1 <= *(unsigned char*)s2;
  case 10:  // '=', numeric
    while(*s1=='0') s1++;
    while(*s2=='0') s2++;
    while(*s1==*s2 && isdigi(*(unsigned char*)s1))
      { s1++; s2++; }
    if(*s1=='.') {
      if(*s2=='.') {
        do { s1++; s2++; }
          while(*s1==*s2 && isdigi(*(unsigned char*)s1));
        if(!isdigi(*(unsigned char*)s1)) {
          while(*s2=='0') s2++;
          return !isdigi(*(unsigned char*)s2);
          }
        if(!isdigi(*(unsigned char*)s2)) {
          while(*s1=='0') s1++;
          return !isdigi(*(unsigned char*)s1);
          }
        return !isdigi(*(unsigned char*)s1) &&
          !isdigi(*(unsigned char*)s2);
        }
      do s1++;
        while(*s1=='0');
      return !isdigi(*(unsigned char*)s1);
      }
    if(*s2=='.') {
      do s2++;
        while(*s2=='0');
      return !isdigi(*(unsigned char*)s2);
      }
    return !isdigi(*(unsigned char*)s1) && !isdigi(*(unsigned char*)s2);
  case 11:  // '!=', numeric
    while(*s1=='0') s1++;
    while(*s2=='0') s2++;
    while(*s1==*s2 && isdigi(*(unsigned char*)s1))
      { s1++; s2++; }
    if(*s1=='.') {
      if(*s2=='.') {
        do { s1++; s2++; }
          while(*s1==*s2 && isdigi(*(unsigned char*)s1));
        if(!isdigi(*(unsigned char*)s1)) {
          while(*s2=='0') s2++;
          return isdigi(*(unsigned char*)s2);
          }
        if(!isdigi(*(unsigned char*)s2)) {
          while(*s1=='0') s1++;
          return isdigi(*(unsigned char*)s1);
          }
        return isdigi(*(unsigned char*)s1) ||
          isdigi(*(unsigned char*)s2);
        }
      do s1++;
        while(*s1=='0');
      return isdigi(*(unsigned char*)s1);
      }
    if(*s2=='.') {
      do s2++;
        while(*s2=='0');
      return isdigi(*(unsigned char*)s2);
      }
    return isdigi(*(unsigned char*)s1) || isdigi(*(unsigned char*)s2);
  case 12:  /* '<', numeric */
    #define Ds1 s1
    #define Ds2 s2
    s1v= *(unsigned char*)Ds1; s2v= *(unsigned char*)Ds2;
    if(s1v=='-') {
      if(s2v=='-') {
        Ds1++; s2v= *(unsigned char*)Ds1;
        Ds2++; s1v= *(unsigned char*)Ds2;
        goto op_14;
        }
      return true;
      }
    else if(s2v=='-')
      return false;
    op_12:
    while(s1v=='0') { Ds1++; s1v= *(unsigned char*)Ds1; }
    while(s2v=='0') { Ds2++; s2v= *(unsigned char*)Ds2; }
    while(s1v==s2v && isdigi(s1v)) {
      Ds1++; s1v= *(unsigned char*)Ds1;
      Ds2++; s2v= *(unsigned char*)Ds2;
      }
    diff= digival(s1v)-digival(s2v);
    while(isdigi(s1v) && isdigi(s2v)) {
      Ds1++; s1v= *(unsigned char*)Ds1;
      Ds2++; s2v= *(unsigned char*)Ds2;
      }
    if(s1v=='.') {
      if(s2v=='.') {
        if(diff!=0)
          return diff<0;
        do {
          Ds1++; s1v= *(unsigned char*)Ds1;
          Ds2++; s2v= *(unsigned char*)Ds2;
          } while(s1v==s2v && isdigi(s1v));
        while(s2v=='0') { Ds2++; s2v= *(unsigned char*)Ds2; }
        return digival(s1v) < digival(s2v);
        }
      return isdigi(s2v) || diff<0;
      }
    if(s2v=='.') {
      if(isdigi(s1v))
        return false;
      if(diff!=0)
        return diff<0;
      do { Ds2++; s2v= *(unsigned char*)Ds2; } while(s2v=='0');
      return isdigi(s2v);
      }
    return isdigi(s2v) || (!isdigi(s1v) && diff<0);
    #undef Ds1
    #undef Ds2
  case 13:  /* '>=', numeric */
    #define Ds1 s1
    #define Ds2 s2
    s1v= *(unsigned char*)Ds1; s2v= *(unsigned char*)Ds2;
    if(s1v=='-') {
      if(s2v=='-') {
        Ds1++; s2v= *(unsigned char*)Ds1;
        Ds2++; s1v= *(unsigned char*)Ds2;
        goto op_15;
        }
      return false;
      }
    else if(s2v=='-')
      return true;
    op_13:
    while(s1v=='0') { Ds1++; s1v= *(unsigned char*)Ds1; }
    while(s2v=='0') { Ds2++; s2v= *(unsigned char*)Ds2; }
    while(s1v==s2v && isdigi(s1v)) {
      Ds1++; s1v= *(unsigned char*)Ds1;
      Ds2++; s2v= *(unsigned char*)Ds2;
      }
    diff= digival(s1v)-digival(s2v);
    while(isdigi(s1v) && isdigi(s2v)) {
      Ds1++; s1v= *(unsigned char*)Ds1;
      Ds2++; s2v= *(unsigned char*)Ds2;
      }
    if(s1v=='.') {
      if(s2v=='.') {
        if(diff!=0)
          return diff>=0;
        do {
          Ds1++; s1v= *(unsigned char*)Ds1;
          Ds2++; s2v= *(unsigned char*)Ds2;
          } while(s1v==s2v && isdigi(s1v));
        while(s2v=='0') { Ds2++; s2v= *(unsigned char*)Ds2; }
        return digival(s1v) >= digival(s2v);
        }
      return !isdigi(s2v) && diff>=0;
      }
    if(s2v=='.') {
      if(isdigi(s1v))
        return true;
      if(diff!=0)
        return diff>=0;
      do { Ds2++; s2v= *(unsigned char*)Ds2; } while(s2v=='0');
      return !isdigi(s2v);
      }
    return !isdigi(s2v) && (isdigi(s1v) || diff>=0);
    #undef Ds1
    #undef Ds2
  case 14:  /* '>', numeric */
    #define Ds1 s2
    #define Ds2 s1
    s1v= *(unsigned char*)Ds1; s2v= *(unsigned char*)Ds2;
    if(s1v=='-') {
      if(s2v=='-') {
        Ds1++; s2v= *(unsigned char*)Ds1;
        Ds2++; s1v= *(unsigned char*)Ds2;
        goto op_12;
        }
      return true;
      }
    else if(s2v=='-')
      return false;
    op_14:
    while(s1v=='0') { Ds1++; s1v= *(unsigned char*)Ds1; }
    while(s2v=='0') { Ds2++; s2v= *(unsigned char*)Ds2; }
    while(s1v==s2v && isdigi(s1v)) {
      Ds1++; s1v= *(unsigned char*)Ds1;
      Ds2++; s2v= *(unsigned char*)Ds2;
      }
    diff= digival(s1v)-digival(s2v);
    while(isdigi(s1v) && isdigi(s2v)) {
      Ds1++; s1v= *(unsigned char*)Ds1;
      Ds2++; s2v= *(unsigned char*)Ds2;
      }
    if(s1v=='.') {
      if(s2v=='.') {
        if(diff!=0)
          return diff<0;
        do {
          Ds1++; s1v= *(unsigned char*)Ds1;
          Ds2++; s2v= *(unsigned char*)Ds2;
          } while(s1v==s2v && isdigi(s1v));
        while(s2v=='0') { Ds2++; s2v= *(unsigned char*)Ds2; }
        return digival(s1v) < digival(s2v);
        }
      return isdigi(s2v) || diff<0;
      }
    if(s2v=='.') {
      if(isdigi(s1v))
        return false;
      if(diff!=0)
        return diff<0;
      do { Ds2++; s2v= *(unsigned char*)Ds2; } while(s2v=='0');
      return isdigi(s2v);
      }
    return isdigi(s2v) || (!isdigi(s1v) && diff<0);
    #undef Ds1
    #undef Ds2
  case 15:  /* '<=', numeric */
    #define Ds1 s2
    #define Ds2 s1
    s1v= *(unsigned char*)Ds1; s2v= *(unsigned char*)Ds2;
    if(s1v=='-') {
      if(s2v=='-') {
        Ds1++; s2v= *(unsigned char*)Ds1;
        Ds2++; s1v= *(unsigned char*)Ds2;
        goto op_13;
        }
      return false;
      }
    else if(s2v=='-')
      return true;
    op_15:
    while(s1v=='0') { Ds1++; s1v= *(unsigned char*)Ds1; }
    while(s2v=='0') { Ds2++; s2v= *(unsigned char*)Ds2; }
    while(s1v==s2v && isdigi(s1v)) {
      Ds1++; s1v= *(unsigned char*)Ds1;
      Ds2++; s2v= *(unsigned char*)Ds2;
      }
    diff= digival(s1v)-digival(s2v);
    while(isdigi(s1v) && isdigi(s2v)) {
      Ds1++; s1v= *(unsigned char*)Ds1;
      Ds2++; s2v= *(unsigned char*)Ds2;
      }
    if(s1v=='.') {
      if(s2v=='.') {
        if(diff!=0)
          return diff>=0;
        do {
          Ds1++; s1v= *(unsigned char*)Ds1;
          Ds2++; s2v= *(unsigned char*)Ds2;
          } while(s1v==s2v && isdigi(s1v));
        while(s2v=='0') { Ds2++; s2v= *(unsigned char*)Ds2; }
        return digival(s1v) >= digival(s2v);
        }
      return !isdigi(s2v) && diff>=0;
      }
    if(s2v=='.') {
      if(isdigi(s1v))
        return true;
      if(diff!=0)
        return diff>=0;
      do { Ds2++; s2v= *(unsigned char*)Ds2; } while(s2v=='0');
      return !isdigi(s2v);
      }
    return !isdigi(s2v) && (isdigi(s1v) || diff>=0);
    #undef Ds1
    #undef Ds2
  // (no default)
    }  // depending on comparison operator
  return false;  // (we never get here)
  }  // end   fil__cmp()

#define fil__pairM 1000  // maximum number of key-val-pairs
#define fil__pairkM 100  // maximum length of key or val;
#define fil__pairtM 12  // maximum number of filter types;
  // these filter types are defined as follows:
  //  0: keep  node     object;
  //  1: keep  way      object;
  //  2: keep  relation object;
  //  3:  drop node     object;
  //  4:  drop way      object;
  //  5:  drop relation object;
  //  6: keep  node     tag;
  //  7: keep  way      tag;
  //  8: keep  relation tag;
  //  9:  drop node     tag;
  // 10:  drop way      tag;
  // 11:  drop relation tag;
typedef struct {  // key/val pair for the include filter
  char k[fil__pairkM+8];  // key to compare;
    // [0]==0 && [1]==0: same key as previous key in list;
  char v[fil__pairkM+8];  // value to the key in .k[];
    // the first byte represents a comparison operator,
    // see parameter s2[]in fil__cmp() for details;
    // [0]==0 && [1]==0: any value will be accepted;
  int left_bracketn;
    // number of opening brackets right before the comparison
  int right_bracketn;
    // number of closing brackets right after the comparison
  bool operator;
    // Boolean operator right after the closing bracket, resp.
    // right after the comparison, if there is no closing bracket;
    // false: OR; true: AND;
  } fil__pair_t;
static fil__pair_t fil__pair[fil__pairtM][fil__pairM+2]=
  {{{{0},{0},0,0,false}}};
static fil__pair_t* fil__paire[fil__pairtM]=
  { &fil__pair[0][0],&fil__pair[1][0],
    &fil__pair[2][0],&fil__pair[3][0],
    &fil__pair[4][0],&fil__pair[5][0],
    &fil__pair[6][0],&fil__pair[7][0],
    &fil__pair[8][0],&fil__pair[9][0],
    &fil__pair[10][0],&fil__pair[11][0] };
static fil__pair_t* fil__pairee[fil__pairtM]=
  { &fil__pair[0][fil__pairM],&fil__pair[1][fil__pairM],
    &fil__pair[2][fil__pairM],&fil__pair[3][fil__pairM],
    &fil__pair[4][fil__pairM],&fil__pair[5][fil__pairM],
    &fil__pair[6][fil__pairM],&fil__pair[7][fil__pairM],
    &fil__pair[8][fil__pairM],&fil__pair[9][fil__pairM],
    &fil__pair[10][fil__pairM],&fil__pair[11][fil__pairM] };
static int fil__err_tagsbool= 0;
  // number of Boolean expressions in tags filter
static int fil__err_tagsbracket= 0;
  // number of brackets in tags filter

//------------------------------------------------------------

static inline void fil_cpy(char *dest, const char *src,
    size_t len,int op) {
  // similar as strmpy(), but remove every initial '\\' character;
  // len: length of the source string - without terminating zero;
  // op: comparison operator;
  //         2: '='
  //         4: '<'
  //         5: '>='
  //         6: '>'
  //         7: '<='
  // return: dest[0]: comparison operator; additional possible values:
  //         0: '=', and there are wildcards coded in dest[1]:
  //                 dest[1]==1: wildcard at start;
  //                 dest[1]==2: wildcard at end;
  //                 dest[1]==3: wildcard at both, start and end;
  //         1: '!=', and there are wildcards coded in dest[1];
  //        10: '=', numeric
  //        11: '!=', numeric
  //        12: '<', numeric
  //        13: '>=', numeric
  //        14: '>', numeric
  //        15: '<=', numeric
  int wc;  // wildcard indicator, see fil__cmp()

  if(op<0) {  // unknown operator
    WARNv("unknown comparison at: %.80s",src)
    op= 2;  // assume '='
    }
  if(len>(fil__pairkM)) {
    len= fil__pairkM;  // delimit value length
    WARNv("filter argument too long: %.*s",fil__pairkM,src)
    }
  wc= 0;  // (default)
  if(len>=2 && src[0]=='*') {  // wildcard at start
    wc|= 1;
    src++; len--;
    }
  if((len>=2 && src[len-1]=='*' && src[len-2]!='\\') ||
      (len==1 && src[len-1]=='*')) {
      // wildcard at end
    wc|= 2;
    len--;
    }
  if(wc==0) {  // no wildcard(s)
    const char* v;

    v= src;
    if(*v=='-') v++;  // jump over sign
    if(isdig(*v))  // numeric value
      op+= 8;
    dest[0]= op;
    fil_stresccpy(dest+1,src,len);  // store this value
    }  // no wildcard(s)
  else {  // wildcard(s)
    dest[0]= op&1;
    dest[1]= wc;
    fil_stresccpy(dest+2,src,len);  // store this value
    }  // wildcard(s)
  }  // end   fil_cpy()

static bool fil_active[fil__pairtM]=
    {false,false,false,false,false,false,
    false,false,false,false,false,false};
  // the related filter list has at least one element;
  // may be written only by this module;
static bool fil_activeo[3]= {false,false,false};
  // at least one of the filter lists 6..8 and 9..11 has
  // at least one element;
  // index is otype: 0: node; 1: way; 2: relation;
static bool fil_meetall[fil__pairtM]=
    {false,false,false,false,false,false,
    false,false,false,false,false,false};
  // the tested object must meet all criteria of this filter;
  // for ftype==0..3 ('keep object'):
  // conditions are combined with 'AND';
  // ftype==6..8 ('keep tags'):
  // every tag which is not listed in the filter parameter will
  // be deleted;
  // this element is valid only if there is at least one
  // entry in previous fields;

static bool fil_filterheader= false;
  // there are filter parameters which affect the header; therefore the
  // OSM object header values must be checked;
  // this value must not be changed from outside the module;

static void fil_ini() {
  // initialize this mudule;
  // (this procedure is not speed-optimized)
  int i;

  //memset(fil__pair,0,sizeof(fil__pair));
  for(i= 0; i<fil__pairtM; i++) {
    fil__paire[i]= &fil__pair[i][0];
    fil__pairee[i]= &fil__pair[i][fil__pairM];
    fil_active[i]= false;
    fil_activeo[i/4]= false;
    fil_meetall[i]= false;
    fil__pair[i][0].left_bracketn= 0;
    }
  fil__err_tagsbool= fil__err_tagsbracket= 0;
  fil_filterheader= false;
  }  // fil_ini()

static void fil_parse(int ftype,const char* arg) {
  // interprets a command line argument and stores filter information;
  // Boolean terms are recognized;
  // ftype: filter type; see explanation at fil__pairtM;
  // arg[]: filter information; e.g.:
  //        "amenity=restaurant && name=John =Meyer"
  fil__pair_t* fp,*fe,*fee;
  const char* pk,*pv,*pe;  // pointers in parameter for key/val pairs;
    // pk: key; pv: val; pe: end of val;
  bool meetall;  // same as fil_meetall[ftype]
  int len;  // string length
  int argop;  // argument operator; 0: unknown; '&', '|', '(', ')';
  int op;  // operator, see fil__cmp()

  fp= fil__pair[ftype];
  fe= fil__paire[ftype];
  fee= fil__pairee[ftype];
  if(loglevel>0)
    PINFOv("Filter: %s %s%s:",
      ftype/3%2==0? "keep": "drop",ONAME(ftype%3),
      ftype<6? "s": " tags")
  pk= arg;
  while(*pk==' ') pk++;  // jump over spaces
  if(strzcmp(pk,"all ")==0 || strzcmp(pk,"and ")==0) {
    if(loglevel>0)
      PINFO("Filter: meet all conditions.")
    fil_meetall[ftype]= true;
    pk+= 4;
    }
  meetall= fil_meetall[ftype];
  while(pk!=NULL && fe<fee) {  // for every key/val pair
    while(*pk==' ') pk++;  // jump over (additional) spaces
    if(*pk==0)
  break;
    pe= pk;
    while((*pe!=' ' || pe[-1]=='\\') && *pe!=0) pe++;
      // get end of this pair
    len= pe-pk;  // length of this argument
    argop= 0;  // (default)
    if(len==2 && strzcmp(pk,"&&")==0) argop= '&';
    else if(len==2 && strzcmp(pk,"||")==0) argop= '|';
    else if(len==3 && strzcmp(pk,"AND")==0) argop= '&';
    else if(len==2 && strzcmp(pk,"OR")==0) argop= '|';
    else if(len==3 && strzcmp(pk,"and")==0) argop= '&';
    else if(len==2 && strzcmp(pk,"or")==0) argop= '|';
    else if(len==1 && strzcmp(pk,"(")==0) argop= '(';
    else if(len==1 && strzcmp(pk,")")==0) argop= ')';
    if(argop!=0) {  // this is an argument operator
      if(ftype>=6) {  // filter type applies to tags
        if(argop=='(' || argop==')')
          fil__err_tagsbracket++;
        else
          fil__err_tagsbool++;
        pk= pe;  // jump to next key/val pair in parameter list
  continue;
        }  // filter type applies to tags
      if(fe==fp) {  // first argument
        if(argop=='(') {
          if(loglevel>0) PINFO("Filter:   (")
          fe[0].left_bracketn++;
          }
        else
          WARNv("Unknown operator at start of: %.80s",pk)
        }
      else switch(argop) {  // in dependence of argument operator
        // add Boolean operator to previous comparison
      case '&':
        if(loglevel>0) PINFO("Filter:     AND")
        fe[-1].operator= true; break;
      case '|':
        if(loglevel>0) PINFO("Filter:   OR")
        fe[-1].operator= false; break;
      case ')':
        if(loglevel>0) PINFO("Filter:   )")
        if(fe[0].left_bracketn!=0)
          WARNv("Bracket error at: %.80s",pk)
        fe[-1].right_bracketn++;
        break;
      case '(':
        if(loglevel>0) PINFO("Filter:   (")
        fe[0].left_bracketn++;
        break;
        }  // in dependence of argument operator
      pk= pe;  // jump to next key/val pair in parameter list
  continue;
      }
    pv= pk;
    while(((*pv!='=' && *pv!='<' && *pv!='>' &&
        (*pv!='!' || pv[1]!='=')) ||
        (pv>pk && pv[-1]=='\\')) && pv<pe) pv++;
      // find operator =, <, >, !=
    if(pv>=pe-1) pv= pe;  // there was no operator in this pair
    len= pv-pk;  // length of this key
    if(len>(fil__pairkM)) {
      len= fil__pairkM;  // delimit key length
      WARNv("filter key too long: %.*s",fil__pairkM,pk)
      }
    op= -1;  // 'unknown operator' (default)
    if(pv>=pe) {  // there is a key but no value
      if(len>0 && pk[len-1]=='=') len--;
      fil_cpy(fe->k,pk,len,2);  // store this key, op='='
      memset(fe->v,0,3);  // store empty value
      }
    else {  // key and value
      if(len==0)  // no key given
        memset(fe->k,0,3);  // store empty key,
          // i.e., mark pair as 'pair with same key';
          // note that this is not allowed at start of term,
          // after bracket(s) and after different operators;
          // this will be checked in fil_plausi();
      else
        fil_cpy(fe->k,pk,len,2);  // store this key, op='='
      if(*pv=='=') op= 2;
      else if(*pv=='!' && pv[1]=='=') op= 3;
      else if(*pv=='<' && pv[1]!='=') op= 4;
      else if(*pv=='>' && pv[1]=='=') op= 5;
      else if(*pv=='>' && pv[1]!='=') op= 6;
      else if(*pv=='<' && pv[1]=='=') op= 7;
      if(op<0) {  // unknown operator
        WARNv("unknown comparison at: %.80s",pv)
        op= 2;  // assume '='
        }
      pv++;  // jump over operator
      if(pv<pe && *pv=='=') pv++;
        // jump over second character of a two-character operator
      len= pe-pv;  // length of this value
      fil_cpy(fe->v,pv,len,op);  // store this value
      }  // key and value
    if(loglevel>0) {
      static const char* ops[]= { "?",
        "=","!=","=","!=","<",">=",">","<=",
        "?","?","=(numeric)","!=(numeric)",
        "<(numeric)",">=(numeric)",">(numeric)","<=(numeric)" };

      PINFOv("Filter:     %s\"%.80s\"%s %s %s\"%.80s\"%s",
        fe->k[0]<=1 && (fe->k[1] & 1)? "*": "",
        *(int16_t*)(fe->k)==0? "(last key)":
          fe->k[0]>=2? fe->k+1: fe->k+2,
        fe->k[0]<=1 && (fe->k[1] & 2)? "*": "",
        ops[fe->v[0]+1],
        fe->v[0]<=1 && (fe->v[1] & 1)? "*": "",
        *(int16_t*)(fe->v)==0? "(anything)":
          fe->v[0]>=2? fe->v+1: fe->v+2,
        fe->v[0]<=1 && (fe->v[1] & 2)? "*": "");
      }
    if(fe->k[1]=='@')
      fil_filterheader= true;
    fe->right_bracketn= 0;
    fe->operator= false;
    if(fe>fp && meetall && *(int16_t*)fe->k!=0)
        // not first comparison AND all conditions are to be met AND
        // this comparison has not an empty key
      fe[-1].operator= true;
    fe++;  // next pair in key/val table
    fe->left_bracketn= 0;
    pk= pe;  // jump to next key/val pair in parameter list
    }  // end   for every key/val pair
  if(fe>=fee)
    WARN("too many filter parameters.")
  fil__paire[ftype]= fe;
  fil_active[ftype]= true;
  if(ftype/3==2 || ftype/3==3)  // keep tags OR drop tags
    fil_activeo[ftype%3]= true;
  if(ftype<6)
    global_recursive= true;  // recursive processing is necessary
  }  // end   fil_parse()

static int fil_plausi() {
  // check plausibility of filter parameter;
  // may be called after all parameters have been parsed
  // with fil_parse();
  // furthermore, this procedure inserts brackets to invert
  // Boolean operator priorities if the keyword "all" had been given;
  // return: o: OK; !=0: syntax error;
  int ft;  // filter type
  int bracket_balance;
  int bl;  // open brackets without correspondence
  int br;  // closed brackets without correspondence
  int bm;  // bracket occurrences if 'meetall' 
  fil__pair_t* f,*fp,*fe;
  int synt;  // number of syntax errors
  //int tagsbool;  // number of Boolean expressions in tags filter
  //int tagsbracket;  // number of brackets in tags filter

  // check plausibility
  bl= br= bm= synt= 0;
  //tagsbool= tagsbracket= 0;
  for(ft= 0;ft<6;ft++) {  // for every object filter type
    fp= f= fil__pair[ft]; fe= fil__paire[ft];
    bracket_balance= 0;
    while(fp<fe) {  // for every key/val pair in filter
      bracket_balance+= fp->left_bracketn-fp->right_bracketn;
      if(fil_meetall[ft] &&
          (fp->left_bracketn!=0 || fp->right_bracketn!=0))
        bm++;
      if(*(int16_t*)fp->k==0) {  // empty key
        if(fp==f) {
          PERR("empty key cannot start a term.")
          synt++;
          }
        else if(fp[-1].right_bracketn!=0 || fp[0].left_bracketn!=0) {
          PERR("empty key not valid after bracket.")
          synt++;
          }
        else if(fp>=f+2 && !fp[-1].operator &&
            fp[-2].operator) {
          // last operators were AND and OR
          PERR("empty key must not follow OR after AND.")
          synt++;
          }
        #if 0
        else if(fp>=f+2 && *(int16_t*)fp[-1].k==0 &&
            fp[-1].operator!=fp[-2].operator) {
          // last key was empty too, AND operator changed
          PERR("empty keys must not follow different operators.")
          synt++;
          }
        #endif
        }  // empty key
      fp++;
      }  // for every key/val pair in filter
    if(bracket_balance>0)
      bl+= bracket_balance;
    else if(bracket_balance<0)
      br+= -bracket_balance;
    }  // for every filter type
  if(bl==1)
    PERR("missing one right bracket.")
  else if(bl>1)
    PERRv("missing %i right brackets.",bl)
  if(br==1)
    PERR("missing one left bracket.")
  else if(br>1)
    PERRv("missing %i left brackets.",br)
  if(bm>0)
    PERR("brackets not allowed if keyword \"all\".")
  if(fil__err_tagsbool!=0)
    PERR("Boolean operators must not be used in tags filter.")
  if(fil__err_tagsbracket!=0)
    PERR("brackets must not be used in tags filter.")

  // preprocess operator priority if keyword "all"
  for(ft= 0;ft<fil__pairtM;ft++) {  // for every filter type
    if(fil_meetall[ft]) {
      fp= f= fil__pair[ft]; fe= fil__paire[ft];
      while(fp<fe) {  // for every key/val pair in filter

        if(!fp[0].operator && fp<fe-1 &&
            (fp==f || fp[-1].operator)) {
            // change to 'or'
          fp[0].left_bracketn++;
            if(loglevel>=2)
              PINFOv("inserting[%i][%i]: \"(\"",
                ft,(int)(fp-fil__pair[ft]))
          }
        else if(!fp[-1].operator && fp>f &&
            (fp[0].operator || fp==fe-1)) {
            // change to 'and'
          fp[0].right_bracketn++;
            if(loglevel>=2)
              PINFOv("inserting[%i][%i]: \")\"",
                ft,(int)(fp-fil__pair[ft]))
          }
        fp++;
        }  // for every key/val pair in filter
      }
    }  // for every filter type

  return bl+br+bm*100+synt*1000+
    (fil__err_tagsbool+fil__err_tagsbracket)*10000;
  }  // fil_plausi()

static inline bool fil_check0(int otype,
    char** key,char** keye,char** val,char** vale) {
  // check if OSM object matches filter criteria;
  // at this procedure, filter type 0..2 is applied: 'keep object';
  // keyp,keye,valp,vale: tag list;
  // otype: 0: node; 1: way; 2: relation;
  // return: given tag list matches keep criteria;
  bool result,gotkey;
  char** keyp,**valp;
  fil__pair_t* fp,*fe;
  int bracket_balance;
  int bb;  // temporary for bracket_balance
  char* v;  // previous value of a key which compared successfully

  result= false;
  v= NULL;  // (default)
  valp= &v;  // (default)
  bracket_balance= 0;
  fp= fil__pair[otype]; fe= fil__paire[otype];
  while(fp<fe) {  // for every key/val pair in filter
    bracket_balance+= fp->left_bracketn;
    if(*(int16_t*)(fp->k)==0) {
      if(v!=NULL)
        result= fil__cmp(v,fp->v);
      }
    else {
      result= gotkey= false;  // (default)
      keyp= key; valp= val;
      while(keyp<keye) {  // for all key/val pairs of this object
        if(fil__cmp(*keyp,fp->k)) {  // right key
          gotkey= true;
          v= *valp;
          if(*(int16_t*)(fp->k)==0 || fil__cmp(v,fp->v)) {
            // right value
            result= true;
      break;
            }
          }
        keyp++; valp++;
        }  // for all key/val pairs of this object
      if(!gotkey) {  // did not find a matching key
        char c;

        c= *fp->v;
        if(c==1 || c==3)  // 'unequal' operator  2012-02-03
          result= true;  // accept this equation as fulfilled
        }
      }
    #if MAXLOGLEVEL>=3
      if(loglevel>=3)
        PINFOv("comparison[%i][%i]==%i",otype,fp-fil__pair[otype],result)
    #endif
    if(result) {  // comparison satisfied
      if(fp->operator) {  // Boolean operator is AND
        // (continue with next comparison)
        }  // Boolean operator is AND
      else {  // Boolean operator is OR
        // at each encountered 'or':
        // jump to after next operand at lower layer
        bracket_balance-= fp->right_bracketn;
        if(bracket_balance<=0)  // we already are at lowest level
return result;
        bb= bracket_balance;
        fp++;
        while(fp<fe) {
          bracket_balance+= fp->left_bracketn;
          bracket_balance-= fp->right_bracketn;
          if(bracket_balance>=bb) {  // same level or higher
            fp++;
        continue;
            }
          if(fp->operator) {  // next operator is 'and'
            fp++;
        break;  // go on by evaluating this operator
            }
          // here: next operator is an 'or'
          if(bracket_balance<=0)  // we are at lowest level
return result;
          bb= bracket_balance;  // from now on ignore this level
          fp++;
          }
        v= NULL;  // previous value no longer valid
  continue;
        }  // Boolean operator is OR
      }  // comparison satisfied
    else {  // comparison not satisfied
      if(fp->operator) {  // Boolean operator is AND
        // jump to after next 'or' within same brackets or
        // lower layer, but not into the space between new brackets
        bracket_balance-= fp->right_bracketn;
        bb= bracket_balance;
        fp++;
        while(fp<fe) {
          bracket_balance+= fp->left_bracketn;
          bracket_balance-= fp->right_bracketn;
          if(bracket_balance<bb)
            bb= bracket_balance;
          if(bracket_balance<=bb && !fp->operator) {
              // not in a new bracket AND next operator is 'or'
            fp++;
        break;
            }
          fp++;
          }
        v= NULL;  // previous value no longer valid
  continue;
        }  // Boolean operator is AND
      else {  // Boolean operator is OR
        // (continue with next comparison)
        }  // Boolean operator is OR
      }  // comparison not satisfied
    bracket_balance-= fp->right_bracketn;
    fp++;
    }  // for every key/val pair in filter
  return result;
  }  // end   fil_check0()

static inline bool fil_check1(int otype,
    char** key,char** keye,char** val,char** vale) {
  // check if OSM object matches filter criteria;
  // at this procedure, filter type 4..6 is applied: 'drop object';
  // keyp,keye,valp,vale: tag list;
  // otype: 0: node; 1: way; 2: relation;
  // return: given tag list matches keep criteria;
  bool result;
  char** keyp,**valp;
  fil__pair_t* fp,*fe;
  int bracket_balance;
  int bb;  // temporary for bracket_balance
  char* v;  // previous value of a key which compared successfully

  result= false;
  v= NULL;  // (default)
  valp= &v;  // (default)
  bracket_balance= 0;
  fp= fil__pair[3+otype]; fe= fil__paire[3+otype];
  while(fp<fe) {  // for every key/val pair in filter
    bracket_balance+= fp->left_bracketn;
    if(*(int16_t*)(fp->k)==0) {
      if(v!=NULL)
        result= fil__cmp(v,fp->v);
      }
    else {
      result= false;  // (default)
      keyp= key; valp= val;
      while(keyp<keye) {  // for all key/val pairs of this object
        if(fil__cmp(*keyp,fp->k)) {  // right key
          v= *valp;
          if(*(int16_t*)(fp->k)==0 || fil__cmp(v,fp->v)) {
            // right value
            result= true;
      break;
            }
          }
        keyp++; valp++;
        }  // for all key/val pairs of this object
      }
    #if MAXLOGLEVEL>=3
      if(loglevel>=3)
        PINFOv("comparison[%i][%i]==%i",
          3+otype,fp-fil__pair[3+otype],result)
    #endif
    if(result) {  // comparison satisfied
      if(fp->operator) {  // Boolean operator is AND
        // (continue with next comparison)
        }  // Boolean operator is AND
      else {  // Boolean operator is OR
        // at each encountered 'or':
        // jump to after next operand at lower layer
        bracket_balance-= fp->right_bracketn;
        if(bracket_balance<=0)  // we already are at lowest level
return result;
        bb= bracket_balance;
        fp++;
        while(fp<fe) {
          bracket_balance+= fp->left_bracketn;
          bracket_balance-= fp->right_bracketn;
          if(bracket_balance>=bb) {  // same level or higher
            fp++;
        continue;
            }
          if(fp->operator) {  // next operator is 'and'
            fp++;
        break;  // go on by evaluating this operator
            }
          // here: next operator is an 'or'
          if(bracket_balance<=0)  // we are at lowest level
return result;
          bb= bracket_balance;  // from now on ignore this level
          fp++;
          }
        v= NULL;  // previous value no longer valid
  continue;
        }  // Boolean operator is OR
      }  // comparison satisfied
    else {  // comparison not satisfied
      if(fp->operator) {  // Boolean operator is AND
        // jump to after next 'or' within same brackets or
        // lower layer, but not into the space between new brackets
        bracket_balance-= fp->right_bracketn;
        bb= bracket_balance;
        fp++;
        while(fp<fe) {
          bracket_balance+= fp->left_bracketn;
          bracket_balance-= fp->right_bracketn;
          if(bracket_balance<bb)
            bb= bracket_balance;
          if(bracket_balance<=bb && !fp->operator) {
              // not in a new bracket AND next operator is 'or'
            fp++;
        break;
            }
          fp++;
          }
        v= NULL;  // previous value no longer valid
  continue;
        }  // Boolean operator is AND
      else {  // Boolean operator is OR
        // (continue with next comparison)
        }  // Boolean operator is OR
      }  // comparison not satisfied
    bracket_balance-= fp->right_bracketn;
    fp++;
    }  // for every key/val pair in filter
  return result;
  }  // end   fil_check1()

static inline bool fil_check2(int otype,
    const char* key,const char* val) {
  // test if filter allows this tag to be kept;
  // at this procedure, filters type 6..8 and 9..11 are applied:
  // 'keep tag';
  // otype: 0: node; 1: way; 2: relation;
  // return: given key[] and val[] match keep criteria;
  fil__pair_t* fp,*fe;
  const char* k;  // last key in filter
  bool keymatch;

  // apply keep-filter
  if(fil_active[6+otype]) {
    k= "name";  // (default)
    keymatch= false;
    fp= &fil__pair[6+otype][0]; fe= fil__paire[6+otype];
    while(fp<fe) {
      if(*(int16_t*)(fp->k)!=0) k= fp->k;
      keymatch= fil__cmp(key,k);
      if(keymatch && (*(int16_t*)(fp->v)==0 || fil__cmp(val,fp->v)))
        goto keep;
      fp++;
      }
    if(keymatch || fil_meetall[6+otype])
return false;
    }
  keep:
  // apply drop-filter
  if(fil_active[9+otype]) {
    k= "name";  // (default)
    fp= &fil__pair[9+otype][0]; fe= fil__paire[9+otype];
    while(fp<fe) {
      if(*(int16_t*)(fp->k)!=0) k= fp->k;
      if(fil__cmp(key,k) && (*(int16_t*)(fp->v)==0 ||
          fil__cmp(val,fp->v)))
return false;
      fp++;
      }
    }
  return true;
  }  // end   fil_check2()

//------------------------------------------------------------
// end   Module fil_   osm filter module
//------------------------------------------------------------



//------------------------------------------------------------
// Module rr_   relref temporary module
//------------------------------------------------------------

// this module provides procedures to use a temporary file for
// storing relation's references;
// as usual, all identifiers of a module have the same prefix,
// in this case 'rr_'; one underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static char rr__filename[400]= "";
static int rr__fd= -1;  // file descriptor for temporary file
#define rr__bufM 400000
static int32_t rr__buf[rr__bufM],*rr__bufp,*rr__bufe,*rr__bufee;
  // buffer - used for write, and later for read;
static bool rr__writemode;  // buffer is used for writing

static void rr__flush() {
  if(!rr__writemode || rr__bufp==rr__buf)
return;
  UR(write(rr__fd,rr__buf,(char*)rr__bufp-(char*)rr__buf))
  rr__bufp= rr__buf;
  }  // end   rr__flush()

static inline void rr__write(int32_t i) {
  // write an int to tempfile, use a buffer;
  if(rr__bufp>=rr__bufee) rr__flush();
  *rr__bufp++= i;
  }  // end   rr__write()

static void rr__end() {
  // clean-up for temporary file access;
  // will be called automatically at program end;
  if(rr__fd>2) {
    close(rr__fd);
    rr__fd= -1;
    }
  if(loglevel<2) unlink(rr__filename);
  }  // end   rr__end()

//------------------------------------------------------------

static int rr_ini(const char* filename) {
  // open a temporary file with the given name for random r/w access;
  // return: ==0: ok; !=0: error;
  strcpy(stpmcpy(rr__filename,filename,sizeof(rr__filename)-2),".0");
  if(rr__fd>=0)  // file already open
return 0;  // ignore this call
  unlink(rr__filename);
  rr__fd= open(rr__filename,O_RDWR|O_CREAT|O_TRUNC|O_BINARY,00600);
  if(rr__fd<0) {
    PERRv("could not open temporary file: %.80s",rr__filename)
return 1;
    }
  atexit(rr__end);
  rr__bufee= rr__buf+rr__bufM;
  rr__bufp= rr__bufe= rr__buf;
  rr__writemode= true;
  return 0;
  }  // end   rr_ini()

static inline void rr_rel(int32_t relid) {
  // store the id of a relation in tempfile;
  rr__write(0);
  rr__write(relid);
  } // end   rr_rel()

static inline void rr_ref(int32_t refid) {
  // store the id of an interrelation reference in tempfile;
  rr__write(refid);
  } // end   rr_ref()

static int rr_rewind() {
  // rewind the file pointer;
  // return: ==0: ok; !=0: error;
  if(rr__writemode) {
    rr__flush(); rr__writemode= false; }
  if(lseek(rr__fd,0,SEEK_SET)<0) {
    PERR("could not rewind temporary file.");
return 1;
    }
  rr__bufp= rr__bufe= rr__buf;
  return 0;
  } // end   rr_rewind()

static int rr_read(int32_t* ip) {
  // read one integer; meaning of the values of these integers:
  // every value is an interrelation reference id, with one exception:
  // integers which follow a 0-integer directly are relation ids;
  // note that we take 32-bit-integers instead of the 64-bit-integers
  // we usually take for object ids; this is because the range of
  // relation ids will not exceed the 2^15 range in near future;
  // return: ==0: ok; !=0: eof;
  int r,r2;

  if(rr__bufp>=rr__bufe) {
    r= read(rr__fd,rr__buf,sizeof(rr__buf));
    if(r<=0)
return 1;
    rr__bufe= (int32_t*)((char*)rr__buf+r);
    if((r%4)!=0) { // odd number of bytes
      r2= read(rr__fd,rr__bufe,4-(r%4));  // request the missing bytes
      if(r2<=0)  // did not get the missing bytes
        rr__bufe= (int32_t*)((char*)rr__bufe-(r%4));
      else
        rr__bufe= (int32_t*)((char*)rr__bufe+r2);
      }
    rr__bufp= rr__buf;
    }
  *ip= *rr__bufp++;
  return 0;
  }  // end   rr_read()

//------------------------------------------------------------
// end   Module rr_   relref temporary module
//------------------------------------------------------------
  


//------------------------------------------------------------
// Module o5_   o5m conversion module
//------------------------------------------------------------

// this module provides procedures which convert data to
// o5m format;
// as usual, all identifiers of a module have the same prefix,
// in this case 'o5'; one underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static inline void stw_reset();

#define o5__bufM UINT64_C(5000000)
static byte* o5__buf= NULL;  // buffer for one object in .o5m format
static byte* o5__bufe= NULL;
  // (const) water mark for buffer filled nearly 100%
static byte* o5__bufp= NULL;
static byte* o5__bufr0= NULL,*o5__bufr1= NULL;
  // start end end mark of a reference area in o5__buf[];
  // ==NULL: no mark set;

// basis for delta coding
static int64_t o5_id;
static uint32_t o5_lat,o5_lon;
static int64_t o5_cset;
static int64_t o5_time;
static int64_t o5_ref[3];  // for node, way, relation

static inline void o5__resetvars(void) {
  // reset all delta coding counters;
  o5__bufp= o5__buf;
  o5__bufr0= o5__bufr1= o5__buf;
  o5_id= 0;
  o5_lat= o5_lon= 0;
  o5_cset= 0;
  o5_time= 0;
  o5_ref[0]= o5_ref[1]= o5_ref[2]= 0;
  stw_reset();
  }  // end   o5__resetvars()

static void o5__end() {
  // clean-up for o5 module;
  // will be called at program's end;
  if(o5__buf!=NULL) {
    free(o5__buf); o5__buf= NULL; }
  }  // end   o5__end()

//------------------------------------------------------------

static inline void o5_reset(void) {
  // perform and write an o5m Reset;
  o5__resetvars();
  write_char(0xff);  // write .o5m Reset
  }  // end   o5_reset()

static int o5_ini(void) {
  // initialize this module;
  // must be called before any other procedure is called;
  // return: 0: everything went ok;
  //         !=0: an error occurred;
  static bool firstrun= true;

  if(firstrun) {
    firstrun= false;
    o5__buf= (byte*)malloc(o5__bufM);
    if(o5__buf==NULL)
return 1;
    atexit(o5__end);
    o5__bufe= o5__buf+o5__bufM-400000;
    }
  o5__resetvars();
  return 0;
  }  // end   o5_ini()

static inline void o5_byte(byte b) {
  // write a single byte;
  // writing starts at position o5__bufp;
  // o5__bufp: incremented by 1;
  *o5__bufp++= b;
  }  // end   o5_byte()

static inline int o5_str(const char* s) {
  // write a zero-terminated string;
  // writing starts at position o5__bufp;
  // return: bytes written;
  // o5__bufp: increased by the number of written bytes;
  byte* p0;
  byte c;

  p0= o5__bufp;
  if(o5__bufp>=o5__bufe) {
    static int msgn= 1;
    if(--msgn>=0) {
    PERR(".o5m memory overflow.")
return 0;
      }
    }
  do *o5__bufp++= c= *s++;
    while(c!=0);
return o5__bufp-p0;
  }  // end   o5_str()

static inline int o5_uvar32buf(byte* p,uint32_t v) {
  // write an unsigned 32 bit integer as Varint into a buffer;
  // writing starts at position p;
  // return: bytes written;
  byte* p0;
  uint32_t frac;

  p0= p;
  frac= v&0x7f;
  if(frac==v) {  // just one byte
    *p++= frac;
return 1;
    }
  do {
    *p++= frac|0x80;
    v>>= 7;
    frac= v&0x7f;
    } while(frac!=v);
  *p++= frac;
return p-p0;
  }  // end   o5_uvar32buf()

static inline int o5_uvar32(uint32_t v) {
  // write an unsigned 32 bit integer as Varint;
  // writing starts at position o5__bufp;
  // return: bytes written;
  // o5__bufp: increased by the number of written bytes;
  byte* p0;
  uint32_t frac;

  if(o5__bufp>=o5__bufe) {
    static int msgn= 1;
    if(--msgn>=0) {
    PERR(".o5m memory overflow.")
return 0;
      }
    }
  p0= o5__bufp;
  frac= v&0x7f;
  if(frac==v) {  // just one byte
    *o5__bufp++= frac;
return 1;
    }
  do {
    *o5__bufp++= frac|0x80;
    v>>= 7;
    frac= v&0x7f;
    } while(frac!=v);
  *o5__bufp++= frac;
return o5__bufp-p0;
  }  // end   o5_uvar32()

static inline int o5_svar32(int32_t v) {
  // write a signed 32 bit integer as signed Varint;
  // writing starts at position o5__bufp;
  // return: bytes written;
  // o5__bufp: increased by the number of written bytes;
  byte* p0;
  uint32_t u;
  uint32_t frac;

  if(o5__bufp>=o5__bufe) {
    static int msgn= 1;
    if(--msgn>=0) {
    PERR(".o5m memory overflow.")
return 0;
      }
    }
  p0= o5__bufp;
  if(v<0) {
    u= -v;
    u= (u<<1)-1;
    }
  else
    u= v<<1;
  frac= u&0x7f;
  if(frac==u) {  // just one byte
    *o5__bufp++= frac;
return 1;
    }
  do {
    *o5__bufp++= frac|0x80;
    u>>= 7;
    frac= u&0x7f;
    } while(frac!=u);
  *o5__bufp++= frac;
return o5__bufp-p0;
  }  // end   o5_svar32()

static inline int o5_svar64(int64_t v) {
  // write a signed 64 bit integer as signed Varint;
  // writing starts at position o5__bufp;
  // return: bytes written;
  // o5__bufp: increased by the number of written bytes;
  byte* p0;
  uint64_t u;
  uint32_t frac;

  if(o5__bufp>=o5__bufe) {
    static int msgn= 1;
    if(--msgn>=0) {
    PERR(".o5m memory overflow.");
return 0;
      }
    }
  p0= o5__bufp;
  if(v<0) {
    u= -v;
    u= (u<<1)-1;
    }
  else
    u= v<<1;
  frac= u&0x7f;
  if(frac==u) {  // just one byte
    *o5__bufp++= frac;
return 1;
    }
  do {
    *o5__bufp++= frac|0x80;
    u>>= 7;
    frac= u&0x7f;
    } while(frac!=u);
  *o5__bufp++= frac;
return o5__bufp-p0;
  }  // end   o5_svar64()

static inline void o5_markref(int pos) {
  // mark reference area;
  // pos: ==0: start; ==1: end;
  //      0 is accepted only once per dataset; only the first
  //      request is valid;
  //      1 may be repeated, the last one counts;
  if(pos==0) {
    if(o5__bufr0==o5__buf) o5__bufr0= o5__bufp;
    }
  else
    o5__bufr1= o5__bufp;
  }  // end   o5_markref()

static void o5_type(int type) {
  // mark object type we are going to process now;
  // should be called every time a new object is started to be
  // written into o5_buf[];
  // type: object type; 0: node; 1: way; 2: relation;
  //       if object type hase changed, a 0xff byte ("reset")
  //       will be written;
  static int oldtype= -1;

  // process changes of object type
  if(type!=oldtype) {  // object type has changed
    oldtype= type;
    o5_reset();
    }
  oldtype= type;
  }  // end   o5_type()

static void o5_write() {
  // write o5__buf[] contents to standard output;
  // include object length information after byte 0 and include
  // ref area length information right before o5__bufr0 (if !=NULL);
  // if buffer is empty, this procedure does nothing;
  byte buftemp[30];
  int reflen;  // reference area length
  int len;  // object length

  // get some length information
  len= o5__bufp-o5__buf;
  if(len<=0) goto o5_write_end;
  reflen= 0;  // (default)
  if(o5__bufr1<o5__bufr0) o5__bufr1= o5__bufr0;
  if(o5__bufr0>o5__buf) {
      // reference area contains at least 1 byte
    reflen= o5__bufr1-o5__bufr0;
    len+= o5_uvar32buf(buftemp,reflen);
    }  // end   reference area contains at least 1 byte

  // write header
  if(--len>=0) {
    write_char(o5__buf[0]);
    write_mem(buftemp,o5_uvar32buf(buftemp,len));
    }

  // write body
  if(o5__bufr0==o5__buf)  // no reference area
    write_mem(o5__buf+1,o5__bufp-(o5__buf+1));
  else {  // valid reference area
    write_mem(o5__buf+1,o5__bufr0-(o5__buf+1));
    write_mem(buftemp,o5_uvar32buf(buftemp,reflen));
    write_mem(o5__bufr0,o5__bufp-o5__bufr0);
    }  // end   valid reference area

  // reset buffer pointer
  o5_write_end:
  o5__bufp= o5__buf;  // set original buffer pointer to buffer start
  o5__bufr0= o5__bufr1= o5__buf;  // clear reference area marks
  }  // end   o5_write()

//------------------------------------------------------------
// end   Module o5_   o5m conversion module
//------------------------------------------------------------



//------------------------------------------------------------
// Module stw_   string write module
//------------------------------------------------------------

// this module provides procedures for conversions from
// c formatted strings into referenced string data stream objects
// - and writing it to buffered standard output;
// as usual, all identifiers of a module have the same prefix,
// in this case 'stw'; one underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

#define stw__tabM 15000
#define stw__tabstrM 250  // must be < row size of stw__rab[]
#define stw__hashtabM 150001  // (preferably a prime number)
static char stw__tab[stw__tabM][256];
  // string table; see o5m documentation;
  // row length must be at least stw__tabstrM+2;
  // each row contains a double string; each of the two strings
  // is terminated by a zero byte, the lengths must not exceed
  // stw__tabstrM bytes in total;
static int stw__tabi= 0;
  // index of last entered element in string table
static int stw__hashtab[stw__hashtabM];
  // has table; elements point to matching strings in stw__tab[];
  // -1: no matching element;
static int stw__tabprev[stw__tabM],stw__tabnext[stw__tabM];
  // for to chaining of string table rows which match
  // the same hash value; matching rows are chained in a loop;
  // if there is only one row matching, it will point to itself;
static int stw__tabhash[stw__tabM];
  // has value of this element as a link back to the hash table;
  // a -1 element indicates that the string table entry is not used;

static inline int stw__hash(const char* s1,const char* s2) {
  // get hash value of a string pair;
  // s2: ==NULL: single string; this is treated as s2=="";
  // return: hash value in the range 0..(stw__hashtabM-1);
  // -1: the strings are longer than stw__tabstrM characters in total;
  uint32_t h;
  uint32_t c;
  int len;

  len= stw__tabstrM;
  h= 0;
  for(;;) {
    if((c= *s1++)==0 || --len<0) break; h+= c; 
    if((c= *s1++)==0 || --len<0) break; h+= c<<8;
    if((c= *s1++)==0 || --len<0) break; h+= c<<16;
    if((c= *s1++)==0 || --len<0) break; h+= c<<24;
    }
  if(s2!=NULL) for(;;) {
    if((c= *s2++)==0 || --len<0) break; h+= c; 
    if((c= *s2++)==0 || --len<0) break; h+= c<<8;
    if((c= *s2++)==0 || --len<0) break; h+= c<<16;
    if((c= *s2++)==0 || --len<0) break; h+= c<<24;
    }
  if(len<0)
return -1;
  h%= stw__hashtabM;
  return h;
  }  // end   stw_hash()

static inline int stw__getref(int stri,const char* s1,const char* s2) {
  // get the string reference of a string pair;
  // the strings must not have more than 250 characters in total
  // (252 including terminators), there is no check in this procedure;
  // stri: presumed index in string table (we got it from hash table);
  //       must be >=0 and <stw__tabM, there is no boundary check;
  // s2: ==NULL: it's not a string pair but a single string;
  // stw__hashnext[stri]: chain to further occurrences;
  // return: reference of the string;
  //         ==-1: this string is not stored yet
  int strie;  // index of last occurrence
  const char* sp,*tp;
  int ref;

  if(s2==NULL) s2="";
  strie= stri;
  do {
    // compare the string (pair) with the tab entry
    tp= stw__tab[stri];
    sp= s1;
    while(*tp==*sp && *tp!=0) { tp++; sp++; }
    if(*tp==0 && *sp==0) {
        // first string identical to first string in table
      tp++;  // jump over first string terminator
      sp= s2;
      while(*tp==*sp && *tp!=0) { tp++; sp++; }
      if(*tp==0 && *sp==0) {
          // second string identical to second string in table
        ref= stw__tabi-stri;
        if(ref<=0) ref+= stw__tabM;
return ref;
        }
      }  // end   first string identical to first string in table
    stri= stw__tabnext[stri];
    } while(stri!=strie);
  return -1;
  }  // end   stw__getref()

//------------------------------------------------------------

static inline void stw_reset() {
  // clear string table and string hash table;
  // must be called before any other procedure of this module
  // and may be called every time the string processing shall
  // be restarted;
  int i;

  stw__tabi= 0;
  i= stw__tabM;
  while(--i>=0) stw__tabhash[i]= -1;
  i= stw__hashtabM;
  while(--i>=0) stw__hashtab[i]= -1;
  }  // end   stw_reset()

static void stw_write(const char* s1,const char* s2) {
  // write a string (pair), e.g. key/val, to o5m buffer;
  // if available, write a string reference instead of writing the
  // string pair directly;
  // no reference is used if the strings are longer than
  // 250 characters in total (252 including terminators);
  // s2: ==NULL: it's not a string pair but a single string;
  int h;  // hash value
  int ref;

  /* try to find a matching string (pair) in string table */ {
    int i;  // index in stw__tab[]

    ref= -1;  // ref invalid (default)
    h= stw__hash(s1,s2);
    if(h>=0) {  // string (pair) short enough for the string table
      i= stw__hashtab[h];
      if(i>=0)  // string (pair) presumably stored already
        ref= stw__getref(i,s1,s2);
      }  // end   string (pair) short enough for the string table
    if(ref>=0) {  // we found the string (pair) in the table
      o5_uvar32(ref);  // write just the reference
return;
      }  // end   we found the string (pair) in the table
    else {  // we did not find the string (pair) in the table
      // write string data
      o5_byte(0); o5_str(s1);
      if(s2!=NULL) o5_str(s2);  // string pair, not a single string
      if(h<0)  // string (pair) too long,
          // cannot be stored in string table
return;
      }  // end   we did not find the string (pair) in the table
    }  // end   try to find a matching string (pair) in string table
  // here: there is no matching string (pair) in the table

  /* free new element - if still being used */ {
    int h0;  // hash value of old element

    h0= stw__tabhash[stw__tabi];
    if(h0>=0) {  // new element in string table is still being used
      // delete old element
      if(stw__tabnext[stw__tabi]==stw__tabi)
          // self-chain, i.e., only this element
        stw__hashtab[h0]= -1;  // invalidate link in hash table
      else {  // one or more other elements in chain
        stw__hashtab[h0]= stw__tabnext[stw__tabi];  // just to ensure
          // that hash entry does not point to deleted element
        // now unchain deleted element
        stw__tabprev[stw__tabnext[stw__tabi]]= stw__tabprev[stw__tabi];
        stw__tabnext[stw__tabprev[stw__tabi]]= stw__tabnext[stw__tabi];
        }  // end   one or more other elements in chain
      }  // end   next element in string table is still being used
    }  // end   free new element - if still being used

  /* enter new string table element data */ {
    char* sp;
    int i;

    sp= stpcpy0(stw__tab[stw__tabi],s1)+1;
      // write first string into string table
    if(s2==NULL)  // single string
      *sp= 0;  // second string must be written as empty string
        // into string table
    else
      stpcpy0(sp,s2);  // write second string into string table
    i= stw__hashtab[h];
    if(i<0)  // no reference in hash table until now
      stw__tabprev[stw__tabi]= stw__tabnext[stw__tabi]= stw__tabi;
        // self-link the new element;
    else {  // there is already a reference in hash table
      // in-chain the new element
      stw__tabnext[stw__tabi]= i;
      stw__tabprev[stw__tabi]= stw__tabprev[i];
      stw__tabnext[stw__tabprev[stw__tabi]]= stw__tabi;
      stw__tabprev[i]= stw__tabi;
      }
    stw__hashtab[h]= stw__tabi;  // link the new element to hash table
    stw__tabhash[stw__tabi]= h;  // backlink to hash table element
    // new element now in use; set index to oldest element
    if(++stw__tabi>=stw__tabM) {  // index overflow
      stw__tabi= 0;  // restart index
      if(loglevel>=2) {
        static int rs= 0;
        fprintf(stderr,
          "osmfilter: string table index restart %i\n",++rs);
        }
      }  // end   index overflow
    }  // end   enter new string table element data
  }  // end   stw_write()

//------------------------------------------------------------
// end   Module stw_   string write module
//------------------------------------------------------------



//------------------------------------------------------------
// Module str_   string read module
//------------------------------------------------------------

// this module provides procedures for conversions from
// strings which have been stored in data stream objects to
// c-formatted strings;
// as usual, all identifiers of a module have the same prefix,
// in this case 'str'; one underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

#define str__tabM (15000+4000)
  // +4000 because it might happen that an object has a lot of
  // key/val pairs or refroles which are not stored already;
#define str__tabstrM 250  // must be < row size of str__rab[]
typedef struct str__info_struct {
  // members of this structure must not be accessed
  // from outside this module;
  char tab[str__tabM][256];
    // string table; see o5m documentation;
    // row length must be at least str__tabstrM+2;
    // each row contains a double string; each of the two strings
    // is terminated by a zero byte, the logical lengths must not
    // exceed str__tabstrM bytes in total;
    // the first str__tabM lines of this array are used as
    // input buffer for strings;
  int tabi;  // index of last entered element in string table;
  int tabn;  // number of valid strings in string table;
  struct str__info_struct* prev;  // address of previous unit;
  } str_info_t;
str_info_t* str__infop= NULL;

static void str__end() {
  // clean-up this module;
  str_info_t* p;

  while(str__infop!=NULL) {
    p= str__infop->prev;
    free(str__infop);
    str__infop= p;
    }
  }  // end str__end()

//------------------------------------------------------------

static str_info_t* str_open() {
  // open an new string client unit;
  // this will allow us to process multiple o5m input files;
  // return: handle of the new unit;
  //         ==NULL: error;
  // you do not need to care about closing the unit(s);
  static bool firstrun= true;
  str_info_t* prev;

  prev= str__infop;
  str__infop= (str_info_t*)malloc(sizeof(str_info_t));
  if(str__infop==NULL) {
    PERR("could not get memory for string buffer.")
return NULL;
    }
  str__infop->tabi= 0;
  str__infop->tabn= 0;
  str__infop->prev= prev;
  if(firstrun) {
    firstrun= false;
    atexit(str__end);
    }
  return str__infop;
  }  // end   str_open()

#if 0  // unused at present
static void str_switch(str_info_t* sh) {
  // switch to another string unit
  // sh: string unit handle;
  str__infop= sh;
  }  // end str_switch()
#endif

static void str_reset() {
  // clear string table;
  // must be called before any other procedure of this module
  // and may be called every time the string processing shall
  // be restarted;
  str__infop->tabi= str__infop->tabn= 0;
  }  // end   str_reset()

static void str_read(byte** pp,char** s1p,char** s2p) {
  // read an o5m formatted string (pair), e.g. key/val, from
  // standard input buffer;
  // if got a string reference, resolve it, using an internal
  // string table;
  // no reference is used if the strings are longer than
  // 250 characters in total (252 including terminators);
  // pp: address of a buffer pointer;
  //     this pointer will be incremented by the number of bytes
  //     the converted protobuf element consumes;
  // s2p: ==NULL: read not a string pair but a single string;
  // return:
  // *s1p,*s2p: pointers to the strings which have been read;
  char* p;
  int len1,len2;
  int ref;
  bool donotstore;  // string has 'do not store flag'  2012-10-01

  p= (char*)*pp;
  if(*p==0) {  // string (pair) given directly
    p++;
    donotstore= false;
    #if 0  // not used because strings would not be transparent anymore
    if(*p==(char)0xff) {  // string has 'do-not-store' flag
      donotstore= true;
      p++;
      }  // string has 'do-not-store' flag
      #endif
    *s1p= p;
    len1= strlen(p);
    p+= len1+1;
    if(s2p==NULL) {  // single string
      if(!donotstore && len1<=str__tabstrM) {
          // single string short enough for string table
        stpcpy0(str__infop->tab[str__infop->tabi],*s1p)[1]= 0;
          // add a second terminator, just in case someone will try
          // to read this single string as a string pair later;
        if(++str__infop->tabi>=str__tabM) str__infop->tabi= 0;
        if(str__infop->tabn<str__tabM) str__infop->tabn++;
        }  // end   single string short enough for string table
      }  // end   single string
    else {  // string pair
      *s2p= p;
      len2= strlen(p);
      p+= len2+1;
      if(!donotstore && len1+len2<=str__tabstrM) {
          // string pair short enough for string table
        memcpy(str__infop->tab[str__infop->tabi],*s1p,len1+len2+2);
        if(++str__infop->tabi>=str__tabM) str__infop->tabi= 0;
        if(str__infop->tabn<str__tabM) str__infop->tabn++;
        }  // end   string pair short enough for string table
      }  // end   string pair
    *pp= (byte*)p;
    }  // end   string (pair) given directly
  else {  // string (pair) given by reference
    ref= pbf_uint32(pp);
    if(ref>str__infop->tabn) {  // string reference invalid
      WARNv("invalid .o5m string reference: %i->%i",
        str__infop->tabn,ref)
      *s1p= "(invalid)";
      if(s2p!=NULL)  // caller wants a string pair
        *s2p= "(invalid)";
      }  // end   string reference invalid
    else {  // string reference valid
      ref= str__infop->tabi-ref;
      if(ref<0) ref+= str__tabM;
      *s1p= str__infop->tab[ref];
      if(s2p!=NULL)  // caller wants a string pair
        *s2p= strchr(str__infop->tab[ref],0)+1;
      }  // end   string reference valid
    }  // end   string (pair) given by reference
  }  // end   str_read()

//------------------------------------------------------------
// end   Module str_   string read module
//------------------------------------------------------------



//------------------------------------------------------------
// Module wo_   write osm module
//------------------------------------------------------------

// this module provides procedures which write osm objects;
// it uses procedures from module o5_;
// as usual, all identifiers of a module have the same prefix,
// in this case 'wo'; one underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static int wo__format= 0;  // output format;
  // 0: o5m; 11: native XML; 12: pbf2osm; 13: Osmosis; 14: Osmium;
  // -1: write key list;
static bool wo__logaction= false;  // write action for change files,
  // e.g. "<create>", "<delete>", etc.
static char* wo__xmlclosetag= NULL;  // close tag for XML output;
static bool wo__xmlshorttag= false;
  // write the short tag ("/>") instead of the long tag;
#define wo__CLOSE {  /* close the newest written object; */ \
  if(wo__xmlclosetag!=NULL) { if(wo__xmlshorttag) write_str("\"/>"NL); \
    else write_str(wo__xmlclosetag); \
    wo__xmlclosetag= NULL; wo__xmlshorttag= false; } }
#define wo__CONTINUE {  /* continue an XML object */ \
  if(wo__xmlshorttag) { write_str("\">"NL); wo__xmlshorttag= false; \
      /* from now on: long close tag necessary; */ } }
static int wo__lastaction= 0;  // last action tag which has been set;
  // 0: no action tag; 1: "create"; 2: "modify"; 3: "delete";
  // this is used only in .osc files;

static inline void wo__author(int32_t hisver,int64_t histime,
    int64_t hiscset,uint32_t hisuid,const char* hisuser) {
  // write osm object author information;
  // hisver: version; 0: no author information is to be written
  //                     (necessary if o5m format);
  // histime: time (seconds since 1970)
  // hiscset: changeset
  // hisuid: uid
  // hisuser: user name
  // global_fakeauthor: the author contents will be faked that way
  //                     that the author data will be as short as
  //                     possible;
  // global fakeversion: same as global_fakeauthor, but for .osm
  //                     format: just the version will be written;
  // note that when writing o5m format, this procedure needs to be
  // called even if there is no author information to be written;
  if(global_fakeauthor|global_fakeversion) {
    hisver= 1; histime= 1; hiscset= 1; hisuid= 0; hisuser= "";
    }
  if(wo__format==0) {  // o5m
    if(hisver==0 || global_dropversion)  // no version number
      o5_byte(0x00);
    else {  // version number available
      o5_uvar32(hisver);
      if(global_dropauthor) histime= 0;
      o5_svar64(histime-o5_time); o5_time= histime;
      if(histime!=0) {
          // author information available
        o5_svar64(hiscset-o5_cset); o5_cset= hiscset;
        if(hisuid==0 || hisuser==NULL || hisuser[0]==0)
            // user identification not available
          stw_write("","");
        else {  // user identification available
          byte uidbuf[30];

          uidbuf[o5_uvar32buf(uidbuf,hisuid)]= 0;
          stw_write((const char*)uidbuf,hisuser);
          }  // end   user identification available
        }  // end   author information available
      }  // end   version number available
return;
    }  // end   o5m
  // here: XML format
  if(global_fakeversion) {
    write_str("\" version=\"1");
return;
    }
  if(hisver==0 || global_dropversion)  // no version number
return;
  switch(wo__format) {  // depending on output format
  case 11:  // native XML
    write_str("\" version=\""); write_uint32(hisver);
    if(histime!=0 && !global_dropauthor) {
      write_str("\" timestamp=\""); write_timestamp(histime);
      write_str("\" changeset=\""); write_uint64(hiscset);
      if(hisuid!=0 && hisuser[0]!=0) {  // user information available
        write_str("\" uid=\""); write_uint32(hisuid);
        write_str("\" user=\""); write_xmlstr(hisuser);
        }
      }
    break;
  case 12:  // pbf2osm XML
    write_str("\" version=\""); write_uint32(hisver);
    if(histime!=0 && !global_dropauthor) {
      write_str("\" changeset=\""); write_uint64(hiscset);
      if(hisuid!=0 && hisuser[0]!=0) {  // user information available
        write_str("\" user=\""); write_xmlstr(hisuser);
        write_str("\" uid=\""); write_uint32(hisuid);
        }
      write_str("\" timestamp=\""); write_timestamp(histime);
      }
    break;
  case 13:  // Osmosis XML
    write_str("\" version=\""); write_uint32(hisver);
    if(histime!=0 && !global_dropauthor) {
      write_str("\" timestamp=\""); write_timestamp(histime);
      if(hisuid!=0 && hisuser[0]!=0) {  // user information available
        write_str("\" uid=\""); write_uint32(hisuid);
        write_str("\" user=\""); write_xmlmnstr(hisuser);
        }
      write_str("\" changeset=\""); write_uint64(hiscset);
      }
    break;
  case 14:  // Osmium XML
    write_str("\" version=\""); write_uint32(hisver);
    if(histime!=0 && !global_dropauthor) {
      write_str("\" changeset=\""); write_uint64(hiscset);
      write_str("\" timestamp=\""); write_timestamp(histime);
      if(hisuid!=0 && hisuser[0]!=0) {  // user information available
        write_str("\" uid=\""); write_uint32(hisuid);
        write_str("\" user=\""); write_xmlstr(hisuser);
        }
      }
    break;
    }  // end   depending on output format
  if(global_outosh) {
    if(wo__lastaction==3)
      write_str("\" visible=\"false");
    else
      write_str("\" visible=\"true");
    }
  }  // end   wo__author()

static inline void wo__action(int action) {
  // set one of these action tags: "create", "modify", "delete";
  // write tags only if 'global_outosc' is true;
  // action: 0: no action tag; 1: "create"; 2: "modify"; 3: "delete";
  //         caution: there is no check for validity of this parameter;
  static const char* starttag[]=
    {"","<create>"NL,"<modify>"NL,"<delete>"NL};
  static const char* endtag[]=
    {"","</create>"NL,"</modify>"NL,"</delete>"NL};
  if(global_outosc && action!=wo__lastaction) {  // there was a change
    write_str(endtag[wo__lastaction]);  // end last action
    write_str(starttag[action]);  // start new action
    }
  wo__lastaction= action;
  }  // end   wo__action()

//------------------------------------------------------------

static void wo_start(int format,bool bboxvalid,
    int32_t x1,int32_t y1,int32_t x2,int32_t y2) {
  // start writing osm objects;
  // format: 0: o5m; 11: native XML;
  //         12: pbf2osm; 13: Osmosis; 14: Osmium;
  //         -1: write key list;
  // bboxvalid: the following bbox coordinates are valid;
  // x1,y1,x2,y2: bbox coordinates (base 10^-7);
  if(format<-1 || (format >0 && format<11) || format>14) format= 0;
  wo__format= format;
  wo__logaction= global_outosc || global_outosh;
  if(wo__format<0) {  // item list
    count_ini();
return;
    }
  if(wo__format==0) {  // o5m
    static const byte o5mfileheader[]= {0xff,0xe0,0x04,'o','5','m','2'};
    static const byte o5cfileheader[]= {0xff,0xe0,0x04,'o','5','c','2'};

    if(global_outo5c)
      write_mem(o5cfileheader,sizeof(o5cfileheader));
    else
      write_mem(o5mfileheader,sizeof(o5mfileheader));
    if(bboxvalid) {  // bbox has been supplied
      o5_byte(0xdb);  // border box
      o5_svar32(x1); o5_svar32(y1);
      o5_svar32(x2); o5_svar32(y2);
      o5_write();  // write this object
      }
return;
    }  // end   o5m
  // here: XML
  if(wo__format!=14)
    write_str("<?xml version=\'1.0\' encoding=\'UTF-8\'?>"NL);
  else  // Osmium XML
    write_str("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"NL);
  if(global_outosc)
      write_str("<osmChange version=\"0.6\"");
  else
      write_str("<osm version=\"0.6\"");
  switch(wo__format) {  // depending on output format
  case 11:  // native XML
    write_str(" generator=\"osmfilter "VERSION"\"");
    break;
  case 12:  // pbf2osm XML
    write_str(" generator=\"pbf2osm\"");
    break;
  case 13:  // Osmosis XML
    write_str(" generator=\"Osmosis 0.39\"");
    break;
  case 14:  // Osmium XML
    write_str(" generator="
      "\"Osmium (http://wiki.openstreetmap.org/wiki/Osmium)\"");
    break;
    }  // end   depending on output format
  write_str(">"NL);
  if(wo__format!=12) {  // bbox may be written
    if(bboxvalid) {  // borders are to be applied OR
        // bbox has been supplied
      if(wo__format==13) {  // Osmosis
        // <bound box="53.80000,10.50000,54.00000,10.60000"
        //  origin="0.40.1"/>
        write_str("  <bound box=\""); write_sfix7(y1);
        write_str(","); write_sfix7(x1);
        write_str(","); write_sfix7(y2);
        write_str(","); write_sfix7(x2);
        write_str("\" origin=\"0.40\"/>"NL);
        }  // Osmosis
      else {  // not Osmosis
        // <bounds minlat="53.8" minlon="10.5" maxlat="54."
        //  maxlon="10.6"/>
        write_str("\t<bounds minlat=\""); write_sfix7(y1);
        write_str("\" minlon=\""); write_sfix7(x1);
        write_str("\" maxlat=\""); write_sfix7(y2);
        write_str("\" maxlon=\""); write_sfix7(x2);
        write_str("\"/>"NL);
        }  // not Osmosis
      }
    }  // end   bbox may be written
  }  // end   wo_start()

static void wo_end() {
  // end writing osm objects;
  switch(wo__format) {  // depending on output format
  case -1:
    if(global_outsort)
      count_sort();
    count_write();
    break;
  case 0:  // o5m
    o5_write();  // write last object - if any
    write_char(0xfe);  // write o5m eof indicator
    break;
  case 11:  // native XML
  case 12:  // pbf2osm XML
  case 13:  // Osmosis XML
  case 14:  // Osmium XML
    wo__CLOSE
    wo__action(0);
    write_str(global_outosc? "</osmChange>"NL: "</osm>"NL);
    if(wo__format>=12)
      write_str("<!--End of emulated output.-->"NL);
    break;
    }  // end   depending on output format
  }  // end   wo_end()

static inline void wo_node(int64_t id,
    int32_t hisver,int64_t histime,int64_t hiscset,
    uint32_t hisuid,const char* hisuser,int32_t lon,int32_t lat) {
  // write osm node body;
  // id: id of this object;
  // hisver: version; 0: no author information is to be written
  //                     (necessary if o5m format);
  // histime: time (seconds since 1970)
  // hiscset: changeset
  // hisuid: uid
  // hisuser: user name
  // lon: latitude in 100 nanodegree;
  // lat: latitude in 100 nanodegree;
  if(wo__format==0) {  // o5m
    o5_write();  // write last object - if any
    o5_type(0);
    o5_byte(0x10);  // data set id for node
    o5_svar64(id-o5_id); o5_id= id;
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
    o5_svar32(lon-o5_lon); o5_lon= lon;
    o5_svar32(lat-o5_lat); o5_lat= lat;
return;
    }  // end   o5m
  if(wo__format<0)  // item list
return;
  wo__CLOSE
  if(wo__logaction)
    wo__action(hisver==1? 1: 2);
  switch(wo__format) {  // depending on output format
  case 11:  // native XML
    write_str("\t<node id=\""); write_sint64(id);
    write_str("\" lat=\""); write_sfix7(lat);
    write_str("\" lon=\""); write_sfix7(lon);
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
    wo__xmlclosetag= "\t</node>"NL;  // preset close tag
    break;
  case 12:  // pbf2osm XML
    write_str("\t<node id=\""); write_sint64(id);
    write_str("\" lat=\""); write_sfix7o(lat);
    write_str("\" lon=\""); write_sfix7o(lon);
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
    wo__xmlclosetag= "\t</node>"NL;  // preset close tag
    break;
  case 13:  // Osmosis XML
    write_str("  <node id=\""); write_sint64(id);
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
    write_str("\" lat=\""); write_sfix7(lat);
    write_str("\" lon=\""); write_sfix7(lon);
    wo__xmlclosetag= "  </node>"NL;  // preset close tag
    break;
  case 14:  // Osmium XML
    write_str("  <node id=\""); write_sint64(id);
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
    if(lon>=0) lon= (lon+5)/10;
    else lon= (lon-5)/10;
    if(lat>=0) lat= (lat+5)/10;
    else lat= (lat-5)/10;
    write_str("\" lon=\""); write_sfix6o(lon);
    write_str("\" lat=\""); write_sfix6o(lat);
    wo__xmlclosetag= "  </node>"NL;  // preset close tag
    break;
    }  // end   depending on output format
  wo__xmlshorttag= true;  // (default)
  }  // end   wo_node()

static inline void wo_way(int64_t id,
    int32_t hisver,int64_t histime,int64_t hiscset,
    uint32_t hisuid,const char* hisuser) {
  // write osm way body;
  // id: id of this object;
  // hisver: version; 0: no author information is to be written
  //                     (necessary if o5m format);
  // histime: time (seconds since 1970)
  // hiscset: changeset
  // hisuid: uid
  // hisuser: user name
  if(wo__format==0) {  // o5m
    o5_write();  // write last object - if any
    o5_type(1);
    o5_byte(0x11);  // data set id for way
    o5_svar64(id-o5_id); o5_id= id;
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
    o5_markref(0);
return;
    }  // end   o5m
  if(wo__format<0)  // item list
return;
  wo__CLOSE
  if(wo__logaction)
    wo__action(hisver==1? 1: 2);
  switch(wo__format) {  // depending on output format
  case 11:  // native XML
    write_str("\t<way id=\""); write_sint64(id);
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
    wo__xmlclosetag= "\t</way>"NL;  // preset close tag
    break;
  case 12:  // pbf2osm XML
    write_str("\t<way id=\""); write_sint64(id);
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
    wo__xmlclosetag= "\t</way>"NL;  // preset close tag
    break;
  case 13:  // Osmosis XML
  case 14:  // Osmium XML
    write_str("  <way id=\""); write_sint64(id);
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
    wo__xmlclosetag= "  </way>"NL;  // preset close tag
    break;
    }  // end   depending on output format
  wo__xmlshorttag= true;  // (default)
  }  // end   wo_way()

static inline void wo_relation(int64_t id,
    int32_t hisver,int64_t histime,int64_t hiscset,
    uint32_t hisuid,const char* hisuser) {
  // write osm relation body;
  // id: id of this object;
  // hisver: version; 0: no author information is to be written
  //                     (necessary if o5m format);
  // histime: time (seconds since 1970)
  // hiscset: changeset
  // hisuid: uid
  // hisuser: user name
  if(wo__format==0) {  // o5m
    o5_write();  // write last object - if any
    o5_type(2);
    o5_byte(0x12);  // data set id for relation
    o5_svar64(id-o5_id); o5_id= id;
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
    o5_markref(0);
return;
    }  // end   o5m
  if(wo__format<0)  // item list
return;
  wo__CLOSE
  if(wo__logaction)
    wo__action(hisver==1? 1: 2);
  switch(wo__format) {  // depending on output format
  case 11:  // native XML
    write_str("\t<relation id=\""); write_sint64(id);
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
    wo__xmlclosetag= "\t</relation>"NL;  // preset close tag
    break;
  case 12:  // pbf2osm XML
    write_str("\t<relation id=\""); write_sint64(id);
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
    wo__xmlclosetag= "\t</relation>"NL;  // preset close tag
    break;
  case 13:  // Osmosis XML
  case 14:  // Osmium XML
    write_str("  <relation id=\""); write_sint64(id);
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
    wo__xmlclosetag= "  </relation>"NL;  // preset close tag
    break;
    }  // end   depending on output format
  wo__xmlshorttag= true;  // (default)
  }  // end   wo_relation()

static void wo_delete(int otype,int64_t id,
    int32_t hisver,int64_t histime,int64_t hiscset,
    uint32_t hisuid,const char* hisuser) {
  // write osm delete request;
  // this is possible for o5m format only;
  // for any other output format, this procedure does nothing;
  // otype: 0: node; 1: way; 2: relation;
  // id: id of this object;
  // hisver: version; 0: no author information is to be written
  //                     (necessary if o5m format);
  // histime: time (seconds since 1970)
  // hiscset: changeset
  // hisuid: uid
  // hisuser: user name
  if(wo__format<0)  // item list
return;
  if(otype<0 || otype>2)
return;
  if(wo__format==0) {  // o5m (.o5c)
    o5_write();  // write last object - if any
    o5_type(otype);
    o5_byte(0x10+otype);  // data set id
    o5_svar64(id-o5_id); o5_id= id;
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
    }  // end   o5m (.o5c)
  else {  // .osm (.osc)
    wo__CLOSE
    if(wo__logaction)
      wo__action(3);
    if(wo__format>=13) write_str("  <"); else write_str("\t<");
    write_str(ONAME(otype));
    write_str(" id=\""); write_sint64(id);
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
    if(global_fakelonlat)
      write_str("\" lat=\"0\" lon=\"0");
    wo__xmlclosetag= "\"/>"NL;  // preset close tag
    wo__xmlshorttag= false;  // (default)
    wo__CLOSE  // write close tag
    }  // end   .osm (.osc)
  }  // end   wo_delete()

static inline void wo_noderef(int64_t noderef) {
  // write osm object node reference;
  if(wo__format==0) {  // o5m
    o5_svar64(noderef-o5_ref[0]); o5_ref[0]= noderef;
    o5_markref(1);
return;
    }  // end   o5m
  if(wo__format<0)  // item list
return;
  // here: XML format
  wo__CONTINUE
  switch(wo__format) {  // depending on output format
  case 11:  // native XML
  case 12:  // pbf2osm XML
    write_str("\t\t<nd ref=\""); write_sint64(noderef);
    write_str("\"/>"NL);
    break;
  case 13:  // Osmosis XML
  case 14:  // Osmium XML
    write_str("    <nd ref=\""); write_sint64(noderef);
    write_str("\"/>"NL);
    break;
    }  // end   depending on output format
  }  // end   wo_noderef()

static inline void wo_ref(int64_t refid,int reftype,
    const char* refrole) {
  // write osm object reference;
  if(wo__format==0) {  // o5m
    char o5typerole[4000];

    o5_svar64(refid-o5_ref[reftype]); o5_ref[reftype]= refid;
    o5typerole[0]= reftype+'0';
    strmcpy(o5typerole+1,refrole,sizeof(o5typerole)-1);
    stw_write(o5typerole,NULL);
    o5_markref(1);
return;
    }  // end   o5m
  if(wo__format<0)  // item list
return;
  // here: XML format
  wo__CONTINUE
  switch(wo__format) {  // depending on output format
  case 11:  // native XML
  case 12:  // pbf2osm XML
    if(reftype==0)
      write_str("\t\t<member type=\"node\" ref=\"");
    else if(reftype==1)
      write_str("\t\t<member type=\"way\" ref=\"");
    else
      write_str("\t\t<member type=\"relation\" ref=\"");
    write_sint64(refid);
    write_str("\" role=\""); write_xmlstr(refrole);
    write_str("\"/>"NL);
    break;
  case 13:  // Osmosis XML
  case 14:  // Osmium XML
    if(reftype==0)
      write_str("    <member type=\"node\" ref=\"");
    else if(reftype==1)
      write_str("    <member type=\"way\" ref=\"");
    else
      write_str("    <member type=\"relation\" ref=\"");
    write_sint64(refid);
    write_str("\" role=\""); write_xmlmnstr(refrole);
    write_str("\"/>"NL);
    break;
    }  // end   depending on output format
  }  // end   wo_ref()

static inline void wo_keyval(const char* key,const char* val) {
  // write osm object's keyval;
  if(wo__format==0) {  // o5m
    stw_write(key,val);
return;
    }  // end   o5m
  if(wo__format<0) {  // item list
    if(*(int16_t*)global_outkey==0)  // list keys
      count_add(key);
    else {  // list vals of one specific key
      if(fil__cmp(key,global_outkey))
          // this is the key of which we want its vals listed
        count_add(val);
      }
return;
    }
  // here: XML format
  wo__CONTINUE
  switch(wo__format) {  // depending on output format
  case 11:  // native XML
    write_str("\t\t<tag k=\""); write_xmlstr(key);
    write_str("\" v=\""); write_xmlstr(val);
    write_str("\"/>"NL);
    break;
  case 12:  // pbf2osm XML
    write_str("\t\t<tag k=\""); write_xmlstr(key);
    write_str("\" v=\""); write_xmlstr(val);
    write_str("\" />"NL);
    break;
  case 13:  // Osmosis XML
  case 14:  // Osmium XML
    write_str("    <tag k=\""); write_xmlmnstr(key);
    write_str("\" v=\""); write_xmlmnstr(val);
    write_str("\"/>"NL);
    break;
    }  // end   depending on output format
  }  // end   wo_keyval()

//------------------------------------------------------------
// end   Module wo_   write osm module
//------------------------------------------------------------



//------------------------------------------------------------
// Module oo_   osm to osm module
//------------------------------------------------------------

// this module provides procedures which read osm objects,
// process them and write them as osm objects, using module wo_;
// that goes for .osm format as well as for .o5m format;
// as usual, all identifiers of a module have the same prefix,
// in this case 'oo'; one underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static void oo__inverserrprocessing(int* maxrewindp) {
  // process temporary relation reference file;
  // the file must have been written; this procedure processes
  // the interrelation references of this file and updates
  // the hash table of module hash_ accordingly;
  // maxrewind: maximum number of rewinds;
  // return:
  // maxrewind: <0: maximum number of rewinds was not sufficient;
  // if there is no relation reference file, this procedure does
  // nothing;
  int changed;
    // number of relations whose flag has been changed, i.e.,
    // the recursive processing will continue;
    // if none of the relations' flags has been changed,
    // this procedure will end;
  int h;
  int32_t relid;  // relation id;
  int32_t refid;  // interrelation reference id;
  bool flag;

  h= 0;
  relid= 0; flag= false;
  while(*maxrewindp>=0) {  // for every recursion
    changed= 0;
    if(rr_rewind())  // could not rewind
  break;
    for(;;) {  // for every reference
      for(;;) {  // get next id
        if(rr_read(&refid))
          goto rewind;  // if at file end, rewind
        if(refid!=0)
      break;
        // here: a relation id will follow
        rr_read(&relid);  // get the relation id
        flag= hash_geti(2,relid);  // get flag of this relation
        }  // end   get next id
      if(!flag)  // flag of this relation has not been set
    continue;  // go on until next relation
      if(hash_relseti(refid))  // set flag of referenced relation;
          // flag of reference was not set
        changed++;  // memorize that we changed a flag
      }  // end   for every reference
    rewind:
    if(loglevel>0) fprintf(stderr,
      "Interrelational hierarchy %i: %i dependencies.\n",++h,changed);
    if(changed==0)  // no changes have been made in last recursion
  break;  // end the processing
    (*maxrewindp)--;
    }  // end   for every recursion
  }  // end   oo__inverserrprocessing()

static byte oo__whitespace[]= {
  0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,  // HT LF VT FF CR
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // SPC
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#define oo__ws(c) (oo__whitespace[(byte)(c)])
static byte oo__whitespacenul[]= {
  1,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,  // NUL HT LF VT FF CR
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,  // SPC /
  0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,  // <
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#define oo__wsnul(c) (oo__whitespacenul[(byte)(c)])
static byte oo__letter[]= {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#define oo__le(c) (oo__letter[(byte)(c)])

static const uint8_t* oo__hexnumber= (uint8_t*)
  // convert a hex character to a number
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x00\x00\x00\x00\x00\x00"
  "\x00\x0a\x0b\x0c\x0d\x0e\x0f\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x0a\x0b\x0c\x0d\x0e\x0f\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

static uint32_t oo__strtouint32(const char* s) {
  // read a number and convert it to an unsigned 64-bit integer;
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
  }  // end   oo__strtouint32()

#if 0  // presently unused
static int32_t oo__strtosint32(const char* s) {
  // read a number and convert it to a signed 64-bit integer;
  // return: number;
  int sign;
  int32_t i;
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
  }  // end   oo__strtosint32()
#endif

static int64_t oo__strtosint64(const char* s) {
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
  }  // end   oo__strtosint64()

static const int32_t oo__nildeg= 2000000000L;

static int32_t oo__strtodeg(char* s) {
  // read a number which represents a degree value and
  // convert it to a fixpoint number;
  // s[]: string with the number between -180 and 180,
  //      e.g. "-179.99", "11", ".222";
  // return: number in 10 millionth degrees;
  //         =='oo__nildeg': syntax error;
  static const long di[]= {10000000L,10000000L,1000000L,100000L,
    10000L,1000L,100L,10L,1L};
  static const long* dig= di+1;
  int sign;
  int d;  // position of decimal digit;
  long k;
  char c;

  if(*s=='-') { s++; sign= -1; } else sign= 1;
  if(!isdig(*s) && *s!='.')
return oo__nildeg;
  k= 0;
  d= -1;
  do {  // for every digit
    c= *s++;
    if(c=='.') { d= 0; continue; }  // start fractional part
    else if(!isdig(c) || c==0)
  break;
    k= k*10+c-'0';
    if(d>=0) d++;
    } while(d<7);  // end   for every digit
  k*= dig[d]*sign;
  return k;
  }  // end   oo__strtodeg()

static int64_t oo__strtimetosint64(const char* s) {
  // read a timestamp in OSM format, e.g.: "2010-09-30T19:23:30Z",
  // and convert it to a signed 64-bit integer;
  // return: time as a number (seconds since 1970);
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
  // use replcement for timegm() because Windows does not know it
    #if 0
    if(oo__tz==NULL) {
      oo__tz= getenv("TZ");
      putenv("TZ=");
      tzset();
      }
    return mktime(&tm);
    #endif
  return mktime(&tm)-timezone;
  #else
  return timegm(&tm);
  #endif
  }  // end   oo__strtimetosint64()

static void oo__xmltostr(char* s) {
  // read an xml string and convert is into a regular UTF-8 string,
  // for example: "Mayer&apos;s" -> "Mayer's";
  char* t;  // pointer in overlapping target string
  char c;
  uint32_t u;

  //char* s0; s0= s;
  for(;;) {  // for all characters, until first '&' or string end;
    c= *s;
    if(c==0)  // no character to convert
return;
    if(c=='&')
  break;
    s++;
    }
  //fprintf(stderr,"A %s\n",s0);//,,
  t= s;
  for(;;) {  // for all characters after the first '&'
    c= *s++;
    if(c==0)  // at the end of string
  break;
    if(c!='&') {
      *t++= c;
  continue;
      }
    c= *s;
    if(c=='#') {  // numeric value
      c= *++s;
      if(c=='x') {  // hex value
        s++;
        u= 0;
        for(;;) {
          c= *s++;
          if(c==';' || c==0)
        break;
          u= (u<<4)+oo__hexnumber[(byte)c];
          }
        }  // end   hex value
      else {  // decimal value
        u= 0;
        for(;;) {
          c= *s++;
          if(c==';' || c==0)
        break;
          u= u*10+c-'0';
          }
        }  // end   decimal value
      if(u<128)  // 1 byte sufficient
        *t++= (char)u;
      else if(u<2048) {  // 2 bytes sufficient
        *t++= (u>>6)|0xc0; *t++= (u&0x3f)|0x80; }
      else if(u<65536) {  // 3 bytes sufficient
        *t++= (u>>12)|0xe0; *t++= ((u>>6)&0x3f)|0x80;
        *t++= (u&0x3f)|0x80; }
      else {  // 4 bytes necessary
        *t++= ((u>>18)&0x07)|0xf0; *t++= ((u>>12)&0x3f)|0x80;
        *t++= ((u>>6)&0x3f)|0x80; *t++= (u&0x3f)|0x80; }
      }  // end   numeric value
    else if(strzcmp(s,"quot;")==0) {
      s+= 5; *t++= '\"'; }
    else if(strzcmp(s,"apos;")==0) {
      s+= 5; *t++= '\''; }
    else if(strzcmp(s,"amp;")==0) {
      s+= 4; *t++= '&'; }
    else if(strzcmp(s,"lt;")==0) {
      s+= 3; *t++= '<'; }
    else if(strzcmp(s,"gt;")==0) {
      s+= 3; *t++= '>'; }
    else {  // unknown escape code
      *t++= '&';
      }
    }  // end   for all characters after the first '&'
  *t= 0;  // terminate target string
  //fprintf(stderr,"Z %s\n",s0);sleep(1);//,,
  }  // end   oo__xmltostr()

static bool oo__xmlheadtag;  // currently, we are inside an xml start tag,
  // maybe a short tag, e.g. <node ... > or <node ... />
  // (the second example is a so-called short tag)
static char* oo__xmlkey,*oo__xmlval;  // return values of oo__xmltag

static bool oo__xmltag() {
  // read the next xml key/val and return them both;
  // due to performance reasons, global and module global variables
  // are used;
  // read_bufp: address at which the reading begins;
  // oo__xmlheadtag: see above;
  // return: no more xml keys/vals to read inside the outer xml tag;
  // oo__xmlkey,oo__xmlval: newest xml key/val which have been read;
  //                        "","": encountered the end of an
  //                               enclosed xml tag;
  char c;
  char xmldelim;

  for(;;) {  // until break
    while(!oo__wsnul(*read_bufp)) read_bufp++;
      // find next whitespace or null character or '/'
    while(oo__ws(*read_bufp)) read_bufp++;
      // find first character after the whitespace(s)
    c= *read_bufp;
    if(c==0) {
      oo__xmlkey= oo__xmlval= "";
return true;
      }
    else if(c=='/') {
      oo__xmlkey= oo__xmlval= "";
      c= *++read_bufp;
      read_bufp++;
      if(c=='>') {  // short tag ands here
        if(oo__xmlheadtag) {
            // this ending short tag is the object's tag
          oo__xmlheadtag= false;
return true;
          }
return false;
        }  // end   short tag ands here
  continue;
      }
    else if(c=='<') {
      oo__xmlheadtag= false;
      if(*++read_bufp=='/' && (
          (c= *++read_bufp)=='n' || c=='w' || c=='r') ) {
        // this has been long tag which is ending now
        while(!oo__wsnul(*read_bufp)) read_bufp++;
          // find next whitespace
        oo__xmlkey= oo__xmlval= "";
return true;
        }
  continue;
      }
    oo__xmlkey= (char*)read_bufp;
    while(oo__le(*read_bufp)) read_bufp++;
    if(*read_bufp!='=') {
      oo__xmlkey= "";
  continue;
      }
    *read_bufp++= 0;
    if(*read_bufp!='\"' && *read_bufp!='\'')
  continue;
    xmldelim= (char)*read_bufp;
    oo__xmlval= (char*)(++read_bufp);
    for(;;) {
      c= *read_bufp;
      if(c==xmldelim)
    break;
      if(c==0) {
      oo__xmlkey= oo__xmlval= "";
return true;
        }
      read_bufp++;
      }
    *read_bufp++= 0;
  break;
    }  // end   until break
  oo__xmltostr(oo__xmlkey);
  oo__xmltostr(oo__xmlval);
  return false;
  }  // end   oo__xmltag()

typedef struct {
  read_info_t* ri;  // file handles for input files
  int format;  // input file format;
    // ==-9: unknown; ==0: o5m; ==10: xml; ==-1: pbf;
  str_info_t* str;  // string unit handle (if o5m format)
  const char* filename;
  bool endoffile;
  int deleteobject;  // replacement for .osc <delete> tag
    // 0: not to delete; 1: delete this object; 2: delete from now on;
  int64_t o5id;  // for o5m delta coding
  int32_t o5lon,o5lat;  // for o5m delta coding
  int64_t o5histime;  // for o5m delta coding
  int64_t o5hiscset;  // for o5m delta coding
  int64_t o5rid[3];  // for o5m delta coding
  } oo__if_t;
static oo__if_t oo__if[global_fileM];
static oo__if_t* oo__ifp= oo__if;  // currently used element in oo__if[]
#define oo__ifI (oo__ifp-oo__if)  // index
static oo__if_t* oo__ife= oo__if;  // logical end of elements in oo__if[]
static oo__if_t* oo__ifee= oo__if+global_fileM;
  // physical end of oo_if[]
static int oo_ifn= 0;  // number of currently open files

static int oo__getformat() {
  // determine the formats of all opened files of unknown format
  // and store these determined formats;
  // do some intitialization for the format, of necessary;
  // oo__if[].format: !=-9: do nothing for this file;
  // return: 0: ok; !=0: error;
  //         5: too many pbf files;
  //            this is, because the module pbf (see above)
  //            does not have multi-client capabilities;
  // oo__if[].format: input file format; ==0: o5m; ==10: xml; ==-1: pbf;
  oo__if_t* ifptemp;
  byte* bufp;
  #define bufsp ((char*)bufp)  // for signed char

  ifptemp= oo__ifp;
  oo__ifp= oo__if;
  while(oo__ifp<oo__ife) {  // for all input files
    if(oo__ifp->ri!=NULL && oo__ifp->format==-9) {
        // format not yet determined
      read_switch(oo__ifp->ri);
      if(read_bufp>=read_bufe) {  // file empty
        PERRv("file empty: %.80s",oo__ifp->filename)
return 2;
        }
      bufp= read_bufp;
      if(bufp[0]==0 && bufp[1]==0 && bufp[2]==0 &&
          bufp[3]>8 && bufp[3]<20) {  // presumably .pbf format
        PERR("cannot process .pbf format.");
return 5;
        }
      else if(strzcmp(bufsp,"<?xml")==0 ||
          strzcmp(bufsp,"<osm")==0) {  // presumably .osm format
        oo__ifp->format= 10;
        }
      else if(bufp[0]==0xff && bufp[1]==0xe0 && (
          strzcmp(bufsp+2,"\x04""o5m2")==0 ||
          strzcmp(bufsp+2,"\x04""o5c2")==0 )) {
          // presumably .o5m format
        oo__ifp->format= 0;
        oo__ifp->str= str_open();
          // call some initialization of string read module
        }
      else if((bufp[0]==0xff && bufp[1]>=0x10 && bufp[1]<=0x12) ||
          (bufp[0]==0xff && bufp[1]==0xff &&
          bufp[2]>=0x10 && bufp[2]<=0x12) ||
          (bufp[0]==0xff && read_bufe==read_bufp+1)) {
          // presumably shortened .o5m format
        if(loglevel>=2)
          fprintf(stderr,"osmfilter: Not a standard .o5m file header "
            "%.80s\n",oo__ifp->filename);
        oo__ifp->format= 0;
        oo__ifp->str= str_open();
          // call some initialization of string read module
        }
      else {  // unknown file format
        PERRv("unknown file format: %.80s",oo__ifp->filename)
return 3;
        }
      }  // format not yet determined
    oo__ifp++;
    }  // for all input files
  oo__ifp= ifptemp;
  return 0;
  #undef bufsp
  }  // end oo__getformat()

static void oo__reset() {
  // reset counters for writing o5m files;
  if(oo__ifp->format==0) {  // o5m
    oo__ifp->o5id= 0;
    oo__ifp->o5lat= oo__ifp->o5lon= 0;
    oo__ifp->o5hiscset= 0;
    oo__ifp->o5histime= 0;
    oo__ifp->o5rid[0]= oo__ifp->o5rid[1]= oo__ifp->o5rid[2]= 0;
    str_reset();
    }  // o5m
  }  // oo__reset()

static bool oo__bbvalid= false;
  // the following bbox coordinates are valid;
static int32_t oo__bbx1= 0,oo__bby1= 0,oo__bbx2= 0,oo__bby2= 0;
  // bbox coordinates (base 10^-7);

static void oo__findbb() {
  // find border box in input file (if any);
  // return:
  // oo__bbvalid: following border box information is valid;
  // oo__bbx1 .. oo__bby2: border box coordinates;
  // read_bufp will not be changed;
  byte* bufp,*bufe;

  read_input();
  bufp= read_bufp; bufe= read_bufe;
  if(oo__ifp->format==0) {  // o5m
    byte b;  // latest byte which has been read
    int l;

    while(bufp<bufe) {  // for all bytes
      b= *bufp;
      if(b==0 || (b>=0x10 && b<=0x12))  // regular dataset id
return;
      if(b>=0xf0) {  // single byte dataset
        bufp++;
  continue;
        }  // end   single byte dataset
      // here: non-object multibyte dataset
      if(b==0xdb) {  // border box
        bufp++;
        l= pbf_uint32(&bufp);
        bufe= bufp+l;
        if(bufp<bufe) oo__bbx1= pbf_sint32(&bufp);
        if(bufp<bufe) oo__bby1= pbf_sint32(&bufp);
        if(bufp<bufe) oo__bbx2= pbf_sint32(&bufp);
        if(bufp<bufe) {
          oo__bby2= pbf_sint32(&bufp);
          oo__bbvalid= true;
          }
return;
        }  // border box
      bufp++;
      l= pbf_uint32(&bufp);  // jump over this dataset
      bufp+= l;  // jump over this dataset
      }  // end   for all bytes
    }  // end   o5m
  else {  // osm xml
    char* sp;
    char c1,c2,c3;  // next available characters

    while(bufp<bufe) {  // for all bytes
      sp= strchr((char*)bufp,'<');
      if(sp==NULL)
    break;
      c1= sp[1]; c2= sp[2]; c3= sp[3];
      if(c1=='n' && c2=='o' && c3=='d')
return;
      else if(c1=='w' && c2=='a' && c3=='y')
return;
      else if(c1=='r' && c2=='e' && c3=='l')
return;
      else if(c1=='b' && c2=='o' && c3=='u') {  // bounds
        // bounds may be supplied in one of these formats:
        // <bounds minlat="53.01104" minlon="8.481593"
        //   maxlat="53.61092" maxlon="8.990601"/>
        // <bound box="49.10868,6.35017,49.64072,7.40979"
        //   origin="http://www.openstreetmap.org/api/0.6"/>
        uint32_t bboxcomplete;  // flags for oo__bbx1 .. oo__bby2
        int l;
        char c;

        bboxcomplete= 0;
        sp++;  // jump over '<'
        for(;;) {  // jump over "bounds ", resp. "bound "
          c= *sp;
          if(oo__wsnul(c))
        break;
          sp++;
          }
        for(;;) {  // for every word in 'bounds'
          c= *sp;
          if(c=='/' || c=='>' || c==0)
        break;
          if(oo__ws(c) || c==',') {
            sp++;
        continue;
            }
          if((l= strzlcmp(sp,"box=\""))>0 ||
              (l= strzlcmp(sp,"box=\'"))>0) {
            sp+= l;
            c= *sp;
            }
          if((l= strzlcmp(sp,"minlat=\""))>0 ||
              (l= strzlcmp(sp,"minlat=\'"))>0 ||
              ((isdig(c) || c=='-' || c=='.') && (bboxcomplete&2)==0)) {
            sp+= l;
            oo__bby1= oo__strtodeg(sp);
            if(oo__bby1!=oo__nildeg) bboxcomplete|= 2;
            }
          else if((l= strzlcmp(sp,"minlon=\""))>0 ||
              (l= strzlcmp(sp,"minlon=\'"))>0 ||
              ((isdig(c) || c=='-' || c=='.') && (bboxcomplete&1)==0)) {
            sp+= l;
            oo__bbx1= oo__strtodeg(sp);
            if(oo__bbx1!=oo__nildeg) bboxcomplete|= 1;
            }
          else if((l= strzlcmp(sp,"maxlat=\""))>0 ||
              (l= strzlcmp(sp,"maxlat=\'"))>0 ||
              ((isdig(c) || c=='-' || c=='.') && (bboxcomplete&8)==0)) {
            sp+= l;
            oo__bby2= oo__strtodeg(sp);
            if(oo__bby2!=oo__nildeg) bboxcomplete|= 8;
            }
          else if((l= strzlcmp(sp,"maxlon=\""))>0 ||
              (l= strzlcmp(sp,"maxlon=\'"))>0 ||
              ((isdig(c) || c=='-' || c=='.') && (bboxcomplete&4)==0)) {
            sp+= l;
            oo__bbx2= oo__strtodeg(sp);
            if(oo__bbx2!=oo__nildeg) bboxcomplete|= 4;
            }
          for(;;) {  // find next blank or comma
            c= *sp;
            if(oo__wsnul(c) || c==',')
          break;
            sp++;
            }
          }  // end   for every word in 'bounds'
        oo__bbvalid= bboxcomplete==15;
return;
        }  // bounds
      else {
        bufp++;
    continue;
        }
      }  // end   for all bytes of the file
    }  // end   osm xml
  }  // end   oo__findbb()

static int oo__findpos() {
  // find input file positions of the starts of node, way
  // and relation sections;
  // oo__ifp->format: ==0: o5m; ==1: osm xml;
  // return: ==0: OK; !=0: error;
  // positions are stored via read_jump(o,false), whereas
  // o==0: node, o==1: way; o==2: relation;
  // note that each of these positions will be stored, even if there
  // is no object of the related type;
  // this procedure assumes that the read position in the file is at
  // byte 0 when being called; the caller may not expect the read
  // cursor to be at the relations' start when this function returns;
  // if there are no relations in the file, the read cursor will be
  // at or near the end of the file;
  int otype,otypeold;  // type of currently processed object;
    // -1: unknown; 0: node; 1: way; 2: relation;

  read_jump(0,false);  // start of nodes (default)
  read_jump(1,false);  // start of ways (default)
  read_jump(2,false);  // start of relations (default)
  if(oo__ifp->format==0) {  // o5m
    byte b;  // latest byte which has been read
    int l;
    bool reset;

    otypeold= -1;
    reset= true;  // (default for file start)
    while(read_bufp<read_bufe) {  // for all bytes of the file
      read_input();
      b= *read_bufp;
      if(b<0x10 || b>0x12) {  // not a regular dataset id
        if(b>=0xf0) {  // single byte dataset
          if(b==0xff)  // file start, resp. o5m reset
            reset= true;
          read_bufp++;
    continue;
          }  // end   single byte dataset
        // here: non-object multibyte dataset
        read_bufp++;
        l= pbf_uint32(&read_bufp);  // jump over this dataset
        read_bufp+= l;  // jump over this dataset
    continue;
        }  // end   not a regular dataset id
      otype= b&3;
      if(otype!=otypeold) {  // object type has changed
        if(!reset) {
          PERRv("no .o5m reset tag before first %s",ONAME(otype))
return 1;
          }
        read_jump(otype,false);  // store this position
        if(otype>=otypeold+2)
          read_jump(otype-1,false);
            // store this position for start of last object too
        if(otype>=otypeold+3)
          read_jump(otype-2,false);
            // store this position for start of object
            // before last object too
        if(otype==2)  // we are at start of relations
return 0;
        otypeold= otype;
        }  // object type has changed
      read_bufp++;
      l= pbf_uint32(&read_bufp);  // jump over this dataset
      read_bufp+= l;  // jump over this dataset
      reset= false;
      }  // end   for all bytes of the file
    }  // end   o5m
  else {  // osm xml
    char* sp;
    char c1,c2,c3;  // next available characters

    otypeold= -1;
    while(read_bufp<read_bufe) {  // for all bytes of the file
      sp= strchr((char*)read_bufp,'<');
      if(sp==NULL)
    break;
      if(sp+10>=(char*)read_bufe) {
          // too close to end of prefetched data
        read_bufp= (byte*)sp;
        read_input();
        sp= (char*)read_bufp;
        }
      c1= sp[1]; c2= sp[2]; c3= sp[3];
      if(c1=='n' && c2=='o' && c3=='d')
        otype= 0;
      else if(c1=='w' && c2=='a' && c3=='y')
        otype= 1;
      else if(c1=='r' && c2=='e' && c3=='l')
        otype= 2;
      else {
        read_bufp= (byte*)sp+1;
    continue;
        }
      read_bufp= (byte*)sp;
      if(otype!=otypeold) {  // object type has changed
        read_jump(otype,false);  // store this position
        if(otype>=otypeold+2)
          read_jump(otype-1,false);
            // store this position for start of last object too
        if(otype>=otypeold+3)
          read_jump(otype-2,false);
            // store this position for start of object
            // before last object too
        if(otype==2)  // we are at start of relations
return 0;
        otypeold= otype;
        }  // object type has changed
      read_bufp= (byte*)sp+1;
      }  // end   for all bytes of the file
    }  // end   osm xml
  if(otype<2)  // did not encounter any relations
    read_jump(2,false);  // set end position as start of relations
  if(otype<1)  // did not encounter any ways
    read_jump(1,false);  // set end position as start of ways
return 0;
  }  // end   oo__findpos()

static void oo__close() {
  // close an input file;
  // oo__ifp: handle of currently active input file;
  // if this file has already been closed, nothing happens;
  // after calling this procedure, the handle of active input file
  // will be invalid;
  if(oo__ifp!=NULL && oo__ifp->ri!=NULL) {
    if(!oo__ifp->endoffile  && oo_ifn>0)  // missing logical end of file
      fprintf(stderr,"osmfilter Warning: "
        "unexpected end of input file: %.80s\n",oo__ifp->filename);
    read_close(oo__ifp->ri);
    oo__ifp->ri= NULL;
    oo_ifn--;
    }
  oo__ifp= NULL;
  }  // end oo__close()

static void oo__end() {
  // clean-up this module;
  oo_ifn= 0;  // mark end of program;
    // this is used to supress warning messages in oo__close()
  while(oo__ife>oo__if) {
    oo__ifp= --oo__ife;
    oo__close();
    }
  oo_ifn= 0;
    #if 0
    if(oo__tz!=NULL) {  // time zone must be restored
      char s[256];

      snprintf(s,sizeof(s),"TZ=%s",oo__tz);
      putenv(s);
      tzset();
      oo__tz= NULL;
      }  // time zone must be restored
    #endif
  }  // end oo__end()

//------------------------------------------------------------

static bool oo_open(const char* filename) {
  // open an input file;
  // filename[]: path and name of input file;
  //             ==NULL: standard input;
  // return: 0: ok; 1: no appropriate input file;
  //         2: maximum number of input files exceeded;
  // the handle for the current input file oo__ifp is set
  // to the opened file;
  // after having opened all input files, call oo__getformat();
  // you do not need to care about closing the file;
  static bool firstrun= true;

  if(oo__ife>=oo__ifee) {
    PERR("too many input files.")
return 2;
    }
  if(read_open(filename)!=0)
return 1;
  oo__ife->ri= read_infop;
  oo__ife->str= NULL;
  oo__ife->format= -9;  // 'not yet determined'
  oo__ife->filename= filename;
  oo__ife->endoffile= false;
  oo__ife->deleteobject= 0;
  oo__ifp= oo__ife++;
  oo_ifn++;
  if(firstrun) {
    firstrun= false;
    atexit(oo__end);
    }
  return 0;
  }  // end   oo_open()

static int oo_sequencetype= -1;
  // type of last object which has been processed;
  // -1: no object yet; 0: node; 1: way; 2: relation;
static int64_t oo_sequenceid= INT64_C(-0x7fffffffffffffff);
  // id of last object which has been processed;

static int oo_main() {
  // start reading osm objects;
  // return: ==0: ok; !=0: error;
  // this procedure must only be called once;
  // before calling this procedure you must open an input file
  // using oo_open();
  int wformat;  // output format
    // 0: o5m; 11: osm; 12: pbf2osm emulation; 13: Osmosis emulation;
    // 21: output key list;
  int filterstage;
    // stage of the processing of interrelation dependencies;
    // 0: search for start of ways and start of relations;
    //    change to stage 1 as soon as encountered the first relation;
    // 1: write interrelation references into a tempfile;
    //    apply filter and update hash flags for relations;
    //    change to stage 2 as soon as end of file has been reached;
    // 1->2: process interrelation dependencies,
    //       jump to start of relations;
    // 2: parse relations and update hash flags of dependent objects;
    //    change to stage 3 as soon as last relation has been parsed;
    // 2->3: jump to start of ways;
    // 3: parse ways and update hash flags of dependent nodes;
    //    change to stage 3 as soon as last way has been parsed;
    // 3->4: jump to start of nodes;
    // 4: process the whole file;
  static char o5mtempfile[400];  // must be static because
    // this file will be deleted by an at-exit procedure;
  #define oo__maxrewindINI 12
  int maxrewind;  // maximum number of relation-relation dependencies
  bool writeheader;  // header must be written
  int otype;  // type of currently processed object;
    // 0: node; 1: way; 2: relation;
  uint32_t complete;  // flags for valid data
  int64_t id;  // flag mask 1 (see oo__if_t)
  int32_t lon,lat;  // flag masks 2, 4 (see oo__if_t)
  uint32_t hisver;  // flag mask 8
  int64_t histime;  // flag mask 16 (see oo__if_t)
  int64_t hiscset;  // flag mask 32 (see oo__if_t)
  uint32_t hisuid;  // flag mask 64
  char* hisuser;  // flag mask 128
  // int64_t rid[3];  // for delta-coding (see oo__if_t)
  #define oo__refM 100000
  int64_t refid[oo__refM];
  int64_t* refidee;  // end address of array
  int64_t* refide,*refidp;  // pointer in array
  byte reftype[oo__refM];
  byte* reftypee,*reftypep;  // pointer in array
  char* refrole[oo__refM];
  char** refrolee,**refrolep;  // pointer in array
  #define oo__keyvalM 8000
  char* key[oo__keyvalM],*val[oo__keyvalM];
  char** keyee;  // end address of array
  char** keye,**keyp;  // pointer in array
  char** vale,**valp;  // pointer in array
  char** keyf,**valf;  // same as keye, vale, but for filter procedure;
  byte* bufp;  // pointer in read buffer
  #define bufsp ((char*)bufp)  // for signed char
  byte* bufe;  // pointer in read buffer, end of object
  char c;  // latest character which has been read
  byte b;  // latest byte which has been read
  int l;
  byte* bp;
  char* sp;

  // procedure initialization
  atexit(oo__end);
  filterstage= 0;
    // 0: search for start of ways and start of relations;
  maxrewind= oo__maxrewindINI;
  if(rr_ini(global_tempfilename))
return 4;
  writeheader= true;
  if(global_outo5m) wformat= 0;
  else if(global_emulatepbf2osm) wformat= 12;
  else if(global_emulateosmosis) wformat= 13;
  else if(global_emulateosmium) wformat= 14;
  else if(global_outkey!=NULL) wformat= -1;
  else wformat= 11;
  refidee= refid+oo__refM;
  keyee= key+oo__keyvalM-5;  // decremented because we are going to add
    // some header information for filtering;
  // get input file format and care about tempfile name
  if(oo__getformat())
return 5;
  strcpy(stpmcpy(o5mtempfile,global_tempfilename,
    sizeof(o5mtempfile)-2),".0");

  oo__findbb();
  if(global_recursive) {
    // find file positions of node, way and relation section's starts
    // here: filterstage==0
    // 0: search for start of ways and start of relations;
    if(oo__findpos())
  return 6;
    filterstage= 1;
      // 1: write interrelation references into a tempfile;
    }
  else
    filterstage= 4;
      // 4: process the whole file;

  // process the file
  for(;;) {  // read input file

    // get next object
    read_input();

    // care about recursive processing
    if(read_bufp>=read_bufe) {
        // at end of input file;
      if(filterstage==1) {
          // 1: write interrelation references into a tempfile;
        // 1->2: process interrelation dependencies,
        oo__inverserrprocessing(&maxrewind);
        if(read_jump(1,true))  // jump to start of ways
return 14;
        if(oo__ifp->format==0) oo__reset();
        filterstage= 2;  // 2: parse relations and update hash flags;
  continue;
        }
      if(filterstage==2) {
          // 2: parse relations and update hash flags;
        if(read_jump(1,true))  // jump to start of ways
return 15;
        if(oo__ifp->format==0) oo__reset();
        filterstage= 3;  // 3: parse ways and update hash flags;
  continue;
        }
      if(filterstage==3) {
          // 3: parse ways and update hash flags;
          // (we did not expect eof here)
        if(read_jump(0,true))  // jump to start of nodes
return 16;
        if(oo__ifp->format==0) oo__reset();
        filterstage= 4;  // 4: process the whole file;
  continue;
        }
      // here: filterstage==4  // 4: process the whole file;
      if(loglevel>0) {
        if(global_recursive)
          fprintf(stderr,
            "osmfilter: Relation hierarchies: %i of maximal %i.\n",
            oo__maxrewindINI-maxrewind,oo__maxrewindINI);
        else
          fprintf(stderr,"osmfilter: No hierarchical filtering.\n");
        }
      if(maxrewind<0)
        fprintf(stderr,
          "osmfilter Warning: relation dependencies too complex\n"
          "         (more than %i hierarchy levels).\n"
          "         A few relations might have been excluded\n"
          "         although meeting filter criteria.\n",
          oo__maxrewindINI);
        oo__close();
  break;
      }  // end   at end of input file
    if(oo__ifp->endoffile) {  // after logical end of file
      fprintf(stderr,"osmfilter Warning: unexpected contents "
        "after logical end of file.\n");
  break;
      }
    bufp= read_bufp;
    b= *bufp; c= (char)b;

    // care about header and unknown objects
    if(oo__ifp->format==0) {  // o5m
      if(b<0x10 || b>0x12) {  // not a regular dataset id
        if(b>=0xf0) {  // single byte dataset
          if(b==0xff)  // file start, resp. o5m reset
            oo__reset();
          else if(b==0xfe) {
            if(filterstage==4)
              oo__ifp->endoffile= true;
            }
          else if(write_testmode)
            WARNv("unknown .o5m short dataset id: 0x%02x",b)
          read_bufp++;
  continue;
          }  // end   single byte dataset
        else {  // unknown multibyte dataset
          if(write_testmode && b!=0xe0 && b!=0xdb && b!=0xdc)
                WARNv("unknown .o5m dataset id: 0x%02x",b)
          read_bufp++;
          l= pbf_uint32(&read_bufp);  // jump over this dataset
          read_bufp+= l;  // jump over this dataset
  continue;
          }  // end   unknown multibyte dataset
        }  // end   not a regular dataset id
      otype= b&3;
      }  // end   o5m
    else {  // xml
      while(c!=0 && c!='<') c= (char)*++bufp;
      if(c==0) {
        read_bufp= read_bufe;
  continue;
        }
      c= bufsp[1];
      if(c=='n' && bufsp[2]=='o' && bufsp[3]=='d')  // node 2012-12-13
        otype= 0;
      else if(c=='w' && bufsp[2]=='a' && bufsp[3]=='y')  // way
        otype= 1;
      else if(c=='r' && bufsp[2]=='e' && bufsp[3]=='l')  // relation
        otype= 2;
      else if(c=='c' || (c=='m' && bufsp[2]=='o') || c=='d') {
          // create, modify or delete
        if(c=='d')
          oo__ifp->deleteobject= 2;
        read_bufp= bufp+1;
  continue;
        }   // end   create, modify or delete
      else if(c=='/') {  // xml end object
        if(bufsp[2]=='d')  // end of delete
          oo__ifp->deleteobject= 0;
        else if(strzcmp(bufsp+2,"osm>")==0) {  // end of file
          if(filterstage==4)
            oo__ifp->endoffile= true;
          read_bufp= bufp+6;
          while(oo__ws(*read_bufp)) read_bufp++;
  continue;
          }   // end   end of file
        goto unknownxmlobject;
        }   // end   xml end object
      else {  // unknown xml object
        unknownxmlobject:
        bufp++;
        for(;;) {  // find end of xml object
          c= *bufsp;
          if(c=='>' || c==0)
        break;
          bufp++;
          }
        read_bufp= bufp;
  continue;
        }  // end   unknown XML object
      read_bufp= bufp;
      }  // end   xml

    // care about filterstage changes
    if(filterstage==3 && otype==2) {
        // 3: parse ways and update hash flags;
        // here: encountered the first relation;
      // 3->4: jump to start of nodes;
      if(read_jump(0,true))
return 18;
      if(oo__ifp->format==0) oo__reset();
      filterstage= 4;  // 4: process the whole file;
  continue;
      }

    // write header
    if(writeheader) {
      writeheader= false;
      wo_start(wformat,oo__bbvalid && oo_ifn==1,
        oo__bbx1,oo__bby1,oo__bbx2,oo__bby2);
      }

    // object initialization
    complete= 0;
    hisver= 0;
    histime= 0;
    hiscset= 0;
    hisuid= 0;
    hisuser= "";
    refide= refid;
    reftypee= reftype;
    refrolee= refrole;
    keye= key;
    vale= val;
    if(oo__ifp->deleteobject==1) oo__ifp->deleteobject= 0;

    // read one osm object
    if(oo__ifp->format==0) {  // o5m
      // read object id
      bufp++;
      l= pbf_uint32(&bufp);
      read_bufp= bufe= bufp+l;
      id= oo__ifp->o5id+= pbf_sint64(&bufp);
      // read author
      hisver= pbf_uint32(&bufp);
      if(hisver!=0) {  // author information available
        if(!global_dropversion) complete|= 8;
        histime= oo__ifp->o5histime+= pbf_sint64(&bufp);
        if(histime!=0) {
          hiscset= oo__ifp->o5hiscset+= pbf_sint32(&bufp);
          str_read(&bufp,&sp,&hisuser);
          hisuid= pbf_uint64((byte**)&sp);
          if(!global_dropauthor) complete|= 16+32+64+128;
          }
        }  // end   author information available
      if(bufp>=bufe)
          // just the id and author, i.e. this is a delete request
        oo__ifp->deleteobject= 1;
      else {  // not a delete request
        oo__ifp->deleteobject= 0;
        // read coordinates (for nodes only)
        if(otype==0) {  // node
          // read node body
          lon= oo__ifp->o5lon+= pbf_sint32(&bufp);
          lat= oo__ifp->o5lat+= pbf_sint32(&bufp);
          }  // end   node
        complete|= 1+2+4;
        // read noderefs (for ways only)
        if(otype==1) {  // way
          l= pbf_uint32(&bufp);
          bp= bufp+l;
          if(bp>bufe) bp= bufe;  // (format error)
          while(bufp<bp && refide<refidee)
            *refide++= oo__ifp->o5rid[0]+= pbf_sint64(&bufp);
          }  // end   way
        // read refs (for relations only)
        else if(otype==2) {  // relation
          int64_t ri;  // temporary, refid
          int rt;  // temporary, reftype
          char* rr;  // temporary, refrole

          l= pbf_uint32(&bufp);
          bp= bufp+l;
          if(bp>bufe) bp= bufe;  // (format error)
          while(bufp<bp && refide<refidee) {
            ri= pbf_sint64(&bufp);
            str_read(&bufp,&rr,NULL);
            *reftypee++= rt= (*rr++ -'0')%3;  // (suppress errors)
            *refide++= oo__ifp->o5rid[rt]+= ri;
            *refrolee++= rr;
            }
          }  // end   relation
        // read node key/val pairs
        keye= key; vale= val;
        while(bufp<bufe && keye<keyee)
          str_read(&bufp,keye++,vale++);
        }  // end   not a delete request
      }  // end   o5m
    else {  // osm xml
      int64_t ri;  // temporary, refid, rcomplete flag 1
      int rt;  // temporary, reftype, rcomplete flag 2
      char* rr;  // temporary, refrole, rcomplete flag 3
      int rcomplete;
      char* k;  // temporary, key
      char* v;  // temporary, val
      int r;

      read_bufp++;  // jump over '<'
      oo__xmlheadtag= true;  // (default)
      rcomplete= 0;
      k= v= NULL;
      for(;;) {  // until break;
        r= oo__xmltag();
        if(oo__xmlheadtag) {  // still in object header
          if(oo__xmlkey[0]=='i') { // id
            id= oo__strtosint64(oo__xmlval); complete|= 1; }
          else if(oo__xmlkey[0]=='l') {  // letter l
            if(oo__xmlkey[1]=='o') { // lon
              lon= oo__strtodeg(oo__xmlval); complete|= 2; }
            else if(oo__xmlkey[1]=='a') { // lon
              lat= oo__strtodeg(oo__xmlval); complete|= 4; }
            }  // end   letter l
          else if(oo__xmlkey[0]=='v' && oo__xmlkey[1]=='i') {  // visible
            if(oo__xmlval[0]=='f' || oo__xmlval[0]=='n')
              if(oo__ifp->deleteobject==0)
                oo__ifp->deleteobject= 1;
            }  // end   visible
          else if(oo__xmlkey[0]=='a' && oo__xmlkey[1]=='c') {  // action
            if(oo__xmlval[0]=='d' && oo__xmlval[1]=='e')
              if(oo__ifp->deleteobject==0)
                oo__ifp->deleteobject= 1;
            }  // end   action
          else if(!global_dropversion) {  // version not to drop
            if(oo__xmlkey[0]=='v' && oo__xmlkey[1]=='e') { // hisver
              hisver= oo__strtouint32(oo__xmlval); complete|= 8; }
            if(!global_dropauthor) {  // author not to drop
              if(oo__xmlkey[0]=='t') { // histime
                histime= oo__strtimetosint64(oo__xmlval); complete|= 16; }
              else if(oo__xmlkey[0]=='c') { // hiscset
                hiscset= oo__strtosint64(oo__xmlval); complete|= 32; }
              else if(oo__xmlkey[0]=='u' && oo__xmlkey[1]=='i') {// hisuid
                hisuid= oo__strtouint32(oo__xmlval); complete|= 64; }
              else if(oo__xmlkey[0]=='u' && oo__xmlkey[1]=='s') {//hisuser
                hisuser= oo__xmlval; complete|= 128; }
              }  // end   author not to drop
            }  // end   version not to drop
          }  // end   still in object header
        else {  // in object body
          if(oo__xmlkey[0]==0) {  // xml tag completed
            if(rcomplete>=3) {  // at least refid and reftype
              *refide++= ri;
              *reftypee++= rt;
              if(rcomplete<4)  // refrole is missing
                rr= "";  // assume an empty string as refrole
              *refrolee++= rr;
              }  // end   at least refid and reftype
            rcomplete= 0;
            if(v!=NULL && k!=NULL) {  // key/val available
              *keye++= k; *vale++= v;
              k= v= NULL;
              }  // end   key/val available
            }  // end   xml tag completed
          else {  // inside xml tag
            if(otype!=0 && refide<refidee) {
                // not a node AND still space in refid array
              if(oo__xmlkey[0]=='r' && oo__xmlkey[1]=='e') { // refid
                ri= oo__strtosint64(oo__xmlval); rcomplete|= 1;
                if(otype==1) {rt= 0; rcomplete|= 2; } }
              else if(oo__xmlkey[0]=='t' && oo__xmlkey[1]=='y') {
                  // reftype
                rt= oo__xmlval[0]=='n'? 0: oo__xmlval[0]=='w'? 1: 2;
                rcomplete|= 2; }
              else if(oo__xmlkey[0]=='r' && oo__xmlkey[1]=='o') {
                  // refrole
                rr= oo__xmlval; rcomplete|= 4; }
              }  // end   still space in refid array
            if(keye<keyee) {  // still space in key/val array
              if(oo__xmlkey[0]=='k') // key
                k= oo__xmlval;
              else if(oo__xmlkey[0]=='v') // val
                v= oo__xmlval;
              }  // end   still space in key/val array
            }  // end   inside xml tag
          }  // end   in object body
        if(r)
      break;
        }  // end   until break;
      }  // end   xml

    // care about possible array overflows
    if(refide>refidee)
      WARNv("way %"PRIi64" has too many noderefs.",id)
    if(refide>refidee)
      WARNv("relation %"PRIi64" has too many refs.",id)
    if(keye>=keyee)
      WARNv("%s %"PRIi64" has too many key/val pairs.",
        ONAME(otype),id)

    // check sequence, if in right filterstage
    if(filterstage==4) {
      if(otype<=oo_sequencetype &&
          (otype<oo_sequencetype || id<oo_sequenceid ||
          (oo_ifn>1 && id<=oo_sequenceid)))
        WARNv("wrong sequence at %s %"PRIi64,ONAME(otype),id)
      oo_sequencetype= otype; oo_sequenceid= id;
      }

    // (process object deletion - moved downward)

    // write interrelation dependencies into temporary file
    if(filterstage==1 && otype==2) {  // in stage 1 AND have relation
      int64_t ri;  // temporary, refid
      int rt;  // temporary, reftype
      bool idwritten;

      idwritten= false;
      refidp= refid; reftypep= reftype;
      while(refidp<refide) {  // for every referenced object
        ri= *refidp;
        rt= *reftypep;
        if(rt==2) {  // referenced object is a relation
          if(!idwritten) {  // did not yet write our relation's id
            rr_rel(id);  // write it now
            idwritten= true;
            }
          rr_ref(ri);
          }
        refidp++; reftypep++;
        }  // end   for every referenced object
      }  // in stage 1

    // prepare author information for filtering:
    // simply add them as key/val tags;
    keyf= keye; valf= vale;
    if(fil_filterheader) {
      static char ids[30],uids[30];

      *keyf++= "@id";
      *valf++= int64toa(id,ids);
      *keyf++= "@uid";
      *valf++= int32toa(hisuid,uids);
      *keyf++= "@user";
      *valf++= hisuser;
      }

    /* care about filtering, dependent on the filterstage */ {
      bool flag;  // flag for this object;
      bool keep;  // keep this object;

      keep= flag= hash_geti(otype,id);
      if(!keep && (filterstage!=4 || otype==0 || !global_recursive)) {
          // object not already to keep AND
          // (in stage !=4, or if object is a node)
        if(!fil_active[otype])
          keep= true;
        else  // filter 'keep object' shall be applied
          keep= fil_check0(otype,key,keyf,val,valf);
        }  // in stage !=3, or if object is a way

      // care about 'drop object' filter
      if(keep || (filterstage==1 && otype==2)) {
        if(fil_active[3+otype]) {
            // filter 'drop object' shall be applied
          if(fil_check1(otype,key,keyf,val,valf))
            keep= false;
          }  // filter 'drop object' shall be applied
        }

      if(!keep)  // object is to dispose
  continue;  // we are finished here
      if(!flag)  // flag for this object is not set yet
        hash_seti(otype,id);  // memorize that this object will be kept

      // care about objects which depend on this object
      if(filterstage<4) {  // in stage 1, 2 or 3
        if(filterstage==2 && otype==2) {
            // in stage 2 AND have relation
            // 2: parse relations and update hash flags
            //    of dependent objects;
          refidp= refid; reftypep= reftype;
          while(refidp<refide) {  // for every referenced object
            hash_seti(*reftypep++,*refidp++);  // mark referenced node
            }  // end   for every referenced object
          }  // in stage 2 AND have way
        else if(filterstage==3 && otype==1) {
            // in stage 3 AND have way
            // 3: parse ways and update hash flags of dependent nodes;
          refidp= refid;
          while(refidp<refide) {  // for every referenced node
            hash_seti(0,*refidp++);  // mark referenced node
            }  // end   for every referenced node
          }  // in stage 3 AND have relation
  continue;  // we are finished here
        }  // in stage 1, 2 or 3
      // here: 4: process the whole file;
      }  // care about filtering, dependent on the filterstage

    // process object deletion
    if(oo__ifp->deleteobject!=0) {  // object is to delete
      if((otype==0 && !global_dropnodes) ||
          (otype==1 && !global_dropways) ||
          (otype==2 && !global_droprelations))
          // section is not to drop anyway
        if(global_outo5c || global_outosc || global_outosh)
          // write o5c, osc or osh file
          wo_delete(otype,id,hisver,histime,hiscset,hisuid,hisuser);
            // write delete request
  continue;  // end processing for this object
      }  // end   object is to delete

    // write the object
    if(otype==0) {  // write node
      if(!global_dropnodes) {  // not to drop
        wo_node(id,
          hisver,histime,hiscset,hisuid,hisuser,lon,lat);
        keyp= key; valp= val;
        while(keyp<keye) {  // for all key/val pairs of this object
          if(!fil_activeo[otype] || fil_check2(otype,*keyp,*valp))
            wo_keyval(*keyp,*valp);
          keyp++; valp++;
          }
        }  // end   not to drop
      }  // write node
    else if(otype==1) {  // write way
      if(!global_dropways) {  // not ways to drop
        wo_way(id,hisver,histime,hiscset,hisuid,hisuser);
        refidp= refid;
        while(refidp<refide)  // for every referenced node
          wo_noderef(*refidp++);
        keyp= key; valp= val;
        while(keyp<keye) {  // for all key/val pairs of this object
          if(!fil_activeo[otype] || fil_check2(otype,*keyp,*valp))
            wo_keyval(*keyp,*valp);
          keyp++; valp++;
          }
        }  // end   not ways to drop
      }  // write way
    else if(otype==2) {  // write relation
      if(!global_droprelations) {  // not relations to drop
        wo_relation(id,hisver,histime,hiscset,hisuid,hisuser);
        refidp= refid; reftypep= reftype; refrolep= refrole;
        while(refidp<refide)  // for every referenced object
          wo_ref(*refidp++,*reftypep++,*refrolep++);
        keyp= key; valp= val;
        while(keyp<keye) {  // for all key/val pairs of this object
          if(!fil_activeo[otype] || fil_check2(otype,*keyp,*valp))
            wo_keyval(*keyp,*valp);
          keyp++; valp++;
          }
        }  // end   not relations to drop
      }  // write relation
    }  // end   read all input files
  if(writeheader)
    wo_start(wformat,oo__bbvalid && oo_ifn==1,
      oo__bbx1,oo__bby1,oo__bbx2,oo__bby2);
  wo_end();
  return 0;
  }  // end   oo_main()

//------------------------------------------------------------
// end   Module oo_   osm to osm module
//------------------------------------------------------------



#if !__WIN32__
void sigcatcher(int sig) {
  fprintf(stderr,"osmfilter: output has been terminated.\n");
  exit(1);
  }  // end   sigchatcher()
#endif

int main(int argc,const char** argv) {
  // main program;
  // for the meaning of the calling line parameters please look at the
  // contents of helptext[];
  static char outputfilename[400]= "";  // standard output file name
    // =="": standard output 'stdout'
  int h_n,h_w,h_r;  // user-suggested hash size in MiB, for
    // hash tables of nodes, ways, and relations;
  int r,l;
  const char* a;
  static FILE* parafile= NULL;
  static char* aa= NULL;  // buffer for parameter file line
  char* ap;  // pointer in aa[]
  int aamax;  // maximum length of string to read
  #define main__aaM 1000000

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
  h_n= h_w= h_r= 0;
  fil_ini();
  #if __WIN32__
    setmode(fileno(stdout),O_BINARY);
    setmode(fileno(stdin),O_BINARY);
  #endif

  // read command line parameters
  if(argc<=1) {  // no command line parameters given
    fprintf(stderr,"osmfilter "VERSION"\n"
      "Filters .o5m, .o5c, .osm, .osc and .osh files.\n"
      "Use command line option -h to get a parameter overview,\n"
      "or --help to get detailed help.\n");
return 0;  // end the program, because without having parameters
      // we do not know what to do;
    }
  while(parafile!=NULL || argc>0) {
      // for every parameter in command line
    if(parafile!=NULL) do {
        // there are parameters waiting in a parameter file
      ap= aa;
      for(;;) {
        aamax= main__aaM-1-(ap-aa);
        if(fgets(ap,aamax,parafile)==NULL) {
          if(ap>aa) {
            if(ap>aa && ap[-1]==' ')
              *--ap= 0;  // cut one trailing space
      break;
            }
          goto parafileend;
          }
        if(strzcmp(ap,"// ")==0)
      continue;
        if(ap>aa && (*ap=='\r' || *ap=='\n' || *ap==0)) {
            // end of this parameter
          while(ap>aa && (ap[-1]=='\r' || ap[-1]=='\n')) *--ap= 0;
            // eliminate trailing NL
          if(ap>aa && ap[-1]==' ')
            *--ap= 0;  // cut one trailing space
      break;
          }
        ap= strchr(ap,0);  // find end of string
        while(ap>aa && (ap[-1]=='\r' || ap[-1]=='\n'))
          *--ap= 0;  // cut newline chars
        *ap++= ' '; *ap= 0;  // add a space
        }
      a= aa;
      while(*a!=0 && strchr(" \t\r\n",*a)!=NULL) a++;
      if(*a!=0)
    break;
    parafileend:
      fclose(parafile); parafile= NULL;
      free(aa); aa= NULL;
      } while(false);
    if(parafile==NULL) {
      if(--argc<=0)
  break;
      argv++;  // switch to next parameter; as the first one is just
        // the program name, we must do this previous reading the
        // first 'real' parameter;
      a= argv[0];
      }
    if((l= strzlcmp(a,"--parameter-file="))>0 && a[l]!=0) {
        // parameter file
      parafile= fopen(a+l,"r");
      if(parafile==NULL) {
        PERRv("Cannot open parameter file: %.80s",a+l)
        perror("osmfilter");
return 1;
        }
      aa= (char*)malloc(main__aaM);
      if(aa==NULL) {
        PERR("Cannot get memory for parameter file.")
        fclose(parafile); parafile= NULL;
return 1;
        }
      aa[0]= 0;
  continue;  // take next parameter
      }
    if(loglevel>0)  // verbose mode
      fprintf(stderr,"osmfilter Parameter: %.2000s\n",a);
    if(strcmp(a,"-h")==0) {  // user wants parameter overview
      fprintf(stdout,"%s",shorthelptext);  // print brief help text
        // (took "%s", to prevent oversensitive compiler reactions)
return 0;
      }
    if(strcmp(a,"-help")==0 || strcmp(a,"--help")==0) {
        // user wants help text
      fprintf(stdout,"%s",helptext);  // print help text
        // (took "%s", to prevent oversensitive compiler reactions)
return 0;
      }
    if(strzcmp(a,"--drop-his")==0) {
        // (deprecated)
      PINFO("Option --drop-history is deprecated. Using --drop-author.");
      global_dropauthor= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--drop-aut")==0) {
        // user does not want author information in standard output
      global_dropauthor= true;
  continue;  // take next parameter
      }
    if(strzcmp(argv[0],"--drop-ver")==0) {
        // user does not want version number in standard output
      global_dropauthor= true;
      global_dropversion= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--fake-his")==0) {
        // (deprecated)
      PINFO("Option --fake-history is deprecated. Using --fake-author.");
      global_fakeauthor= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--fake-aut")==0) {
        // user wants faked author information
      global_fakeauthor= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--fake-ver")==0) {
        // user wants just a faked version number as meta data
      global_fakeversion= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--fake-lonlat")==0) {
        // user wants just faked longitude and latitude
        // in case of delete actions (.osc files);
      global_fakelonlat= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--drop-nodes")==0) {
        // user does not want nodes section in standard output
      global_dropnodes= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--drop-ways")==0) {
        // user does not want ways section in standard output
      global_dropways= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--drop-relations")==0) {
        // user does not want relations section in standard output
      global_droprelations= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--ignore-dep")==0) {
      // user does interobject dependencies to be ignored
      global_ignoredependencies= true;
  continue;  // take next parameter
      }
    if(strcmp(argv[0],"--in-josm")==0) {
      // deprecated;
      // this option is still accepted for compatibility reasons;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-o5m")==0 ||
        strcmp(argv[0],"-5")==0) {
        // user wants output in o5m format
      global_outo5m= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-osm")==0) {
        // user wants output in osm format
      global_outosm= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-o5c")==0 ||
        strcmp(a,"-5c")==0) {
        // user wants output in o5c format
      global_outo5m= global_outo5c= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-osm")==0) {
        // user wants output in osm format
      // this is default anyway, hence ignore this parameter
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-osc")==0) {
        // user wants output in osc format
      global_outosc= true;
  continue;  // take next parameter
      }
    if(strcmp(argv[0],"--out-osh")==0) {
        // user wants output in osc format
      global_outosh= true;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--out-key"))>0 ||
        (l= strzlcmp(a,"--out-count"))>0) {
        // user wants a list of keys or vals as output
      static char k[300]= {0,0,0};
      int len;

      global_outkey= k;  // we shall create a list of keys
      if(a[l]=='=' && a[l+1]!=0) {
          // we shall create list of vals to a certain key
        global_outkey= a+l+1;
        len= strlen(global_outkey);
        if(len>=sizeof(k)-2) len= sizeof(k)-3;
        fil_cpy(k,global_outkey,len,2);
        global_outkey= k;
        }
      if(a[6]=='c') global_outsort= true;
  continue;  // take next parameter
      }
    if(strzcmp(argv[0],"--emulate-pbf2")==0) {
        // emulate pbf2osm compatible output
      global_emulatepbf2osm= true;
  continue;  // take next parameter
      }
    if(strzcmp(argv[0],"--emulate-osmo")==0) {
        // emulate Osmosis compatible output
      global_emulateosmosis= true;
  continue;  // take next parameter
      }
    if(strzcmp(argv[0],"--emulate-osmi")==0) {
        // emulate Osmium compatible output
      global_emulateosmium= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"-t=")==0 && a[3]!=0) {
        // user-defined prefix for names of temorary files
      strmcpy(global_tempfilename,a+3,sizeof(global_tempfilename)-30);
  continue;  // take next parameter
      }
    if(strzcmp(a,"-o=")==0 && a[3]!=0) {
        // reroute standard output to a file
      strMcpy(outputfilename,a+3);
  continue;  // take next parameter
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
          fprintf(stderr,"osmfilter: Verbose mode.\n");
        else
          fprintf(stderr,"osmfilter: Verbose mode %i.\n",loglevel);
        }
  continue;  // take next parameter
      }
    if(strcmp(a,"-t")==0) {
        // test mode
      write_testmode= true;
      fprintf(stderr,"osmfilter: Entering test mode.\n");
  continue;  // take next parameter
      }
    if(((l= strzlcmp(a,"--hash-memory="))>0 ||
        (l= strzlcmp(a,"-h="))>0) && isdig(a[l])) {
        // "-h=...": user wants a specific hash size;
      const char* p;

      p= a+l;  // jump over "-h="
      h_n= h_w= h_r= 0;
      // read the up to three values for hash tables' size;
      // format examples: "-h=200-20-10", "-h=1200"
      while(isdig(*p)) { h_n= h_n*10+*p-'0'; p++; }
      if(*p!=0) { p++; while(isdig(*p)) { h_w= h_w*10+*p-'0'; p++; } }
      if(*p!=0) { p++; while(isdig(*p)) { h_r= h_r*10+*p-'0'; p++; } }
  continue;  // take next parameter
      }
    #define F(t) fil_parse(t,a+l);
    #define D(p,f) if((l= strzlcmp(a,#p))>0) { f continue; }
    D(--keep=,F(0)F(1)F(2))
    D(--keep-nodes=,F(0))
    D(--keep-ways=,F(1))
    D(--keep-relations=,F(2))
    D(--keep-nodes-ways=,F(0)F(1))
    D(--keep-nodes-relations=,F(0)F(2))
    D(--keep-ways-relations=,F(1)F(2))
    D(--drop=,F(3)F(4)F(5))
    D(--drop-nodes=,F(3))
    D(--drop-ways=,F(4))
    D(--drop-relations=,F(5))
    D(--drop-nodes-ways=,F(3)F(4))
    D(--drop-nodes-relations=,F(3)F(5))
    D(--drop-ways-relations=,F(4)F(5))
    D(--keep-tags=,F(6)F(7)F(8))
    D(--keep-node-tags=,F(6))
    D(--keep-way-tags=,F(7))
    D(--keep-relation-tags=,F(8))
    D(--keep-node-way-tags=,F(6)F(7))
    D(--keep-node-relation-tags=,F(6)F(8))
    D(--keep-way-relation-tags=,F(7)F(8))
    D(--drop-tags=,F(9)F(10)F(11))
    D(--drop-node-tags=,F(9))
    D(--drop-way-tags=,F(10))
    D(--drop-relation-tags=,F(11))
    D(--drop-node-way-tags=,F(9)F(10))
    D(--drop-node-relation-tags=,F(9)F(11))
    D(--drop-way-relation-tags=,F(10)F(11))
    #undef D
    #undef F
    if(a[0]=='-') {
      PERRv("unrecognized option: %.80s",a)
return 1;
      }
    // here: parameter must be a file name
    if(oo_open(a))  // file cannot be read
return 1;
    }  // end   for every parameter in command line
  // process parameters
  if(oo_ifn==0) {  // no input files given
    PERR("please specify the input file or try:  osmfilter -h")
return 0;  // end the program, because without having input files
      // we do not know what to do;
    }  // for every parameter in command line

  // check plausibility of filter strings
  if(fil_plausi()!=0)
return 2;

  // initialize hash module
  if(outputfilename[0]!=0 && !global_outo5m &&
      !global_outo5c && !global_outosm && !global_outosc &&
      !global_outosh) {
      // have output file name AND  output format not defined
    // try to determine the output format by evaluating
    // the file name extension
    if(strycmp(outputfilename,".o5m")==0) global_outo5m= true;
    else if(strycmp(outputfilename,".o5c")==0)
      global_outo5m= global_outo5c= true;
    else if(strycmp(outputfilename,".osm")==0) global_outosm= true;
    else if(strycmp(outputfilename,".osc")==0) global_outosc= true;
    else if(strycmp(outputfilename,".osh")==0) global_outosh= true;
    if(strycmp(outputfilename,".pbf")==0) {
      PERR(".pbf format is not supported. Please use .o5m.")
return 3;
      }
    }
  if(write_open(outputfilename[0]!=0? outputfilename: NULL)!=0)
return 3;
  if(global_ignoredependencies)
      // user does interobject dependencies to be ignored
    global_recursive= false;
  if(global_recursive) {
    int r;

    if(h_n==0) h_n= 1000;  // use standard value if not set otherwise
    if(h_w==0 && h_r==0) {
        // user chose simple form for hash memory value
      // take the one given value as reference and determine the 
      // three values using these factors: 90%, 9%, 1%
      h_w= h_n/10; h_r= h_n/100;
      h_n-= h_w; h_w-= h_r; }
    r= hash_ini(h_n,h_w,h_r);  // initialize hash table
    if(r==1)
      fprintf(stderr,"osmfilter: Hash size had to be reduced.\n");
    else if(r==2)
      fprintf(stderr,"osmfilter: Not enough memory for hash.\n");
    }  // end   user wants borders

  // do further initializations
  if(global_outo5m) {  // .o5m format is needed as output
    if(o5_ini()!=0) {
      fprintf(stderr,"osmfilter: Not enough memory for .o5m buffer.\n");
return 5;
      }
    }  // end   .o5m format is needed as output
  sprintf(strchr(global_tempfilename,0),".%"PRIi64,(int64_t)getpid());
  if(loglevel>=2)
    fprintf(stderr,"Tempfiles: %s.*\n",global_tempfilename);

  // do the work
  r= oo_main();
  if(loglevel>=2) {  // verbose
    if(read_bufp!=NULL && read_bufp<read_bufe)
      fprintf(stderr,"osmfilter: Next bytes to parse:\n"
        "  %.02X %.02X %.02X %.02X %.02X %.02X %.02X %.02X\n",
        read_bufp[0],read_bufp[1],read_bufp[2],read_bufp[3],
        read_bufp[4],read_bufp[5],read_bufp[6],read_bufp[7]);
    }  // verbose
  write_flush();
  if(hash_queryerror()!=0)
    r= 91;
  if(write_error) {
    r= 92;
    PERR("write error.")
    }
  if(loglevel>0) {  // verbose mode
    if(oo_sequenceid!=INT64_C(-0x7fffffffffffffff))
      fprintf(stderr,"osmfilter: Last processed: %s %"PRIu64".\n",
        ONAME(oo_sequencetype),oo_sequenceid);
    if(r!=0)
      fprintf(stderr,"osmfilter Exit: %i\n",r);
    }  // verbose mode
  return r;
  }  // end   main()


// osmconvert 2015-04-13 14:20
#define VERSION "0.8.4"
//
// compile this file:
// gcc osmconvert.c -lz -O3 -o osmconvert
//
// (c) 2011..2015 Markus Weber, Nuernberg
// Richard Russo contributed the initiative to --add-bbox-tags option
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
const char* shorthelptext=
"\nosmconvert " VERSION "  Parameter Overview\n"
"(Please use  --help  to get more information.)\n"
"\n"
"<FILE>                    input file name\n"
"-                         read from standard input\n"
"-b=<x1>,<y1>,<x2>,<y2>    apply a border box\n"
"-B=<border_polygon>       apply a border polygon\n"
"--complete-ways           do not clip ways at the borders\n"
"--complex-ways            do not clip multipolygons at the borders\n"
"--all-to-nodes            convert ways and relations to nodes\n"
"--add-bbox-tags           adds bbox tags to ways and relations\n"
"--add-bboxarea-tags       adds tags for estimated bbox areas\n"
"--add-bboxweight-tags     adds tags for log2 of bbox areas\n"
"--object-type-offset=<id> offset for ways/relations if --all-to-nodes\n"
"--max-objects=<n>         space for --all-to-nodes, 1 obj. = 16 bytes\n"
"--drop-broken-refs        delete references to excluded nodes\n"
"--drop-author             delete changeset and user information\n"
"--drop-version            same as before, but delete version as well\n"
"--drop-nodes              delete all nodes\n"
"--drop-ways               delete all ways\n"
"--drop-relations          delete all relations\n"
"--diff                    calculate differences between two files\n"
"--diff-contents           same as before, but compare whole contents\n"
"--subtract                subtract objects given by following files\n"
"--pbf-granularity=<val>   lon/lat granularity of .pbf input file\n"
"--emulate-osmosis         emulate Osmosis XML output format\n"
"--emulate-pbf2osm         emulate pbf2osm output format\n"
"--fake-author             set changeset to 1 and timestamp to 1970\n"
"--fake-version            set version number to 1\n"
"--fake-lonlat             set lon to 0 and lat to 0\n"
"-h                        display this parameter overview\n"
"--help                    display a more detailed help\n"
"--merge-versions          merge versions of each object in a file\n"
"--out-osm                 write output in .osm format (default)\n"
"--out-osc                 write output in .osc format (OSMChangefile)\n"
"--out-osh                 write output in .osh format (visible-tags)\n"
"--out-o5m                 write output in .o5m format (fast binary)\n"
"--out-o5c                 write output in .o5c format (bin. Changef.)\n"
"--out-pbf                 write output in .pbf format (bin. standard)\n"
"--out-csv                 write output in .csv format (plain table)\n"
"--out-none                no standard output (for testing purposes)\n"
"--csv=<column names>      choose columns for csv format\n"
"--csv-headline            start csv output with a headline\n"
"--csv-separator=<sep>     separator character(s) for csv format\n"
"--timestamp=<date_time>   add a timestamp to the data\n"
"--timestamp=NOW-<seconds> add a timestamp in seconds before now\n"
"--out-timestamp           output the file\'s timestamp, nothing else\n"
"--out-statistics          write statistics, nothing else\n"
"--statistics              write statistics to stderr\n"
"-o=<outfile>              reroute standard output to a file\n"
"-t=<tempfile>             define tempfile prefix\n"
"--parameter-file=<file>   param. in file, separated by empty lines\n"
"--verbose                 activate verbose mode\n";
const char* helptext=
"\nosmconvert " VERSION "\n"
"\n"
"This program reads different file formats of the OpenStreetMap\n"
"project and converts the data to the selected output file format.\n"
"These formats can be read:\n"
"  .osm  .osc  .osc.gz  .osh  .o5m  .o5c  .pbf\n"
"These formats can be written:\n"
"  .osm (default)  .osc  .osh  .o5m  .o5c  .pbf\n"
"\n"
"Names of input files must be specified as command line parameters.\n"
"Use - to read from standard input. You do not need to specify the\n"
"input formats, osmconvert will recognize them by itself.\n"
"The output format is .osm by default. If you want a different format,\n"
"please specify it using the appropriate command line parameter.\n"
"\n"
"-b=<x1>,<y1>,<x2>,<y2>\n"
"        If you want to limit the geographical region, you can define\n"
"        a bounding box. To do this, enter the southwestern and the\n"
"        northeastern corners of that area. For example:\n"
"        -b=-0.5,51,0.5,52\n"
"\n"
"-B=<border_polygon>\n"
"        Alternatively to a bounding box you can use a border polygon\n"
"        to limit the geographical region.\n"
"        The format of a border polygon file can be found in the OSM\n"
"        Wiki: http://wiki.openstreetmap.org/wiki/Osmosis/\n"
"              Polygon_Filter_File_Format\n"
"        You do not need to strictly follow the format description,\n"
"        you must ensure that every line of coordinates starts with\n"
"        blanks.\n"
"\n"
"--complete-ways\n"
"        If applying a border box or a border polygon, all nodes\n"
"        the borders are excluded; even then if they belong to a way\n"
"        which is not entirely excluded because it has some nodes\n"
"        inside the borders.\n"
"        This option will ensure that every way stays complete, even\n"
"        it it intersects the borders. This will result in slower\n"
"        processing, and the program will loose its ability to read\n"
"        from standard input. It is recommended to use .o5m format as\n"
"        input format to compensate most of the speed disadvantage.\n"
"\n"
"--complex-ways\n"
"        Same as before, but multipolygons will not be cut at the\n"
"        borders too.\n"
"\n"
"--all-to-nodes\n"
"        Some applications do not have the ability to process ways or\n"
"        relations, they just accept nodes as input. However, more and\n"
"        more complex object are mapped as ways or even relations in\n"
"        order to get all their details into the database.\n"
"        Apply this option if you want to convert ways and relations\n"
"        to nodes and thereby make them available to applications\n"
"        which can only deal with nodes.\n"
"        For each way a node is created. The way's id is increased by\n"
"        10^15 and taken as id for the new node. The node's longitude\n"
"        and latitude are set to the way's geographical center. Same\n"
"        applies to relations, however they get 2*10^15 as id offset.\n"
"\n"
"--add-bbox-tags\n"
"        This option adds a tag with a bounding box to each object.\n"
"        The tag will contain the border coordinates in this order:\n"
"        min Longitude, min Latitude, max Longitude , max Latitude.\n"
"        e.g.:  <tag k=\"bBox\" v=\"-0.5000,51.0000,0.5000,52.0000\"/>\n"
"\n"
"--add-bboxarea-tags\n"
"        A tag for an estimated area value for the bbox is added to\n"
"        each way and each relation. The unit is square meters.\n"
"        For example:  <tag k=\"bBoxArea\" v=\"33828002\"/>\n"
"\n"
"--add-bboxweight-tags\n"
"        This option will add the binary logarithm of the bbox area\n"
"        of each way and each relation.\n"
"        For example:  <tag k=\"bBoxWeight\" v=\"20\"/>\n"
"\n"
"--object-type-offset=<id offset>\n"
"        If applying the --all-to-nodes option as explained above, you\n"
"        may adjust the id offset. For example:\n"
"          --object-type-offset=4000000000\n"
"        By appending \"+1\" to the offset, the program will create\n"
"        ids in a sequence with step 1. This might be useful if the\n"
"        there is a subsequently running application which cannot\n"
"        process large id numbers. Example:\n"
"          --object-type-offset=1900000000+1\n"
"\n"
"--drop-broken-refs\n"
"        Use this option if you need to delete references to nodes\n"
"        which have been excluded because lying outside the borders\n"
"        (mandatory for some applications, e.g. Map Composer, JOSM).\n"
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
"--diff\n"
"        Calculate difference between two files and create a new .osc\n"
"        or .o5c file.\n"
"        There must be TWO input files and borders cannot be applied.\n"
"        Both files must be sorted by object type and id. Created\n"
"        objects will appear in the output file as \"modified\", unless\n"
"        having version number 1.\n"
"\n"
"--diff-contents\n"
"        Similar to --diff, this option calculates differences between\n"
"        two OSM files. Here, to determine the differences complete\n"
"        OSM objects are consulted, not only the version numbers.\n"
"        Unfortunately, this option strictly requires both input files\n"
"        to have .o5m format.\n"
"\n"
"--subtract\n"
"        The output file will not contain any object which exists in\n"
"        one of the input files following this directive. For example:\n"
"        osmconvert input.o5m --subtract minus.o5m -o=output.o5m\n"
"\n"
"--pbf-granularity=<val>\n"
"        Rarely .pbf files come with non-standard granularity.\n"
"        osmconvert will recognize this and suggest to specify the\n"
"        abnormal lon/lat granularity using this command line option.\n"
"        Allowed values are: 100 (default), 1000, 10000, ..., 10000000.\n"
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
"        osmconvert will write changeset 1, timestamp 1970.\n"
"\n"
"--fake-version\n"
"        Same as --fake-author, but - if .osm xml is used as output\n"
"        format - only the version number will be written (version 1).\n"
"        This is useful if you want to inspect the data with JOSM.\n"
"\n"
"--fake-lonlat\n"
"        Some programs depend on getting longitude/latitude values,\n"
"        even when the object in question shall be deleted. With this\n"
"        option you can have osmconvert to fake these values:\n"
"           ... lat=\"0\" lon=\"0\" ...\n"
"        Note that this is for XML files only (.osc and .osh).\n"
"\n"
"-h\n"
"        Display a short parameter overview.\n"
"\n"
"--help\n"
"        Display this help.\n"
"\n"
"--merge-versions\n"
"        Some .osc files contain different versions of one object.\n"
"        Use this option to accept such duplicates on input.\n"
"\n"
"--out-osm\n"
"        Data will be written in .osm format. This is the default\n"
"        output format.\n"
"\n"
"--out-osc\n"
"        The OSM Change format will be used for output. Please note\n"
"        that OSM objects which are to be deleted will be represented\n"
"        by their ids only.\n"
"\n"
"--out-osh\n"
"        For every OSM object, the appropriate \'visible\' tag will be\n"
"        added to meet \'full planet history\' specification.\n"
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
"--out-pbf\n"
"        For output, PBF format will be used.\n"
"\n"
"--out-csv\n"
"        A character separated list will be written to output.\n"
"        The default separator is Tab, the default columns are:\n"
"        type, id, name. You can change both by using the options\n"
"        --csv-separator= and --csv=\n"
"\n"
"--csv-headline\n"
"        Choose this option to print a headline to csv output.\n"
"\n"
"--csv-separator=<sep>\n"
"        You may change the default separator (Tab) to a different\n"
"        character or character sequence. For example:\n"
"        --csv-separator=\"; \"\n"
"\n"
"--csv=<columns>\n"
"        If you want to have certain columns in your csv list, please \n"
"        specify their names as shown in this example:\n"
"        --csv=\"@id name ref description\"\n"
"        There are a few special column names for header data:\n"
"        @otype (object type 0..2), @oname (object type name), @id\n"
"        @lon, @lat, @version, @timestamp, @changeset, @uid, @user\n"
"\n"
"--out-none\n"
"        This will be no standard output. This option is for testing\n"
"        purposes only.\n"
"\n"
"--timestamp=<date_and_time>\n"
"--timestamp=NOW<seconds_relative_to_now>\n"
"        If you want to set the OSM timestamp of your output file,\n"
"        supply it with this option. Date and time must be formatted\n"
"        according OSM date/time specifications. For example:\n"
"        --timestamp=2011-01-31T23:59:30Z\n"
"        You also can supply a relative time in seconds, e.g. 24h ago:\n"
"        --timestamp=NOW-86400\n"
"\n"
"--out-timestamp\n"
"        With this option set, osmconvert prints just the time stamp\n"
"        of the input file, nothing else.\n"
"\n"
"--statistics\n"
"        This option activates a statistics counter. The program will\n"
"        print statistical data to stderr.\n"
"\n"
"--out-statistics\n"
"        Same as --statistics, but the statistical data will be\n"
"        written to standard output.\n"
"\n"
"-o=<outfile>\n"
"        Standard output will be rerouted to the specified file.\n"
"        If no output format has been specified, the program will\n"
"        rely  the file name extension.\n"
"\n"
"-t=<tempfile>\n"
"        If borders are to be applied or broken references to be\n"
"        eliminated, osmconvert creates and uses two temporary files.\n"
"        This parameter defines their name prefix. The default value\n"
"        is \"osmconvert_tempfile\".\n"
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
"        osmconvert will display all input parameters.\n"
"\n"
"Examples\n"
"\n"
"./osmconvert europe.pbf --drop-author >europe.osm\n"
"./osmconvert europe.pbf |gzip >europe.osm.gz\n"
"bzcat europe.osm.bz2 |./osmconvert --out-pbf >europe.pbf\n"
"./osmconvert europe.pbf -B=ch.poly >switzerland.osm\n"
"./osmconvert switzerland.osm --out-o5m >switzerland.o5m\n"
"./osmconvert june_july.osc --out-o5c >june_july.o5c\n"
"./osmconvert june.o5m june_july.o5c.gz --out-o5m >july.o5m\n"
"./osmconvert sep.osm sep_oct.osc oct_nov.osc >nov.osm\n"
"./osmconvert northamerica.osm southamerica.osm >americas.osm\n"
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
"2 MB using this option:\n"
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
"There is another temporary memory space which is used only for the\n"
"conversion of ways and relations to nodes (option --all-to-nodes).\n"
"This space is sufficient for up to 25 Mio. OSM objects, 400 MB of\n"
"main memory are needed for this purpose, 800 MB if extended option\n"
"--add-bbox-tags has been invoked. If this is not sufficient or\n"
"if you want to save memory, you can configure the maximum number of\n"
"OSM objects by yourself. For example:\n"
"\n"
"  --max-objects=35000000\n"
"\n"
"The number of references per object is limited to 100,000. This will\n"
"be sufficient for all OSM files. If you are going to create your own\n"
"OSM files by converting shapefiles or other files to OSM format, this\n"
"might result in way objects with more than 100,000 nodes. For this\n"
"reason you will need to increase the maximum accordingly. Example:\n"
"\n"
"  --max-refs=400000\n"
"\n"
"Limitations\n"
"\n"
"When extracting a geographical region (using -b or -B), the input\n"
"file must contain the objects ordered by their type: first, all\n"
"nodes, next, all ways, followed by all relations. Within each of\n"
"these sections, the objects section must be sorted by their id in\n"
"ascending order.\n"
"\n"
"Usual .osm, .osc, .o5m, o5c and .pbf files adhere to this condition.\n"
"This means that you do not have to worry about this limitation.\n"
"osmconvert will display an error message if this sequence is broken.\n"
"\n"
"If a polygon file for borders is supplied, the maximum number of\n"
"polygon points is about 40,000.\n"
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
#include <locale.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

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
#define UR(x) if(x){}  // result value intentionally ignored
#if __WIN32__
  #define NL "\r\n"  // use CR/LF as new-line sequence
  #define off_t off64_t
  #define lseek lseek64
  z_off64_t gzseek64(gzFile,z_off64_t,int);
  #define gzseek gzseek64
#else
  #define NL "\n"  // use LF as new-line sequence
  #define O_BINARY 0
#endif



//------------------------------------------------------------
// Module Global   global variables for this program
//------------------------------------------------------------

// to distinguish global variable from local or module global
// variables, they are preceded by 'global_';

static bool global_diff= false;  // calculate diff between two files
static bool global_diffcontents= false;
  // calculate physical diff between two files; 'physical' means
  // that not only the version number is consulted to determine
  // object differences, the whole object contents is;
static bool global_subtract= false;  // any file which is opened
  // via read_open() resp. oo_open() while global_subtract==true
  // will be subtracted, i.e. the delete flags will be inverted:
  // <delete> works as non-delete and no-<delete> works as "stay";
  // be sure to have set this variable back to false before starting
  // processing, to exclude unwanted effects on temporary files;
static bool global_mergeversions= false;  // accept duplicate versions
static bool global_dropversion= false;  // exclude version
static bool global_dropauthor= false;  // exclude author information
static bool global_fakeauthor= false;  // fake author information
static bool global_fakeversion= false;  // fake just the version number
static bool global_fakelonlat= false;
  // fake longitude and latitude in case of delete actions (.osc);
static bool global_dropbrokenrefs= false;  // exclude broken references
static bool global_dropnodes= false;  // exclude nodes section
static bool global_dropways= false;  // exclude ways section
static bool global_droprelations= false;  // exclude relations section
static bool global_outo5m= false;  // output shall have .o5m format
static bool global_outo5c= false;  // output shall have .o5c format
static bool global_outosm= false;  // output shall have .osm format
static bool global_outosc= false;  // output shall have .osc format
static bool global_outosh= false;  // output shall have .osh format
static bool global_outpbf= false;  // output shall have .pbf format
static bool global_outcsv= false;  // output shall have .csv format
static bool global_outnone= false;  // no standard output at all
static int32_t global_pbfgranularity= 100;
  // granularity of lon/lat in .pbf files; unit: 1 nanodegree;
static int32_t global_pbfgranularity100= 0;
  // granularity of lon/lat in .pbf files; unit: 100 nanodegrees;
  // 0: default: 100 nanodegrees;
static bool global_emulatepbf2osm= false;
  // emulate pbf2osm compatible output
static bool global_emulateosmosis= false;
  // emulate Osmosis compatible output
static bool global_emulateosmium= false;
  // emulate Osmium compatible output
static int64_t global_timestamp= 0;
  // manually chosen file timestamp; ==0: no file timestamp given;
static bool global_outtimestamp= false;
  // print only the file timestamp, nothing else
static bool global_statistics= false;  // print statistics to stderr
static bool global_outstatistics= false;  // print statistics to stdout
static bool global_csvheadline= false;  // headline for csv
static char global_csvseparator[16]= "\t";  // separator for csv
static bool global_completeways= false;  // when applying borders,
  // do not clip ways but include them as whole if at least a single
  // of its nodes lies inside the borders;
static bool global_complexways= false;  // same as global_completeways,
  // but multipolygons are included completely (with all ways and their
  // nodes), even when only a single nodes lies inside the borders;
static int global_calccoords= 0;
  // calculate coordinates for all objects;
  // 0: no coordinates to calculate; 1: calculate coordinates;
  // -1: calculate coordinates and bbox;
static bool global_alltonodes= false;
  // convert all ways and all relations to nodes
static bool global_add= false;
  // add at least one tag shall be added;
  // global_add==global_addbbox|global_addbboxarea|global_addbboxweight
static bool global_addbbox= false;
  // add bBox tags to ways and relations
static bool global_addbboxarea= false;
  // add bBoxArea tags to ways and relations
static bool global_addbboxweight= false;
  // add bBoxWeight tags to ways and relations
static int64_t global_maxobjects= 25000000;
static int64_t global_otypeoffset10= INT64_C(1000000000000000);
  // if global_calccoords!=0:
  // id offset for ways; *2: id offset for relations;
static int64_t global_otypeoffset05,
  global_otypeoffset15,global_otypeoffset20;
  // (just to save CPU time for calculating the offset of relations)
static int64_t global_otypeoffsetstep= 0;
  // if !=0, the program will not create the new id by adding
  // global_otypeoffset but by starting at global_otypeoffset
  // and adding 1 for every new way, resp. relation:
static char global_tempfilename[350]= "osmconvert_tempfile";
  // prefix of names for temporary files
static int64_t global_maxrefs= 100000;
#define PERR(f) { static int msgn= 3; if(--msgn>=0) \
  fprintf(stderr,"osmconvert Error: " f "\n"); }
  // print error message
#define PERRv(f,...) { static int msgn= 3; if(--msgn>=0) \
  fprintf(stderr,"osmconvert Error: " f "\n",__VA_ARGS__); }
  // print error message with value(s)
#define WARN(f) { static int msgn= 3; if(--msgn>=0) \
  fprintf(stderr,"osmconvert Warning: " f "\n"); }
  // print a warning message, do it maximal 3 times
#define WARNv(f,...) { static int msgn= 3; if(--msgn>=0) \
  fprintf(stderr,"osmconvert Warning: " f "\n",__VA_ARGS__); }
  // print a warning message with value(s), do it maximal 3 times
#define PINFO(f) \
  fprintf(stderr,"osmconvert: " f "\n"); // print info message
#define PINFOv(f,...) \
  fprintf(stderr,"osmconvert: " f "\n",__VA_ARGS__);
#define ONAME(i) \
  (i==0? "node": i==1? "way": i==2? "relation": "unknown object")
#define global_fileM 1002  // maximum number of input files

//------------------------------------------------------------
// end   Module Global   global variables for this program
//------------------------------------------------------------



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

static inline bool file_exists(const char* file_name) {
  // query if a file exists;
  // file_name[]: name of the file in question;
  // return: the file exists;
  return access(file_name,R_OK)==0;
  }  // file_exists()

static inline int32_t msbit(int64_t v) {
  // gets the most significant 1-bit of a 64 bit integer value;
  int32_t msb;

  msb= 0;
  if(v>=0x100000000LL) {
    v/= 0x100000000LL;
    msb+= 32;
    }
  if(v>=0x10000L) {
    v/= 0x10000L;
    msb+= 16;
    }
  if(v>=0x100) {
    v/= 0x100;
    msb+= 8;
    }
  if(v>=0x10) {
    v/= 0x10;
    msb+= 4;
    }
  if(v>=0x4) {
    v/= 0x4;
    msb+= 2;
    }
  if(v>=0x2) {
    v/= 0x2;
    msb+= 1;
    }
  if(v!=0) {
    msb+= 1;
    }
  return msb;
  }  // msbit()

static inline int64_t cosrk(int32_t lat) {
  // this procedure calculates the Cosinus of the given latitude,
  // multiplies it with 40000k/(360*10^7)==0.00012345679,
  // and takes the reciprocal value of it;
  // lat: latitude in 100 nano degrees;
  // return: constant k needed to approximate the area of a
  //         coordinte-defined bbox:
  //         (lonmax-lonmin)*(latmax-latmin)/k
  static const int32_t cosrktab[901]= {
    8100,8100,8100,8100,8100,8100,8100,8100,
    8100,8100,8101,8101,8101,8102,8102,8102,
    8103,8103,8103,8104,8104,8105,8105,8106,
    8107,8107,8108,8109,8109,8110,8111,8111,
    8112,8113,8114,8115,8116,8116,8117,8118,
    8119,8120,8121,8122,8123,8125,8126,8127,
    8128,8129,8130,8132,8133,8134,8136,8137,
    8138,8140,8141,8143,8144,8146,8147,8149,
    8150,8152,8154,8155,8157,8159,8160,8162,
    8164,8166,8168,8169,8171,8173,8175,8177,
    8179,8181,8183,8185,8187,8189,8192,8194,
    8196,8198,8200,8203,8205,8207,8210,8212,
    8215,8217,8219,8222,8224,8227,8230,8232,
    8235,8237,8240,8243,8246,8248,8251,8254,
    8257,8260,8263,8265,8268,8271,8274,8277,
    8280,8284,8287,8290,8293,8296,8299,8303,
    8306,8309,8313,8316,8319,8323,8326,8330,
    8333,8337,8340,8344,8347,8351,8355,8358,
    8362,8366,8370,8374,8377,8381,8385,8389,
    8393,8397,8401,8405,8409,8413,8418,8422,
    8426,8430,8434,8439,8443,8447,8452,8456,
    8461,8465,8470,8474,8479,8483,8488,8493,
    8497,8502,8507,8512,8516,8521,8526,8531,
    8536,8541,8546,8551,8556,8561,8566,8571,
    8577,8582,8587,8592,8598,8603,8608,8614,
    8619,8625,8630,8636,8642,8647,8653,8658,
    8664,8670,8676,8682,8687,8693,8699,8705,
    8711,8717,8723,8729,8736,8742,8748,8754,
    8761,8767,8773,8780,8786,8793,8799,8806,
    8812,8819,8825,8832,8839,8846,8852,8859,
    8866,8873,8880,8887,8894,8901,8908,8915,
    8922,8930,8937,8944,8951,8959,8966,8974,
    8981,8989,8996,9004,9012,9019,9027,9035,
    9043,9050,9058,9066,9074,9082,9090,9098,
    9107,9115,9123,9131,9140,9148,9156,9165,
    9173,9182,9190,9199,9208,9216,9225,9234,
    9243,9252,9261,9270,9279,9288,9297,9306,
    9315,9325,9334,9343,9353,9362,9372,9381,
    9391,9400,9410,9420,9430,9439,9449,9459,
    9469,9479,9489,9499,9510,9520,9530,9540,
    9551,9561,9572,9582,9593,9604,9614,9625,
    9636,9647,9658,9669,9680,9691,9702,9713,
    9724,9736,9747,9758,9770,9781,9793,9805,
    9816,9828,9840,9852,9864,9876,9888,9900,
    9912,9924,9937,9949,9961,9974,9986,9999,
    10012,10024,10037,10050,10063,10076,10089,10102,
    10115,10128,10142,10155,10169,10182,10196,10209,
    10223,10237,10251,10265,10279,10293,10307,10321,
    10335,10350,10364,10378,10393,10408,10422,10437,
    10452,10467,10482,10497,10512,10527,10542,10558,
    10573,10589,10604,10620,10636,10652,10668,10684,
    10700,10716,10732,10748,10765,10781,10798,10815,
    10831,10848,10865,10882,10899,10916,10934,10951,
    10968,10986,11003,11021,11039,11057,11075,11093,
    11111,11129,11148,11166,11185,11203,11222,11241,
    11260,11279,11298,11317,11337,11356,11375,11395,
    11415,11435,11455,11475,11495,11515,11535,11556,
    11576,11597,11618,11639,11660,11681,11702,11724,
    11745,11767,11788,11810,11832,11854,11876,11899,
    11921,11944,11966,11989,12012,12035,12058,12081,
    12105,12128,12152,12176,12200,12224,12248,12272,
    12297,12321,12346,12371,12396,12421,12446,12472,
    12497,12523,12549,12575,12601,12627,12654,12680,
    12707,12734,12761,12788,12815,12843,12871,12898,
    12926,12954,12983,13011,13040,13069,13098,13127,
    13156,13186,13215,13245,13275,13305,13336,13366,
    13397,13428,13459,13490,13522,13553,13585,13617,
    13649,13682,13714,13747,13780,13813,13847,13880,
    13914,13948,13982,14017,14051,14086,14121,14157,
    14192,14228,14264,14300,14337,14373,14410,14447,
    14485,14522,14560,14598,14637,14675,14714,14753,
    14792,14832,14872,14912,14952,14993,15034,15075,
    15116,15158,15200,15242,15285,15328,15371,15414,
    15458,15502,15546,15591,15636,15681,15726,15772,
    15818,15865,15912,15959,16006,16054,16102,16151,
    16200,16249,16298,16348,16398,16449,16500,16551,
    16603,16655,16707,16760,16813,16867,16921,16975,
    17030,17085,17141,17197,17253,17310,17367,17425,
    17483,17542,17601,17660,17720,17780,17841,17903,
    17964,18027,18090,18153,18217,18281,18346,18411,
    18477,18543,18610,18678,18746,18814,18883,18953,
    19023,19094,19166,19238,19310,19384,19458,19532,
    19607,19683,19759,19836,19914,19993,20072,20151,
    20232,20313,20395,20478,20561,20645,20730,20815,
    20902,20989,21077,21166,21255,21346,21437,21529,
    21622,21716,21811,21906,22003,22100,22199,22298,
    22398,22500,22602,22705,22810,22915,23021,23129,
    23237,23347,23457,23569,23682,23796,23912,24028,
    24146,24265,24385,24507,24630,24754,24879,25006,
    25134,25264,25395,25527,25661,25796,25933,26072,
    26212,26353,26496,26641,26788,26936,27086,27238,
    27391,27547,27704,27863,28024,28187,28352,28519,
    28688,28859,29033,29208,29386,29566,29748,29933,
    30120,30310,30502,30696,30893,31093,31295,31501,
    31709,31920,32134,32350,32570,32793,33019,33249,
    33481,33717,33957,34200,34447,34697,34951,35209,
    35471,35737,36007,36282,36560,36843,37131,37423,
    37720,38022,38329,38641,38958,39281,39609,39943,
    40282,40628,40980,41337,41702,42073,42450,42835,
    43227,43626,44033,44447,44870,45301,45740,46188,
    46646,47112,47588,48074,48570,49076,49594,50122,
    50662,51214,51778,52355,52946,53549,54167,54800,
    55447,56111,56790,57487,58200,58932,59683,60453,
    61244,62056,62890,63747,64627,65533,66464,67423,
    68409,69426,70473,71552,72665,73814,75000,76225,
    77490,78799,80153,81554,83006,84510,86071,87690,
    89371,91119,92937,94828,96799,98854,100998,103238,
    105580,108030,110598,113290,116118,119090,122220,125518,
    129000,132681,136578,140712,145105,149781,154769,160101,
    165814,171950,178559,185697,193429,201834,211004,221047,
    232095,244305,257873,273037,290097,309432,331529,357027,
    386774,421931,464119,515683,580138,663010,773507,928203,
    1160248,1546993,2320483,4640960,
    2147483647 };  // cosrk values for 10th degrees from 0 to 90

  lat/= 1000000;
    // transform unit 100 nano degree into unit 10th degree
  if(lat<0) lat= -lat;  // make it positive
  if(lat>900) lat= 900; // set maximum of 90 degree
  return cosrktab[lat];
  }  // cosrk()
// the table in the previous procedure has been generated by this
// program:
#if 0  // file cosrk.c, run it with: gcc cosrk.c -lm -o cosrk && ./cosrk
#include <stdio.h>
#include <math.h>
#include <inttypes.h>
int main() {
  int i;
  printf("  static const int32_t cosrktab[901]= {");
  i= 0;
  for(i= 0;i<900;i++) {
    if(i%8==0)
      printf("\n    ");
    printf("%"PRIi32",",(int32_t)(
      1/( cos(i/1800.0*3.14159265359) *0.00012345679)
      ));
    }
  printf("\n    2147483647");
  printf(" };  // cosrk values for 10th degrees from 0 to 90\n");
  return 0; }
#endif



//------------------------------------------------------------
// Module pbf_   protobuf conversions module
//------------------------------------------------------------

// this module provides procedures for conversions from
// protobuf formats to regular numbers;
// as usual, all identifiers of a module have the same prefix,
// in this case 'pbf'; an underline will follow in case of a
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

static inline bool pbf_jump(byte** pp) {
  // jump over a protobuf formatted element - no matter
  // which kind of element;
  // pp: see module header;
  // return: the data do not meet protobuf specifications (error);
  byte* p;
  int type;
  uint32_t u;

  p= *pp;
  type= *p & 0x07;
  switch(type) {  // protobuf type
  case 0:  // Varint
    while(*p & 0x80) p++; p++;  // jump over id
    while(*p & 0x80) p++; p++;  // jump over data
    break;
  case 1: // fixed 64 bit;
    while(*p & 0x80) p++; p++;  // jump over id
    p+= 4;  // jump over data
    break;
  case 2:  // String
    while(*p & 0x80) p++; p++;  // jump over id
    u= pbf_uint32(&p);
    p+= u;  // jump over string contents
    break;
  case 5: // fixed 32 bit;
    while(*p & 0x80) p++; p++;  // jump over id
    p+= 2;  // jump over data
    break;
  default:  // unknown id
    fprintf(stderr,"osmconvert: Format error: 0x%02X.\n",*p);
    (*pp)++;
return true;
    }  // end   protobuf type
  *pp= p;
  return false;
  }  // end   pbf_jump()

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
// in this case 'hash'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static bool hash__initialized= false;
#define hash__M 3
static unsigned char* hash__mem[hash__M]= {NULL,NULL,NULL};
  // start of the hash fields for each object type (node, way, relation);
static uint32_t hash__max[hash__M]= {0,0,0};
  // size of the hash fields for each object type (node, way, relation);
static int hash__error_number= 0;
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
  // range for all input parameters: 1..4000, unit: MiB;
  // the second and any further call of this procedure will be ignored;
  // return: 0: initialization has been successful (enough memory);
  //         1: memory request had to been reduced to fit the system's
  //            resources (warning);
  //         2: memory request was unsuccessful (error);
  // general note concerning OSM database:
  // number of objects at Oct 2010: 950M nodes, 82M ways, 1.3M relations;
  int o;  // object type
  bool warning,error;

  warning= error= false;
  if(hash__initialized)  // already initialized
    return 0;  // ignore the call of this procedure
  // check parameters and store the values
  #define D(x,o) if(x<1) x= 1; else if(x>4000) x= 4000; \
    hash__max[o]= x*(1024*1024);
  D(n,0u) D(w,1u) D(r,2u)
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

static inline void hash_seti(int o,int64_t idi) {
  // set a flag for a specific object type and ID;
  // o: object type; 0: node; 1: way; 2: relation;
  //    caution: due to performance reasons the boundaries
  //    are not checked;
  // id: id of the object;
  unsigned char* mem;  // address of byte in hash table
  unsigned int ido;  // bit offset to idi;

  if(!hash__initialized) return;  // error prevention
  idi+= ((int64_t)hash__max[o])<<3;  // consider small negative numbers
  ido= idi&0x7;  // extract bit number (0..7)
  idi>>=3;  // calculate byte offset
  idi%= hash__max[o];  // consider length of hash table
  mem= hash__mem[o];  // get start address of hash table
  mem+= idi;  // calculate address of the byte
  *mem|= (1<<ido);  // set bit
  }  // end   hash_seti()

static inline void hash_cleari(int o,int64_t idi) {
  // clears a flag for a specific object type and ID;
  // o: object type; 0: node; 1: way; 2: relation;
  //    caution: due to performance reasons the boundaries
  //    are not checked;
  // id: id of the object;
  unsigned char* mem;  // address of byte in hash table
  unsigned int ido;  // bit offset to idi;

  if(!hash__initialized) return;  // error prevention
  idi+= ((int64_t)hash__max[o])<<3;  // consider small negative numbers
  ido= idi&0x7;  // extract bit number (0..7)
  idi>>=3;  // calculate byte offset
  idi%= hash__max[o];  // consider length of hash table
  mem= hash__mem[o];  // get start address of hash table
  mem+= idi;  // calculate address of the byte
  *mem&= (unsigned char)(~0)^(1<<ido);  // clear bit
  }  // end   hash_cleari()

static inline bool hash_geti(int o,int64_t idi) {
  // get the status of a flag for a specific object type and ID;
  // (same as previous procedure, but id must be given as number);
  // o: object type; 0: node; 1: way; 2: relation;  caution:
  //    due to performance reasons the boundaries are not checked;
  // id: id of the object;
  unsigned char* mem;
  unsigned int ido;  // bit offset to idi;
  bool flag;

  if(!hash__initialized) return true;  // error prevention
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
  return hash__error_number;
  }  // end   hash_queryerror()

//------------------------------------------------------------
// end   Module hash_   OSM hash module
//------------------------------------------------------------



//------------------------------------------------------------
// Module border_   OSM border module
//------------------------------------------------------------

// this module provides procedures for reading the border file
// (.poly) and determine if a point lies inside or outside the
// border polygon;
// as usual, all identifiers of a module have the same prefix,
// in this case 'border'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static const int32_t border__nil= 2000000000L;
static int32_t border__bx1= 2000000000L,border__by1,
  border__bx2,border__by2;
  // in case of a border box:
  // coordinates of southwest and northeast corner;
// in case of a border polygon:
// for the border polygon, every edge is stored in a list;
// to speed-up the inside/outside question we need to sort the edges
// by x1; subsequently, for every edge there must be stored references
// which refer to all that edges which overlap horizontally with
// that region between x1 and the next higher x1 (x1 of the next edge
// in the sorted list);
#define border__edge_M 60004
typedef struct border__edge_t {
  int32_t x1,y1,x2,y2;  // coordinates of the edge; always: x1<x2;
  struct border__chain_t* chain;
  } border__edge_t;
  // the last element in this list will have x1==border__nil;
static border__edge_t* border__edge;
static int border__edge_n= 0;  // number of elements in border__edge[0]
#define border__chain_M (border__edge_M*8)
typedef struct border__chain_t {
  border__edge_t* edge;
  struct border__chain_t* next;
  } border__chain_t;
  // the last element in this list will have edge==NULL;
  // the last element of each chain will be terminated with next==NULL;
static border__chain_t* border__chain;

static void border__end() {
  // close this module;
  // this procedure has no parameters because we want to be able
  // to call it via atexit();

  if(border__edge!=NULL)
    free(border__edge);
  border__edge= NULL;
  border__edge_n= 0;
  if(border__chain!=NULL)
    free(border__chain);
  border__chain= NULL;
  }  // end   border__end()

static inline bool border__ini() {
  // initialize this module;
  // you may call this procedure repeatedly; only the first call
  // will have effect; subsequent calls will be ignored;
  // return: ==true: success, or the call has been ignored;
  //         ==false: an error occurred during initialization;
  static bool firstrun= true;

  if(firstrun) {
    firstrun= false;
    atexit(border__end);
    border__edge= (border__edge_t*)
      malloc((border__edge_M+4)*sizeof(border__edge_t));
    if(border__edge==NULL)
return false;
    border__chain= (border__chain_t*)
      malloc((border__chain_M+4)*sizeof(border__chain_t));
    if(border__chain==NULL)
return false;
    }
  return true;
  }  // end   border__ini()

static int border__qsort_edge(const void* a,const void* b) {
  // edge comparison for qsort()
  int32_t ax,bx;

  ax= ((border__edge_t*)a)->x1;
  bx= ((border__edge_t*)b)->x1;
  if(ax>bx)
return 1;
  if(ax==bx)
return 0;
  return -1;
  }  // end   border__qsort_edge()

//------------------------------------------------------------

static bool border_active= false;  // borders are to be considered;
  // this variable must not be written from outside of the module;

static bool border_box(const char* s) {
  // read coordinates of a border box;
  // s[]: coordinates as a string; example: "11,49,11.3,50"
  // return: success;
  double x1f,y1f;  // coordinates of southwestern corner
  double x2f,y2f;  // coordinates of northeastern corner
  int r;

  x1f= y1f= x2f= y2f= 200.1;
  r= sscanf(s,"%lG,%lG,%lG,%lG",&x1f,&y1f,&x2f,&y2f);
  if(r!=4 || x1f<-180.1 || x1f>180.1 || y1f<-90.1 || y1f>90.1 ||
      x2f<-180.1 || x2f>180.1 || y2f<-90.1 || y2f>90.1)
return false;
  border_active=true;
  border__bx1= (int32_t)(x1f*10000000L);
    // convert floatingpoint to fixpoint
  border__by1= (int32_t)(y1f*10000000L);
  border__bx2= (int32_t)(x2f*10000000L);
  border__by2= (int32_t)(y2f*10000000L);
  return true;
  }  // end   border_box()

static bool border_file(const char* fn) {
  // read border polygon file, store the coordinates, and determine
  // an enclosing border box to speed-up the calculations;
  // fn[]: file name;
  // return: success;
  static int32_t nil;

  if(!border__ini())
return false;
  nil= border__nil;

  /* get border polygon */ {
    border__edge_t* bep;  // growing pointer in border__edge[]
    border__edge_t* bee;  // memory end of border__edge[]
    FILE* fi;
    char s[80],*sp;
    int32_t x0,y0;  // coordinate of the first point in a section;
      // this is used to close an unclosed polygon;
    int32_t x1,y1;  // last coordinates
    int32_t x,y;

    border__edge[0].x1= nil;
    fi= fopen(fn,"rb");
    if(fi==NULL)
return false;
    bee= border__edge+(border__edge_M-2);
    bep= border__edge;
    x0= nil;  // (sign that there is no first coordinate at the moment)
    x1= nil;  // (sign that there is no last coordinate at the moment)
    for(;;) {  // for every line in border file
      s[0]= 0;
      sp= fgets(s,sizeof(s),fi);
      if(bep>=bee) {
        fclose(fi);
return false;
        }
      if(s[0]!=' ' && s[0]!='\t') {  // not inside a section
        if(x0!=nil && x1!=nil && (x1!=x0 || y1!=y0)) {
            // last polygon was not closed
          if(x1==x0) {  // the edge would be vertical
            // we have to insert an additional edge
            x0+= 3;
            if(x0>x1)
              { bep->x1= x1; bep->y1= y1; bep->x2= x0; bep->y2= y0; }
            else
              { bep->x1= x0; bep->y1= y0; bep->x2= x1; bep->y2= y1; }
            bep->chain= NULL;
            if(loglevel>=1)
              fprintf(stderr,
                "+ %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
                (int)(bep-border__edge),
                bep->x1,bep->y1,bep->x2,bep->y2);
            bep++;
            x1= x0; y1= y0;
            x0-= 3;
            }  // the edge would be vertical
          // close the polygon
          if(x0>x1)
            { bep->x1= x1; bep->y1= y1; bep->x2= x0; bep->y2= y0; }
          else
            { bep->x1= x0; bep->y1= y0; bep->x2= x1; bep->y2= y1; }
          bep->chain= NULL;
          if(loglevel>=1)
            fprintf(stderr,
              "c %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
              (int)(bep-border__edge),bep->x1,bep->y1,bep->x2,bep->y2);
          bep++;
          }  // end   last polygon was not closed
        x0= x1= nil;
        }  // end   not inside a section
      else {  // inside a section
        double xf,yf;

        xf= yf= 200.1;
        sscanf(s+1,"%lG %lG",&xf,&yf);
        if(xf<-180.1 || xf>180.1 || yf<-90.1 || yf>90.1) x= nil;
        else {
          x= (int32_t)(xf*10000000+0.5);
          y= (int32_t)(yf*10000000+0.5);
          }
        if(x!=nil) {  // data plausible
          if(x1!=nil) {  // there is a preceding coordinate
            if(x==x1) x+= 2;  // do not accept exact north-south
              // lines, because then we may not be able to determine
              // if a point lies inside or outside the polygon;
            if(x>x1)
              { bep->x1= x1; bep->y1= y1; bep->x2= x; bep->y2= y; }
            else
              { bep->x1= x; bep->y1= y; bep->x2= x1; bep->y2= y1; }
            bep->chain= NULL;
            if(loglevel>=1)
              fprintf(stderr,
                "- %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
                (int)(bep-border__edge),
                bep->x1,bep->y1,bep->x2,bep->y2);
            bep++;
            }  // end   there is a preceding coordinate
          x1= x; y1= y;
          if(x0==nil)
            { x0= x; y0= y; }
          }  // end   data plausible
        }  // end   inside a section
      if(sp==NULL)  // end of border file
    break;
      }  // end   for every line in border file
    fclose(fi);
    bep->x1= nil;  // set terminator of edge list
    border__edge_n= bep-border__edge;  // set number of edges
    }  // end   get border polygon

  // sort edges ascending by x1 value
  if(loglevel>=1)
    fprintf(stderr,"Border polygons: %i. Now sorting.\n",
      border__edge_n);
  qsort(border__edge,border__edge_n,sizeof(border__edge_t),
    border__qsort_edge);

  /* generate chains for each edge */ {
    int32_t x2;
    border__chain_t* bcp;  // growing pointer in chain storage
    border__edge_t* bep;  // pointer in border__edge[]
    border__edge_t* bep2;  // referenced edge
    border__chain_t* bcp2;  // chain of referenced edge;

    bep= border__edge;
    bcp= border__chain;
    while(bep->x1!=nil) {  // for each edge in list
      if(loglevel>=1)
        fprintf(stderr,
          "> %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
          (int)(bep-border__edge),bep->x1,bep->y1,bep->x2,bep->y2);
      /*x1= bep->x1;*/ x2= bep->x2;
      bep2= bep;
      while(bep2>border__edge && (bep2-1)->x1==bep2->x1) bep2--;
        // we must examine previous edges having same x1 too;
      while(bep2->x1!=nil && bep2->x1 <= x2) {
          // for each following overlapping edge in list
        if(bep2==bep) {  // own edge
          bep2++;  // (needs not to be chained to itself)
      continue;
          }
        if(bcp>=border__chain+border__chain_M)
            // no more space in chain storage
return false;
        if(loglevel>=2)
          fprintf(stderr,"+ add to chain of %i\n",
            (int)(bep2-border__edge));
        bcp2= bep2->chain;
        if(bcp2==NULL)  // no chain yet
          bep2->chain= bcp;  // add first chain link
        else {  // edge already has a chain
          // go to the chain's end and add new chain link there

          while(bcp2->next!=NULL) bcp2= bcp2->next;
          bcp2->next= bcp;
          }  // end   edge already has a chain
        bcp->edge= bep;
          // add source edge to chain of overlapping edges
        bcp->next= NULL;  // new chain termination
        bcp++;
        bep2++;
        }  // for each following overlapping  edge in list
      bep++;
      }  // end   for each edge in list
    }  // end   generate chains for each edge

  // test output
  if(loglevel>=2) {
    border__edge_t* bep,*bep2;  // pointers in border__edge[]
    border__chain_t* bcp;  // pointer in chain storage

    fprintf(stderr,"Chains:\n");
    bep= border__edge;
    while(bep->x1!=nil) {  // for each edge in list
      fprintf(stderr,
        "> %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
        (int)(bep-border__edge),bep->x1,bep->y1,bep->x2,bep->y2);
      bcp= bep->chain;
      while(bcp!=NULL) {  // for each chain link in edge
        bep2= bcp->edge;
        fprintf(stderr,
          "  %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
          (int)(bep2-border__edge),
          bep2->x1,bep2->y1,bep2->x2,bep2->y2);
        bcp= bcp->next;
        }  // end   for each chain link in edge
      bep++;
      }  // end   for each edge in list
    }  // end   test output

  /* determine enclosing border box */ {
    border__edge_t* bep;  // pointer in border__edge[]

    border__bx1= border__edge[0].x1;
    border__bx2= -2000000000L;  // (default)
    border__by1= 2000000000L; border__by2= -2000000000L;  // (default)
    bep= border__edge;
    while(bep->x1!=nil) {  // for each coordinate of the polygon
      if(bep->x2>border__bx2) border__bx2= bep->x2;
      if(bep->y1<border__by1) border__by1= bep->y1;
      if(bep->y2<border__by1) border__by1= bep->y2;
      if(bep->y1>border__by2) border__by2= bep->y1;
      if(bep->y2>border__by2) border__by2= bep->y2;
      bep++;
      }  // end   for each coordinate of the polygon
    }  // end   determine enclosing border box
  border_active=true;
  if(loglevel>=1)
    fprintf(stderr,"End of border initialization.\n");
  return true;
  }  // end   border_file()

static bool border_queryinside(int32_t x,int32_t y) {
  // determine if the given coordinate lies inside or outside the
  // border polygon(s);
  // x,y: coordinates of the given point in 0.0000001 degrees;
  // return: point lies inside the border polygon(s);
  static int32_t nil;

  nil= border__nil;

  #if MAXLOGLEVEL>=3
  if(loglevel>=3)
    fprintf(stderr,"# %li,%li\n",x,y);
  #endif
  // first, consider border box (if any)
  if(border__bx1!=nil) {  // there is a border box
    if(x<border__bx1 || x>border__bx2 ||
        y<border__by1 || y>border__by2)
        // point lies outside the border box
return false;
    }  // end   there is a border box

  /* second, consider border polygon (if any) */ {
    border__edge_t* bep;  // pointer in border__edge[]
    border__chain_t* bcp;  // pointer in border__chain[]
    int cross;  // number of the crossings a line from the point
      // to the north pole would have ageinst the border lines
      // in border__coord[][];

    if(border__edge==NULL)
return true;
    cross= 0;

    /* binary-search the edge with the closest x1 */ {
      int i,i1,i2;  // iteration indexes

      i1= 0; i2= border__edge_n;
      while(i2>i1+1) {
        i= (i1+i2)/2;
        bep= border__edge+i;
//fprintf(stderr,"s %i %i %i   %li\n",i1,i,i2,bep->x1); ///
        if(bep->x1 > x) i2= i;
        else i1= i;
//fprintf(stderr,"  %i %i %i\n",i1,i,i2); ///
        }
      bep= border__edge+i1;
      }  // end   binary-search the edge with the closest x1

    bcp= NULL;
      // (default, because we want to examine the own edge first)
    for(;;) {  // for own edge and each edge in chain
      if(bep->x1 <= x && bep->x2 > x) {  // point lies inside x-range
        if(bep->y1 > y && bep->y2 > y) {
            // line lies completely north of point
          cross++;
          #if MAXLOGLEVEL>=3
          if(loglevel>=3)
            fprintf(stderr,"= %i %li,%li,%li,%li\n",
              (int)(bep-border__edge),bep->x1,bep->y1,bep->x2,bep->y2);
          #endif
          }
        else if(bep->y1 > y || bep->y2 > y) {
            // one line end lies north of point
          if( (int64_t)(y-bep->y1)*(int64_t)(bep->x2-bep->x1) <
              (int64_t)(x-bep->x1)*(int64_t)(bep->y2-bep->y1) ) {
              // point lies south of the line
            cross++;
            #if MAXLOGLEVEL>=3
            if(loglevel>=3)
              fprintf(stderr,"/ %i %li,%li,%li,%li\n",
                (int)(bep-border__edge),
                bep->x1,bep->y1,bep->x2,bep->y2);
            #endif
            }
          #if MAXLOGLEVEL>=3
          else if(loglevel>=3)
            fprintf(stderr,". %i %li,%li,%li,%li\n",
              (int)(bep-border__edge),
              bep->x1,bep->y1,bep->x2,bep->y2);
          #endif
          }  // end   one line end north of point
        #if MAXLOGLEVEL>=3
        else if(loglevel>=3)
            fprintf(stderr,"_ %i %li,%li,%li,%li\n",
              (int)(bep-border__edge),bep->x1,bep->y1,bep->x2,bep->y2);
        #endif
        }  // end   point lies inside x-range
      if(bcp==NULL)  // chain has not been examined
        bcp= bep->chain;  // get the first chain link
      else
        bcp= bcp->next;  // get the next chain link
      if(bcp==NULL)  // no more chain links
    break;
      bep= bcp->edge;
      }  // end   for own edge and each edge in chain
//if(loglevel>=3) fprintf(stderr,"# %li,%li cross %i\n",x,y,cross);
return (cross&1)!=0;  // odd number of crossings
    }  // end   second, consider border polygon (if any)
  }  // end   border_queryinside()

static void border_querybox(int32_t* x1p,int32_t* y1p,
    int32_t* x2p,int32_t* y2p) {
  // get the values of a previously defined border box;
  // border_box() or border_file() must have been called;
  // return values are valid only if border_active==true;
  // *x1p,*y1p;  // coordinates of southwestern corner;
  // *x2p,*y2p;  // coordinates of northeastern corner;
  int32_t x1,y1,x2,y2;

  if(!border_active) {
    *x1p= *y1p= *x2p= *y2p= 0;
return;
    }
  x1= border__bx1; y1= border__by1;
  x2= border__bx2; y2= border__by2;
  // round coordinates a bit
  #define D(x) { if(x%1000==1) { if(x>0) x--; else x++; } \
    else if((x)%1000==999) { if((x)>0) x++; else x--; } }
  D(x1) D(y1) D(x2) D(y2)
  #undef D
  *x1p= x1; *y1p= y1; *x2p= x2; *y2p= y2;
  }  // end   border_querybox()

//------------------------------------------------------------
// end Module border_   OSM border module
//------------------------------------------------------------



//------------------------------------------------------------
// Module read_   OSM file read module
//------------------------------------------------------------

// this module provides procedures for buffered reading of
// standard input;
// as usual, all identifiers of a module have the same prefix,
// in this case 'read'; an underline will follow in case of a
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
#define read_GZ 3  // determines which read procedure set will be used;
  // ==0: use open();  ==1: use fopen();
  // ==2: use gzopen() (accept gzip compressed input files);
  // ==3: use gzopen() with increased gzip buffer;
typedef struct {  // members may not be accessed from external
  #if read_GZ==0
    int fd;  // file descriptor
    off_t jumppos;  // position to jump to; -1: invalid
  #elif read_GZ==1
    FILE* fi;  // file stream
    off_t jumppos;  // position to jump to; -1: invalid
  #else
    gzFile fi;  // gzip file stream
    #if __WIN32__
      z_off64_t jumppos;  // position to jump to; -1: invalid
    #else
      z_off_t jumppos;  // position to jump to; -1: invalid
    #endif
  #endif
  int64_t counter;
    // byte counter to get the read position in input file;
  char filename[300];
  bool isstdin;  // is standard input
  bool eof;  // we are at the end of input file
  byte* bufp;  // pointer in buf[]
  byte* bufe;  // pointer to the end of valid input in buf[]
  uint64_t bufferstart;
    // dummy variable which marks the start of the read buffer
    // concatenated  with this instance of read info structure;
  } read_info_t;
static bool read__jumplock= false;  // do not change .jumppos anymore;

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
  // note that you should close every opened file with read_close()
  // before the program ends;

  // save status of presently processed input file (if any)
  if(read_infop!=NULL) {
    read_infop->bufp= read_bufp;
    read_infop->bufp= read_bufe;
    }

  // get memory space for file information and input buffer
  read_infop= (read_info_t*)malloc(sizeof(read_info_t)+read__bufM);
  if(read_infop==NULL) {
    fprintf(stderr,"osmconvert Error: could not get "
      "%i bytes of memory.\n",read__bufM);
return 1;
    }

  // initialize read info structure
  #if read_GZ==0
    read_infop->fd= 0;  // (default) standard input
  #else
    read_infop->fi= NULL;  // (default) file not opened
  #endif
  if((read_infop->isstdin= filename==NULL))
    strcpy(read_infop->filename,"standard input");
  else
    strMcpy(read_infop->filename,filename);
  read_infop->eof= false;  // we are not at the end of input file
  read_infop->bufp= read_infop->bufe= read__buf;  // pointer in buf[]
    // pointer to the end of valid input in buf[]
  read_infop->counter= 0;
  read_infop->jumppos= 0;
    // store start of file as default jump destination

  // set modul-global variables which are associated with this file
  read_bufp= read_infop->bufp;
  read_bufe= read_infop->bufe;

  // open the file
  if(loglevel>=2)
    fprintf(stderr,"Read-opening: %s\n",read_infop->filename);
  if(read_infop->isstdin) {  // stdin shall be used
    #if read_GZ==0
      read_infop->fd= 0;
    #elif read_GZ==1
      read_infop->fi= stdin;
    #else
      read_infop->fi= gzdopen(0,"rb");
      #if read_GZ==3 && ZLIB_VERNUM>=0x1235
        gzbuffer(read_infop->fi,128*1024);
      #endif
    #endif
    }
  else if(filename!=NULL) {  // a real file shall be opened
    #if read_GZ==0
      read_infop->fd= open(filename,O_RDONLY|O_BINARY);
    #elif read_GZ==1
      read_infop->fi= fopen(filename,"rb");
    #else
      read_infop->fi= gzopen(filename,"rb");
      #if read_GZ==3 && ZLIB_VERNUM>=0x1235
        if(loglevel>=2)
          fprintf(stderr,"Read-opening: increasing gzbuffer.\n");
        gzbuffer(read_infop->fi,128*1024);
      #endif
    #endif
    #if read_GZ==0
    if(read_infop->fd<0) {
    #else
    if(read_infop->fi==NULL) {
    #endif
      fprintf(stderr,
        "osmconvert Error: could not open input file: %.80s\n",
        read_infop->filename);
      free(read_infop); read_infop= NULL;
      read_bufp= read_bufe= NULL;
return 1;
      }
    }  // end   a real file shall be opened
return 0;
  }  // end   read_open()

static void read_close() {
  // close an opened file;
  // read_infop: handle of the file which is to close;
  if(read_infop==NULL)  // handle not valid;
return;
  if(loglevel>=2)
    fprintf(stderr,"Read-closing: %s\n",read_infop->filename);
  #if read_GZ==0
    if(read_infop->fd>0)  // not standard input
      close(read_infop->fd);
  #elif read_GZ==1
    if(!read_infop->isstdin)  // not standard input
      fclose(read_infop->fi);
  #else
    gzclose(read_infop->fi);
  #endif
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
        #if read_GZ==0
          r= read(read_infop->fd,read_bufe,l);
        #elif read_GZ==1
          r= read(fileno(read_infop->fi),read_bufe,l);
        #else
          r= gzread(read_infop->fi,read_bufe,l);
        #endif
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
        read_infop->counter+= r;
        read_bufe+= r;  // set new mark for end of data
        read_bufe[0]= 0; read_bufe[1]= 0;  // set 4 null-terminators
        read_bufe[2]= 0; read_bufe[3]= 0;
        } while(r<l);  // end   while buffer has not been filled
      }  // end   still bytes to read
    }  // end   read buffer is too low
  return read_infop->eof && read_bufp>=read_bufe;
  }  // end   read__input()

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

static inline int read_rewind() {
  // rewind the file, i.e., the file pointer is set
  // to the first byte in the file;
  // read_infop: handle of the file which is to rewind;
  // return: ==0: ok; !=0: rewind error;
  bool err;

  #if read_GZ==0
    err= lseek(read_infop->fd,0,SEEK_SET)<0;
  #elif read_GZ==1
    err= fseek(read_infop->fi,0,SEEK_SET)<0;
  #else
    err= gzseek(read_infop->fi,0,SEEK_SET)<0;
  #endif
  if(err) {
    PERRv("could not rewind file: %-80s",read_infop->filename)
return 1;
    }
  read_infop->counter= 0;
  read_bufp= read_bufe;  // force refetch
  read_infop->eof= false;  // force retest for end of file
  read_input();  // ensure prefetch
return 0;
  }  // end   read_rewind()

static inline bool read_setjump() {
  // store the current position in the file as a destination
  // for a jump which will follow later;
  // if global_complexways is false, the call will be ignored;
  // the position is not stored anew if it has been locked
  // with read_infop->lockpos;
  // return: jump position has been stored;
  if(!global_complexways)
return false;
  if(read__jumplock)
return false;
  read_infop->jumppos= read_infop->counter-(read_bufe-read_bufp);
  return true;
  }  // end   read_setjump()

static inline void read_lockjump() {
  // prevent a previously stored jump position from being overwritten;
  read__jumplock= true;
  }  // end   read_lockjump()

static int read_jump() {
  // jump to a previously stored location it;
  // return: 0: jump ok;
  //         1: did not actually jump because we already were
  //            at the desired position;
  //         <0: error;
  #if read_GZ<2
    off_t pos;  // present position in the file;
  #else
    #if __WIN32__
      z_off64_t pos;  // position to jump to; -1: invalid
    #else
      z_off_t pos;  // position to jump to; -1: invalid
    #endif
  #endif
  bool err;

  pos= read_infop->counter-(read_bufe-read_bufp);
  if(read_infop->jumppos==-1) {
    PERRv("no jump destination in file: %.80s",read_infop->filename)
return -1;
    }
  #if read_GZ==0
    err= lseek(read_infop->fd,read_infop->jumppos,SEEK_SET)<0;
  #elif read_GZ==1
    err= fseek(read_infop->fi,read_infop->jumppos,SEEK_SET)<0;
  #else
    err= gzseek(read_infop->fi,read_infop->jumppos,SEEK_SET)<0;
  #endif
  if(err) {
    PERRv("could not jump in file: %.80s",read_infop->filename)
return -2;
    }
  if(read_infop->jumppos!=pos) {  // this was a real jump
    read_infop->counter= read_infop->jumppos;
    read_bufp= read_bufe;  // force refetch
    read_infop->eof= false;  // force retest for end of file
    read_input();  // ensure prefetch
return 0;
    }
  // here: did not actually jump because we already were
  // at the desired position
return 1;
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
// in this case 'write'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static const char* write__filename= NULL;
  // last name of the file; ==NULL: standard output;
static const char* write__filename_standard= NULL;
  // name of standard output file; ==NULL: standard output;
static const char* write__filename_temp= NULL;
  // name of the tempfile; ==NULL: no tempfile;
static char write__buf[UINT64_C(16000000)];
static char* write__bufe= write__buf+sizeof(write__buf);
  // (const) water mark for buffer filled 100%
static char* write__bufp= write__buf;
static int write__fd= 1;  // (initially standard output)
static int write__fd_standard= 1;  // (initially standard output)
static inline void write_flush();

static void write__close() {
  // close the last opened file;
  if(loglevel>=2)
    fprintf(stderr,"Write-closing FD: %i\n",write__fd);
  write_flush();
  if(write__fd>1) {  // not standard output
    close(write__fd);
    write__fd= 1;
    }
  }  // end   write__close()

static void write__end() {
  // terminate the services of this module;
  if(write__fd>1)
    write__close();
  if(write__fd_standard>1) {
    write__fd= write__fd_standard;
    write__close();
    write__fd_standard= 0;
    }
  if(loglevel<2)
    if(write__filename_temp!=NULL) unlink(write__filename_temp);
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
      fprintf(stderr,
        "osmconvert Error: could not open output file: %.80s\n",
          filename);
      write__fd= 1;
return 1;
      }
    write__fd_standard= write__fd;
    write__filename_standard= filename;
    }
  if(firstrun) {
    firstrun= false;
    atexit(write__end);
    }
  return 0;
  }  // end   write_open()

static int write_newfile(const char* filename) {
  // change to another (temporary) output file;
  // filename: new name of the output file;
  //           this string must be accessible until program end
  //           because the name will be needed to delete the file;
  //           ==NULL: change back to standard output file;
  // the previous output file is closed by this procedure, unless
  // it is standard output;
  // return: 0: OK; !=0: error;
  if(loglevel>=2)
    fprintf(stderr,"Write-opening: %s\n",
      filename==NULL? "stdout": filename);
  if(filename==NULL) {  // we are to change back to standard output file
    if(loglevel>=2)
      fprintf(stderr,"Write-reopening: %s\n",
        write__filename_standard==NULL? "stdout":
        write__filename_standard);
    write__close();  // close temporary file
    write__filename= write__filename_standard;
    write__fd= write__fd_standard;
    }
  else {  // new temporary file shall be opened
    if(loglevel>=2)
      fprintf(stderr,"Write-opening: %s\n",filename);
    write__filename= filename;
    unlink(filename);
    write__fd= open(filename,O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,00600);
    if(write__fd<1) {
      fprintf(stderr,
        "osmconvert Error: could not open output file: %.80s\n",
        filename);
      write__fd= 1;
return 2;
      }
    write__filename_temp= filename;
    }
  return 0;
  }  // end   write_newfile()

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
  #undef write__char_D
  }  // end   write_xmlstr();

static inline void write_xmlmnstr(const char* s) {
  // write an XML string to stdout, use a buffer;
  // every character which is not allowed within an XML string
  // will be replaced by the appropriate mnemonic or decimal sequence;
  static const byte allowedchar[]= {
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

#if 0  // currently unused
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

static inline void write_createsint64(int64_t v,char* sp) {
  // create a signed 64 bit integer number;
  // return:
  // sp[30]: value v as decimal integer string;
  static char *s1,*s2,c;

  s1= sp;
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
  }  // end write_sint64()

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

static inline char* write_createsfix7o(int32_t v,char* s) {
  // convert a signed 7 decimals fixpoint value into a string;
  // keep trailing zeros;
  // v: fixpoint value
  // return: pointer do string terminator;
  // s[12]: destination string;
  char* s1,*s2,*sterm,c;
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
  sterm= s2;
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  return sterm;
  }  // end write_createsfix7o()

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

static inline void write_createtimestamp(uint64_t v,char* sp) {
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
  }  // end   write_createtimestamp()

static inline void write_timestamp(uint64_t v) {
  // write a timestamp in OSM format, e.g.: "2010-09-30T19:23:30Z"
  char s[30];

  write_createtimestamp(v,s);
  write_str(s);
  }  // end   write_timestamp()

//------------------------------------------------------------
// end   Module write_   write module
//------------------------------------------------------------



//------------------------------------------------------------
// Module csv_   csv write module
//------------------------------------------------------------

// this module provides procedures for generating csv output;
// as usual, all identifiers of a module have the same prefix,
// in this case 'csv'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

#define csv__keyM 200  // max number of keys and vals
#define csv__keyMM 256  // max number of characters +1 in key or val
static char* csv__key= NULL;  // [csv__keyM][csv__keyMM]
static int csv__keyn= 0;  // number of keys
static char* csv__val= NULL;  // [csv__keyM][csv__keyMM]
static int csv__valn= 0;  // number of vals
// some booleans which tell us if certain keys are in column list;
// this is for program acceleration
static bool csv_key_otype= false, csv_key_oname= false,
  csv_key_id= false, csv_key_lon= false, csv_key_lat= false,
  csv_key_version=false, csv_key_timestamp=false,
  csv_key_changeset=false, csv_key_uid= false, csv_key_user= false;
static char csv__sep0= '\t';  // first character of global_csvseparator;
static char csv__rep0= ' ';  // replacement character for separator char

static void csv__end() {
  // clean-up csv processing;

  if(csv__key!=NULL)
    { free(csv__key); csv__key= NULL; }
  if(csv__val!=NULL)
    { free(csv__val); csv__val= NULL; }
  }  // end   csv__end()

//------------------------------------------------------------

static int csv_ini(const char* columns) {
  // initialize this module;
  // must be called before any other procedure is called;
  // may be called more than once; only the first call will
  // initialize this module, every other call will solely
  // overwrite the columns information  if !=NULL;
  // columns[]: space-separated list of keys who are to be
  //            used as column identifiers;
  //            ==NULL: if list has already been given, do not
  //                    change it; if not, set list to default;
  // return: 0: everything went ok;
  //         !=0: an error occurred;
  static bool firstrun= true;

  if(firstrun) {
    firstrun= false;

    csv__key= (char*)malloc(csv__keyM*csv__keyMM);
    csv__val= (char*)malloc(csv__keyM*csv__keyMM);
    if(csv__key==NULL || csv__val==NULL)
return 1;
    atexit(csv__end);
    }
  if(columns==NULL) {  // default columns shall be set
    if(csv__keyn==0) {  // until now no column has been set
      // set default columns
      strcpy(&csv__key[0*csv__keyMM],"@oname");
      csv_key_oname= true;
      strcpy(&csv__key[1*csv__keyMM],"@id");
      csv_key_id= true;
      strcpy(&csv__key[2*csv__keyMM],"name");
      csv__keyn= 3;
      }  // until now no column has been set
    }  // default columns shall be set
  else {  // new columns shall be set
    for(;;) {  // for each column name
      int len;
      char* tp;

      len= strcspn(columns," ");
      if(len==0)
    break;
      if(csv__keyn>=csv__keyM) {
        WARN("too many csv columns")
    break;
        }
      len++;
      if(len>csv__keyMM) len= csv__keyMM;  // limit key length
      tp= &csv__key[csv__keyn*csv__keyMM];
      strmcpy(tp,columns,len);
      csv__keyn++;
      if(strcmp(tp,"@otype")==0) csv_key_otype= true;
      else if(strcmp(tp,"@oname")==0) csv_key_oname= true;
      else if(strcmp(tp,"@id")==0) csv_key_id= true;
      else if(strcmp(tp,"@lon")==0) csv_key_lon= true;
      else if(strcmp(tp,"@lat")==0) csv_key_lat= true;
      else if(strcmp(tp,"@version")==0) csv_key_version= true;
      else if(strcmp(tp,"@timestamp")==0) csv_key_timestamp= true;
      else if(strcmp(tp,"@changeset")==0) csv_key_changeset= true;
      else if(strcmp(tp,"@uid")==0) csv_key_uid= true;
      else if(strcmp(tp,"@user")==0) csv_key_user= true;
      columns+= len-1;
      if(columns[0]==' ') columns++;
      }  // for each column name
    }  // new columns shall be set
  // care about separator chars
  if(global_csvseparator[0]==0 || global_csvseparator[1]!=0) {
    csv__sep0= 0;
    csv__rep0= 0;
    }
  else {
    csv__sep0= global_csvseparator[0];
    if(csv__sep0==' ')
      csv__rep0= '_';
    else
      csv__rep0= ' ';
    }
  return 0;
  }  // end   csv_ini()

static void csv_add(const char* key,const char* val) {
  // test if the key's value shall be printed and do so if yes;
  int keyn;
  const char* kp;

  keyn= csv__keyn;
  kp= csv__key;
  while(keyn>0) {  // for all keys in column list
    if(strcmp(key,kp)==0) {  // key is in column list
      strmcpy(csv__val+(kp-csv__key),val,csv__keyMM);
        // store value
      csv__valn++;
  break;
      }  // key is in column list
    kp+= csv__keyMM;  // take next key in list
    keyn--;
    }  // for all keys in column list
  }  // end   csv_add()

static void csv_write() {
  // write a csv line - if csv data had been stored
  char* vp,*tp;
  int keyn;

  if(csv__valn==0)
return;
  vp= csv__val;
  keyn= csv__keyn;
  while(keyn>0) {  // for all keys in column list
    if(*vp!=0) {  // there is a value for this key
      tp= vp;
      do {
        if(*tp==csv__sep0 || *tp==NL[0] || *tp==NL[1])
              // character identical with separator or line feed
          write_char(csv__rep0);  // replace it by replacement char
        else
          write_char(*tp);
        tp++;
        } while(*tp!=0);
      *vp= 0;  // delete list entry
      }
    vp+= csv__keyMM;  // take next val in list
    keyn--;
    if(keyn>0)  // at least one column will follow
      write_str(global_csvseparator);
    }  // for all keys in column list
  write_str(NL);
  csv__valn= 0;
  }  // end   csv_write()

static void csv_headline() {
  // write a headline to csv output file
  char* kp;
  int keyn;

  if(!global_csvheadline)  // headline shall not be written
return;
  kp= csv__key;
  keyn= csv__keyn;
  while(keyn>0) {  // for all keys in column list
    csv_add(kp,kp);
    kp+= csv__keyMM;  // take next key in list
    keyn--;
    }  // for all keys in column list
  csv_write();
  }  // end   csv_headline()

//------------------------------------------------------------
// end   Module csv_   csv write module
//------------------------------------------------------------



//------------------------------------------------------------
// Module pb_   pbf read module
//------------------------------------------------------------

// this module provides procedures which read osm .pbf objects;
// it uses procedures from modules read_ and pbf_;
// as usual, all identifiers of a module have the same prefix,
// in this case 'pb'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static int pb__decompress(byte* ibuf,uint isiz,byte* obuf,uint osizm,
  uint* osizp) {
  // decompress a block of data;
  // return: 0: decompression was successful;
  //         !=0: error number from zlib;
  // *osizp: size of uncompressed data;
  z_stream strm;
  int r,i;

  // initialization
  strm.zalloc= Z_NULL;
  strm.zfree= Z_NULL;
  strm.opaque= Z_NULL;
  strm.next_in= Z_NULL;
  strm.total_in= 0;
  strm.avail_out= 0;
  strm.next_out= Z_NULL;
  strm.total_out= 0;
  strm.msg= NULL;
  r= inflateInit(&strm);
  if(r!=Z_OK)
return r;
  // read data
  strm.next_in = ibuf;
  strm.avail_in= isiz;
  // decompress
  strm.next_out= obuf;
  strm.avail_out= osizm;
  r= inflate(&strm,Z_FINISH);
  if(r!=Z_OK && r!=Z_STREAM_END) {
    inflateEnd(&strm);
    *osizp= 0;
return r;
    }
  // clean-up
  inflateEnd(&strm);
  obuf+= *osizp= osizm-(i= strm.avail_out);
  // add some zero bytes
  if(i>4) i= 4;
  while(--i>=0) *obuf++= 0;
  return 0;
  }  // end   pb__decompress()

static inline int64_t pb__strtimetosint64(const char* s) {
  // read a timestamp in OSM format, e.g.: "2010-09-30T19:23:30Z",
  // and convert it to a signed 64-bit integer;
  // return: time as a number (seconds since 1970);
  //         ==0: syntax error;
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
    // use replcement for timegm() because Windows does not know it
return mktime(&tm)-timezone;
    #else
return timegm(&tm);
    #endif
    }  // regular timestamp
  }  // end   pb__strtimetosint64()

// for string primitive group table
#define pb__strM (4*1024*1024)
  // maximum number of strings within each block
static char* pb__str[pb__strM];  // string table
static char** pb__stre= pb__str;  // end of data in str[]
static char** pb__stree= pb__str+pb__strM;  // end of str[]
static int pb__strm= 0;
// for tags of densnodes (start and end address)
static byte* pb__nodetags= NULL,*pb__nodetagse= NULL;  // node tag pairs
// for noderefs and tags of ways (start and end address each)
static byte* pb__waynode= NULL,*pb__waynodee= NULL;
static byte* pb__waykey= NULL,*pb__waykeye= NULL;
static byte* pb__wayval= NULL,*pb__wayvale= NULL;
// for refs and tags of relations (start and end address each)
static byte* pb__relrefrole= NULL,*pb__relrefrolee= NULL;
static byte* pb__relrefid= NULL,*pb__relrefide= NULL;
static byte* pb__relreftype= NULL,*pb__relreftypee= NULL;
static byte* pb__relkey= NULL,*pb__relkeye= NULL;
static byte* pb__relval= NULL,*pb__relvale= NULL;

//------------------------------------------------------------

static bool pb_bbvalid= false;
  // the following bbox coordinates are valid;
static int32_t pb_bbx1,pb_bby1,pb_bbx2,pb_bby2;
  // bbox coordinates (base 10^-7);
static uint64_t pb_filetimestamp= 0;
static int pb_type= -9;  // type of the object which has been read;
  // 0: node; 1: way; 2: relation; 8: header;
  // -1: end of file; <= -10: error;
static int64_t pb_id= 0;  // id of read object
static int32_t pb_lon= 0,pb_lat= 0;  // coordinates of read node
static int32_t pb_hisver= 0;
static int64_t pb_histime= 0;
static int64_t pb_hiscset= 0;
static uint32_t pb_hisuid= 0;
static char* pb_hisuser= "";
static int32_t pb_hisvis= -1;  // (default for 'unknown')

static void pb_ini() {
  // initialize this module;
  // must be called as first procedure of this module;
  }  // end   pb_ini()

static int pb_input(bool reset) {
  // read next pbf object and make it available via other
  // procedures of this mudule;
  // pb_ini() must have been called before calling this procedure;
  // reset: just reset al buffers, do nothing else;
  //        this is if the file has been rewound;
  // return: >=0: OK; -1: end of file; <=-10: error; 
  // pb_type: type of the object which has been read;
  // in dependence of object's type the following information
  // will be available:
  // pb_bbvalid: the following bbox coordinates are valid;
  // pb_bbx1,pb_bby1,pb_bbx2,pb_bby2: bbox coordinates (base 10^-7);
  // pb_filetimestamp: timestamp of the file; 0: no file timestamp;
  // pb_id: id of this object;
  // pb_lon: latitude in 100 nanodegree;
  // pb_lat: latitude in 100 nanodegree;
  // pb_hisver: version;
  // pb_histime: time (seconds since 1970)
  // pb_hiscset: changeset
  // pb_hisuid: uid; ==0: no user information available;
  // pb_hisuser: user name
  // subsequent to calling this procedure, the caller may call
  // the following procedures - depending on pb_type():
  // pb_noderef(), pb_ref(), pb_keyval()
  // the caller may omit these subsequent calls for ways and relations,
  // but he must not temporarily omit them for nodes;
  // if he omits such a subsequent call for one node, he must not
  // call pb_keyval() for any other of the following nodes because
  // this would result in wrong key/val data;
  #define END(r) {pb_type= (r); goto end;}
    // jump to procedure's end and provide a return code
  #define ENDE(r,f) { PERR(f) END(r) }
    // print error message, then jump to end
  #define ENDEv(r,f,...) { PERRv(f,__VA_ARGS__) END(r) }
    // print error message with value(s), then jump to end
  int blocktype= -1;
    // -1: expected; 0: unknown; 1: Header; 2: Data;
  #define pb__blockM (32*1024*1024)  // maximum block size
  static byte zbuf[pb__blockM+1000];
  static byte* zbufp= zbuf,*zbufe= zbuf;
  static byte* groupp= zbuf,*groupe= zbuf;
    // memory area for primitive groups
  // start and end of different arrays, all used for dense nodes:
  static byte* nodeid= NULL,*nodeide= NULL;  // node ids
  static byte* nodever= NULL,*nodevere= NULL;  // versions
  static byte* nodetime= NULL,*nodetimee= NULL;  // times
  static byte* nodecset= NULL,*nodecsete= NULL;  // change sets
  static byte* nodeuid= NULL,*nodeuide= NULL;  // user ids
  static byte* nodeuser= NULL,*nodeusere= NULL;  // user names
  static byte* nodevis= NULL,*nodevise= NULL;  // visible
  static byte* nodelat= NULL,*nodelate= NULL;  // node latitudes
  static byte* nodelon= NULL,*nodelone= NULL;  // node longitudes
  static uint32_t hisuser= 0;  // string index of user name (delta coded)
  static bool waycomplete= false,relcomplete= false;

  if(reset) {
    zbufp= zbuf,zbufe= zbuf;
    groupp= zbuf,groupe= zbuf;
    nodeid= NULL,nodeide= NULL;
    nodever= NULL,nodevere= NULL;
    nodetime= NULL,nodetimee= NULL;
    nodecset= NULL,nodecsete= NULL;
    nodeuid= NULL,nodeuide= NULL;
    nodeuser= NULL,nodeusere= NULL;
    nodevis= NULL,nodevise= NULL;
    nodelat= NULL,nodelate= NULL;
    nodelon= NULL,nodelone= NULL;
    hisuser= 0;
    waycomplete= false,relcomplete= false;
    pb_type= 99;
return 0;
    }
  for(;;) {  // until we have a new object
  mainloop:
    if(nodeid<nodeide && nodelat<nodelate &&
        nodelon<nodelone) {  // dense nodes left
      // provide a node
      pb_id+= pbf_sint64(&nodeid);
      if(global_pbfgranularity100!=0) {
        pb_lat+= pbf_sint32(&nodelat)*global_pbfgranularity100;
        pb_lon+= pbf_sint32(&nodelon)*global_pbfgranularity100;
        }
      else {
        pb_lat+= pbf_sint32(&nodelat);
        pb_lon+= pbf_sint32(&nodelon);
        }
      if(nodever>=nodevere || nodetime>=nodetimee ||
          nodecset>=nodecsete || nodeuid>=nodeuide ||
          nodeuser>=nodeusere)  // no author information available
        pb_hisver= 0;
      else {  // author information available
        pb_hisver= pbf_uint32(&nodever);
        pb_histime+= pbf_sint64(&nodetime);
        pb_hiscset+= pbf_sint64(&nodecset);
        pb_hisuid+= pbf_sint32(&nodeuid);
        hisuser+= pbf_sint32(&nodeuser);
        if(hisuser<pb__strm)  // string index ok
          pb_hisuser= pb__str[hisuser];
        else {  // string index overflow
          WARNv("node %"PRIi64" user string index overflow: %u>=%i",
            pb_id,hisuser,pb__strm)
          hisuser= 0; pb_hisuser= "";
          }
        pb_hisvis= -1;
        if(nodevis!=NULL && nodevis<nodevise) {
          pb_hisvis= pbf_uint32(&nodevis);
          }
        }  // end   author information available
      END(0)
      }  // dense nodes left
    if(waycomplete) {  // ways left
      // provide a way
      waycomplete= false;
      // (already got id and author integers)
      if(pb_hisver!=0 && hisuser>0) {
          // author information available
        if(hisuser<pb__strm)  // string index ok
          pb_hisuser= pb__str[hisuser];
        else {  // string index overflow
          WARNv("way %"PRIi64" user string index overflow: %u>=%i",
            pb_id,hisuser,pb__strm)
          hisuser= 0; pb_hisuser= "";
          }
        }  // end   author information available
      END(1)
      }  // ways left
    if(relcomplete) {  // relations left
      // provide a relation
      relcomplete= false;
      // (already got id and author integers)
      if(pb_hisver!=0 && hisuser>0) {
          // author information available
        if(hisuser<pb__strm)  // string index ok
          pb_hisuser= pb__str[hisuser];
        else {  // string index overflow
          WARNv("rel %"PRIi64" user string index overflow: %u>=%i",
            pb_id,hisuser,pb__strm)
          hisuser= 0; pb_hisuser= "";
          }
        }  // end   author information available
      END(2)
      }  // relations left
    if(groupp<groupe) {  // data in primitive group left
//// provide a primitive group object
      byte* bp;
      int l;

      bp= groupp;
      while(bp<groupe) {  // for each element in primitive group
        switch(bp[0]) {  // first byte of primitive group element
        case 0x0a:  // S 1, normal nodes
          ENDE(-201,"can only process dense nodes.")
  //// dense nodes
        case 0x12:  // S 2, dense nodes
          bp++;
          l= pbf_uint32(&bp);
          if(bp+l>groupe)
            ENDEv(-202,"dense nodes too large: %u",l)
          groupp= bp+l;
          /* get all node data lists */ {
            // decode dense node part of primitive group of Data block
            byte* dne;  // end of dense node memory area
            uint l;
            byte* bhise;  // end of author section in buf[]

            dne= groupp;
            while(bp<dne) {  // for every element in this loop
              switch(bp[0]) {  // first byte of element
              case 0x0a:  // S 1, ids
                bp++;
                l= pbf_uint32(&bp);
                if(bp+l>dne)
                  ENDEv(-301,"node id table too large: %u",l)
                nodeid= bp;
                nodeide= (bp+= l);
                break;
              case 0x2a:  // S 5, author - with subelements
                bp++;
                l= pbf_uint32(&bp);
                if(bp+l>dne)
                  ENDEv(-302,"node author section too large: %u",l)
                if(global_dropversion) {
                    // version number is not required
                  bp+= l;  // jump over this section and ignore it
                  break;
                  }
                bhise= bp+l;
                nodevis= NULL;
                while(bp<bhise) {  // for each author subelement
                  switch(bp[0]) {
                      // first byte of element in author section
                  case 0x0a:  // S 1, versions
                    bp++;
                    l= pbf_uint32(&bp);
                    if(bp+l>bhise)
                      ENDEv(-303,"node version table too large: %u",l)
                    nodever= bp;
                    nodevere= (bp+= l);
                    break;
                  case 0x12:  // S 2, times
                    bp++;
                    l= pbf_uint32(&bp);
                    if(bp+l>bhise)
                      ENDEv(-304,"node time table too large: %u",l)
                    nodetime= bp;
                    nodetimee= (bp+= l);
                    break;
                  case 0x1a:  // S 3, change sets
                    bp++;
                    l= pbf_uint32(&bp);
                    if(bp+l>bhise)
                      ENDEv(-305,
                        "node change set table too large: %u",l)
                    nodecset= bp;
                    nodecsete= (bp+= l);
                    break;
                  case 0x22:  // S 4, user ids
                    bp++;
                    l= pbf_uint32(&bp);
                    if(bp+l>bhise)
                      ENDEv(-306,"node user id table too large: %u",l)
                    nodeuid= bp;
                    nodeuide= (bp+= l);
                    break;
                  case 0x2a:  // S 5, user names
                    bp++;
                    l= pbf_uint32(&bp);
                    if(bp+l>bhise)
                      ENDEv(-307,"node user name table too large: %u",l);
                    nodeuser= bp;
                    nodeusere= (bp+= l);
                    break;
                  case 0x32:  // S 6, visible
                    bp++;
                    l= pbf_uint32(&bp);
                    if(bp+l>bhise)
                      ENDEv(-308,"node version table too large: %u",l)
                    nodevis= bp;
                    nodevise= (bp+= l);
                    break;
                  default:
                    WARNv("node author element type unknown: "
                          "0x%02X 0x%02X.",bp[0],bp[1])
                    if(pbf_jump(&bp))
                      END(-308)
                    }  // end   first byte of element
                  }  // end   for each author subelement
                if(bp>bhise)
                  ENDE(-309,"node author format length.")
                bp= bhise;
                break;  // end   author - with subelements
              case 0x42:  // S 8, latitudes
                bp++;
                l= pbf_uint32(&bp);
                if(bp+l>dne)
                  ENDEv(-310,"node latitude table too large: %u",l)
                nodelat= bp;
                nodelate= (bp+= l);
                break;
              case 0x4a:  // S 9, longitudes
                bp++;
                l= pbf_uint32(&bp);
                if(bp+l>dne)
                  ENDEv(-311,"node longitude table too large: %u",l)
                nodelon= bp;
                nodelone= (bp+= l);
                break;
              case 0x52:  // S 10, tag pairs
                bp++;
                l= pbf_uint32(&bp);
                if(bp+l>dne)
                  ENDEv(-312,"node tag pair table too large: %u",l)
                pb__nodetags= bp;
                pb__nodetagse= (bp+= l);
                break;
              default:
                WARNv("dense node element type unknown: "
                  "0x%02X 0x%02X.",bp[0],bp[1])
                if(pbf_jump(&bp))
                  END(-313)
                }  // end   first byte of element
              if(bp>dne)
                ENDE(-314,"dense node format length.")
              }  // end   for every element in this loop
            // reset (delta-coded) variables
            pb_id= 0;
            pb_lat= pb_lon= 0;
            pb_histime= 0;
            pb_hiscset= 0;
            pb_hisuid= 0;
            hisuser= 0;
            pb_hisuser= "";
            bp= groupp;
            if(nodeid<nodeide && nodelat<nodelate && nodelon<nodelone)
                // minimum contents available
              goto mainloop;
            }  // get all node data lists
          break;
  //// ways
        case 0x1a:  // S 3, ways
          bp++;
          l= pbf_uint32(&bp);
          if(bp+l>groupe)
            ENDEv(-204,"ways too large: %u",l)
          groupp= bp+l;
          /* get way data */ {
            byte* bpe;  // end of ways memory area
            uint l;
            byte* bhise;  // end of author section in zbuf[]
            int complete;
              // flags which determine if the dataset is complete
            int hiscomplete;
              // flags which determine if the author is complete

            bpe= groupp;
            complete= hiscomplete= 0;
            while(bp<bpe) {  // for every element in this primitive group
              switch(bp[0]) {  // first byte of element
              case 0x08:  // V 1, id
                bp++;
                pb_id= pbf_uint64(&bp);
                complete|= 1;
                break;
              case 0x12:  // S 2, keys
                bp++;
                l= pbf_uint32(&bp);
                if(bp+l>bpe)
                  ENDEv(-401,"way key table too large: %u",l)
                pb__waykey= bp;
                pb__waykeye= (bp+= l);
                complete|= 2;
                break;
              case 0x1a:  // S 3, vals
                bp++;
                l= pbf_uint32(&bp);
                /* deal with strange S 3 element at data set end */ {
                  if(complete & (4|16)) {
                      // already have vals or node refs
                    WARNv("format 0x1a found: %02X",complete)
                    break;  // ignore this element
                    }
                  }
                if(bp+l>bpe)
                  ENDEv(-403,"way val table too large: %u",l)
                pb__wayval= bp;
                pb__wayvale= (bp+= l);
                complete|= 4;
                break;
              case 0x22:  // S 4, author - with subelements
                bp++;
                l= pbf_uint32(&bp);
                if(bp+l>bpe)
                  ENDEv(-404,"way author section too large: %u",l)
                if(global_dropversion) {
                    // version number is not required
                  bp+= l;  // jump over this section and ignore it
                  break;
                  }
                bhise= bp+l;
                pb_hisvis= -1;
                while(bp<bhise) {  // for each author subelement
                  switch(bp[0]) {
                      // first byte of element in author section
                  case 0x08:  // V 1, version
                    bp++;
                    pb_hisver= pbf_uint32(&bp);
                    hiscomplete|= 1;
                    break;
                  case 0x10:  // V 2, timestamp
                    bp++;
                    pb_histime= pbf_uint64(&bp);
                    hiscomplete|= 2;
                    break;
                  case 0x18:  // V 3, cset
                    bp++;
                    pb_hiscset= pbf_uint64(&bp);
                    hiscomplete|= 4;
                    break;
                  case 0x20:  // V 4, uid
                    bp++;
                    pb_hisuid= pbf_uint32(&bp);
                    hiscomplete|= 8;
                    break;
                  case 0x28:  // V 5, user
                    bp++;
                    hisuser= pbf_uint32(&bp);
                    hiscomplete|= 16;
                    break;
                  case 0x30:  // V 6, visible
                    bp++;
                    pb_hisvis= pbf_uint32(&bp);
                    break;
                  default:
                    WARNv("way author element type unknown: "
                      "0x%02X 0x%02X.",bp[0],bp[1])
                    if(pbf_jump(&bp))
                      END(-408)
                    }  // end   first byte of element
                  }  // end   for each author subelement
                if(bp>bhise)
                  ENDE(-411,"way author format length.")
                bp= bhise;
                complete|= 8;
                break;  // end   author - with subelements
              case 0x42:  // S 8, node refs
                bp++;
                l= pbf_uint32(&bp);
                if(bp+l>bpe)
                  ENDEv(-412,"way noderef table too large: %u",l)
                pb__waynode= bp;
                pb__waynodee= (bp+= l);
                complete|= 16;
                break;
              default:
                WARNv("way element type unknown: "
                  "0x%02X 0x%02X 0x%02X 0x%02X + %i.",
                  bp[0],bp[1],bp[2],bp[3],complete)
                if(pbf_jump(&bp))
                  END(-421)
                }  // end   first byte of element
              if(bp>bpe)
                ENDE(-429,"way format length.")
              }  // for every element in this primitive group
            bp= groupp;
            if((hiscomplete&7)!=7)  // author information not available
              pb_hisver= 0;
            else if((hiscomplete&24)!=24)  // no user information
              pb_hisuid= 0;
            #if 1  // 2014-06-16
            if((complete & 1)==1) {  // minimum contents available
                // (at least id)
            #else
            if((complete & 17)==17) {  // minimum contents available
                // (at least id and node refs)
            #endif
              waycomplete= true;
              goto mainloop;
              }
            }  // get way data
          break;
  //// relations
        case 0x22:  // S 4, rels
          bp++;
          l= pbf_uint32(&bp);
          if(bp+l>groupe)
            ENDEv(-206,"rels too large: %u",l)
          groupp= bp+l;
          /* get relation data */ {
            byte* bpe;  // end of ways memory area
            uint l;
            byte* bhise;  // end of author section in zbuf[]
            int complete;
              // flags which determine if the dataset is complete
            int hiscomplete;  // flags which determine
              // if the author information is complete

            bpe= groupp;
            complete= hiscomplete= 0;
            while(bp<bpe) {  // for every element in this primitive group
              switch(bp[0]) {  // first byte of element
              case 0x08:  // V 1, id
                bp++;
                pb_id= pbf_uint64(&bp);
                complete|= 1;
                break;
              case 0x12:  // S 2, keys
                bp++;
                l= pbf_uint32(&bp);
                if(bp+l>bpe)
                  ENDEv(-501,"rel key table too large: %u",l)
                pb__relkey= bp;
                pb__relkeye= (bp+= l);
                complete|= 2;
                break;
              case 0x1a:  // S 3, vals
                bp++;
                l= pbf_uint32(&bp);
                if(bp+l>bpe)
                  ENDEv(-502,"rel val table too large: %u",l)
                pb__relval= bp;
                pb__relvale= (bp+= l);
                complete|= 4;
                break;
              case 0x22:  // S 4, author - with subelements
                bp++;
                l= pbf_uint32(&bp);
                if(bp+l>bpe)
                  ENDEv(-503,"rel author section too large: %u",l)
                if(global_dropversion) {
                    // version number is not required
                  bp+= l;  // jump over this section and ignore it
                  break;
                  }
                bhise= bp+l;
                pb_hisvis= -1;
                while(bp<bhise) {  // for each author subelement
                  switch(bp[0]) {
                    // first byte of element in author section
                  case 0x08:  // V 1, version
                    bp++;
                    pb_hisver= pbf_uint32(&bp);
                    hiscomplete|= 1;
                    break;
                  case 0x10:  // V 2, timestamp
                    bp++;
                    pb_histime= pbf_uint64(&bp);
                    hiscomplete|= 2;
                    break;
                  case 0x18:  // V 3, cset
                    bp++;
                    pb_hiscset= pbf_uint64(&bp);
                    hiscomplete|= 4;
                    break;
                  case 0x20:  // V 4, uid
                    bp++;
                    pb_hisuid= pbf_uint32(&bp);
                    hiscomplete|= 8;
                    break;
                  case 0x28:  // V 5, user
                    bp++;
                    hisuser= pbf_uint32(&bp);
                    hiscomplete|= 16;
                    break;
                  case 0x30:  // V 6, visible
                    bp++;
                    pb_hisvis= pbf_uint32(&bp);
                    break;
                  default:
                    WARNv("rel author element type unknown: "
                      "0x%02X 0x%02X.",bp[0],bp[1])
                    if(pbf_jump(&bp))
                      END(-509)
                    }  // end   first byte of element
                  }  // end   for each author subelement
                if(bp>bhise)
                  ENDE(-510,"rel author format length.")
                bp= bhise;
                complete|= 8;
                break;  // end   author - with subelements
              case 0x42:  // S 8, refroles
                bp++;
                l= pbf_uint32(&bp);
                if(bp+l>bpe)
                  ENDEv(-511,"rel role table too large: %u",l)
                pb__relrefrole= bp;
                pb__relrefrolee= (bp+= l);
                complete|= 16;
                break;
              case 0x4a:  // S 9, refids
                bp++;
                l= pbf_uint32(&bp);
                if(bp+l>bpe)
                  ENDEv(-512,"rel id table too large: %u",l)
                pb__relrefid= bp;
                pb__relrefide= (bp+= l);
                complete|= 32;
                break;
              case 0x52:  // S 10, reftypes
                bp++;
                l= pbf_uint32(&bp);
                if(bp+l>bpe)
                  ENDEv(-513,"rel type table too large: %u",l)
                pb__relreftype= bp;
                pb__relreftypee= (bp+= l);
                complete|= 64;
                break;
              default:
                WARNv("rel element type unknown: "
                  "0x%02X 0x%02X 0x%02X 0x%02X + %i.",
                  bp[0],bp[1],bp[2],bp[3],complete)
                if(pbf_jump(&bp))
                  END(-514)
                }  // end   first byte of element
              if(bp>bpe)
                ENDE(-519,"rel format length.")
              }  // for every element in this primitive group
            bp= groupp;
            if((hiscomplete&7)!=7)  // author information not available
              pb_hisver= 0;
            else if((hiscomplete&24)!=24)  // no user information
              pb_hisuid= 0;
            #if 1
            if((complete & 1)==1) {  // minimum contents available (id)
            #else
            if((complete & 113)==113 ||
                (complete & 7)==7) {  // minimum contents available
                // have at least id and refs (1|16|32|64) OR
                // have at least id and keyval (1|2|4)
            #endif
              relcomplete= true;
              goto mainloop;
              }
            }  // get way data
          break;
        default:
          WARNv("primitive group element type unknown: "
                "0x%02X 0x%02X.",bp[0],bp[1])
          if(pbf_jump(&bp))
            END(-209)
          }  // end   first byte of primitive group element
        }  // end   for each element in primitive group
      }  // data in primitive group left
    if(zbufp<zbufe) {  // zbuf data left
//// provide next primitive group
      if(blocktype==1) {  // header block
        bool osmschema,densenodes;
        byte* bp;
        uint l;
        byte* bboxe;
        int64_t coord;
          // temporary, for rounding purposes sfix9 -> sfix7
        int bboxflags;

        osmschema= false;
        densenodes= false;
        bboxflags= 0;
        bp= zbufp;
        while(bp<zbufe) {  // for every element in this loop
          switch(bp[0]) {  // first byte of element
          case 0x0a:  // S 1, bbox
            bp++;
            l= pbf_uint32(&bp);
            if(l>=100)  // bbox group too large
              ENDEv(-41,"bbox group too large: %u",l)
            bboxe= bp+l;
            while(bp<bboxe) {  // for every element in bbox
              switch(bp[0]) {  // first byte of element in bbox
              case 0x08:  // V 1, minlon
                bp++;
                coord= pbf_sint64(&bp);
                if(coord<0) coord-= 99;
                pb_bbx1= coord/100;
                bboxflags|= 0x01;
                break;
              case 0x10:  // V 2, maxlon
                bp++;
                coord= pbf_sint64(&bp);
                if(coord>0) coord+= 99;
                pb_bbx2= coord/100;
                bboxflags|= 0x02;
                break;
              case 0x18:  // V 3, maxlat
                bp++;
                coord= pbf_sint64(&bp);
                if(coord>0) coord+= 99;
                pb_bby2= coord/100;
                bboxflags|= 0x04;
                break;
              case 0x20:  // V 4, minlat
                bp++;
                coord= pbf_sint64(&bp);
                if(coord<0) coord-= 99;
                pb_bby1= coord/100;
                bboxflags|= 0x08;
                break;
              default:
                WARNv("bbox element type unknown: "
                  "0x%02X 0x%02X.",bp[0],bp[1])
                if(pbf_jump(&bp))
                  END(-42)
                }  // end   first byte of element
              if(bp>bboxe)
                ENDE(-43,"bbox format length.")
              }  // end   for every element in bbox
            bp= bboxe;
            break;
          case 0x22:  // S 4, required features
            bp++;
            l= pbf_uint32(&bp);
            if(memcmp(bp-1,"\x0e""OsmSchema-V0.6",15)==0)
              osmschema= true;
            else if(memcmp(bp-1,"\x0a""DenseNodes",11)==0)
              densenodes= true;
            else if(memcmp(bp-1,"\x15""HistoricalInformation",21)==0)
              ;
            else  // unsupported feature
              ENDEv(-44,"unsupported feature: %.*s",l>50? 50: l,bp)
            bp+= l;
            break;
          case 0x2a:  // 0x01 S 5, optional features
            bp++;
            l= pbf_uint32(&bp);
            if(memcmp(bp-1,"\x1e""timestamp=",11)==0) {
                // file timestamp available
              pb_filetimestamp= pb__strtimetosint64((char*)bp+10);
              }  // file timestamp available
            bp+= l;
            break;
          case 0x82:  // 0x01 S 16, writing program
            if(bp[1]!=0x01) goto h_unknown;
            bp+= 2;
            l= pbf_uint32(&bp);
            bp+= l;  // (ignore this element)
            break;
          case 0x8a:  // 0x01 S 17, source
            if(bp[1]!=0x01) goto h_unknown;
            bp+= 2;
            l= pbf_uint32(&bp);
            bp+= l;  // (ignore this element)
            break;
          case 0x80:  // 0x02 V 32, osmosis_replication_timestamp
            if(bp[1]!=0x02) goto h_unknown;
            bp+= 2;
            pb_filetimestamp= pbf_uint64(&bp);
            break;
          case 0x88:  // 0x02 V 33, osmosis_replication_sequence_number
            if(bp[1]!=0x02) goto h_unknown;
            bp+= 2;
            pbf_uint64(&bp);  // (ignore this element)
            break;
          case 0x92:  // 0x02 S 34, osmosis_replication_base_url
            if(bp[1]!=0x02) goto h_unknown;
            bp+= 2;
            l= pbf_uint32(&bp);
            bp+= l;  // (ignore this element)
            break;
          default:
          h_unknown:
            WARNv("header block element type unknown: "
              "0x%02X 0x%02X.",bp[0],bp[1])
            if(pbf_jump(&bp))
              END(-45)
            }  // end   first byte of element
          if(bp>zbufe)
            ENDE(-46,"header block format length.")
          }  // end   for every element in this loop
        if(!osmschema)
          ENDE(-47,"expected feature: OsmSchema-V0.6")
        if(!densenodes)
          ENDE(-48,"expected feature: DenseNodes")
        zbufp= bp;
        pb_bbvalid= bboxflags==0x0f;
        END(8)
        }  // header block
      // here: data block
      // provide primitive groups
      /* process data block */ {
        byte* bp;
        uint l;
        static byte* bstre;  // end of string table in zbuf[]

        bp= zbufp;
        pb__stre= pb__str;
        while(bp<zbufe) {  // for every element in this loop
          switch(bp[0]) {  // first byte of element
          case 0x0a:  // S 1, string table
            bp++;
            l= pbf_uint32(&bp);
            if(bp+l>zbufe)
              ENDEv(-101,"string table too large: %u",l)
            bstre= bp+l;
            while(bp<bstre) {  // for each element in string table
              if(bp[0]==0x0a) {  // S 1, string
                *bp++= 0;  // set null terminator for previous string
                l= pbf_uint32(&bp);
                if(bp+l>bstre)  // string too large
                  ENDEv(-102,"string too large: %u",l)
                if(pb__stre>=pb__stree)
                  ENDEv(-103,"too many strings: %i",pb__strM)
                *pb__stre++= (char*)bp;
                bp+= l;
                }  // end   S 1, string
              else {  // element type unknown
                byte* p;

                WARNv("string table element type unknown: "
                  "0x%02X 0x%02X.",bp[0],bp[1])
                p= bp;
                if(pbf_jump(&bp))
                  END(-104)
                *p= 0;  // set null terminator for previous string
                }  // end   element type unknown
              }  // end   for each element in string table
            pb__strm= pb__stre-pb__str;
            bp= bstre;
            break;
          case 0x12:  // S 2, primitive group
            *bp++= 0;  // set null terminator for previous string
            l= pbf_uint32(&bp);
            if(bp+l>zbufe)
              ENDEv(-111,"primitive group too large: %u",l)
            groupp= bp;
            groupe= bp+l;
            zbufp= groupe;
  /**/goto mainloop;  // we got a new primitive group
          case 0x88:  // 0x01 V 17, nanodegrees
            if(bp[1]!=0x01) goto d_unknown;
            bp+= 2;
            l= pbf_uint32(&bp);
            if(l!=global_pbfgranularity) {
              if(l>100)
                ENDEv(-120,"please specify: "
                  "--pbf-granularity=%u",l)
              else if(l==100)
                ENDE(-120,"please do not specify "
                  "--pbf-granularity")
              else
                ENDEv(-121,"granularity %u must be >=100.",l)
              }
            break;
          case 0x90:  // 0x01 V 18, millisec
            if(bp[1]!=0x01) goto d_unknown;
            bp+= 2;
            l= pbf_uint32(&bp);
            if(l!=1000)
              ENDEv(-122,"node milliseconds must be 1000: %u",l)
            break;
          case 0x98:  // 0x01 V 19, latitude offset
            if(bp[1]!=0x01) goto d_unknown;
            bp+= 2;
            if(pbf_sint64(&bp)!=0)
              ENDE(-123,"cannot process latitude offsets.")
            break;
          case 0xa0:  // 0x01 V 20, longitude offset
            if(bp[1]!=0x01) goto d_unknown;
            bp+= 2;
            if(pbf_sint64(&bp)!=0)
              ENDE(-124,"cannot process longitude offsets.")
            break;
          d_unknown:
          default:
            /* block */ {
              byte* p;
              WARNv("data block element type unknown: "
                "0x%02X 0x%02X.",bp[0],bp[1])
              p= bp;
              if(pbf_jump(&bp))
                END(-125)
              *p= 0;  // set null terminator for previous string
              }  // end   block
            }  // end   first byte of element
          if(bp>zbufe)
            ENDE(-129,"data block format length.")
          }  // end   for every element in this loop
        }  // process data block
      }  // zbuf data left
//// provide new zbuf data
    /* get new zbuf data */ {
      int datasize;  // -1: expected;
      int rawsize;  // -1: expected;
      int zdata;  // -1: expected;
        // 1: encountered section with compressed data
      uint l;
      byte* p;
      int r;

      // initialization
      blocktype= datasize= rawsize= zdata= -1;
      read_setjump();

      // care for new input data
      if(read_bufp>read_bufe)
        ENDE(-11,"main format length.")
      read_input();  // get at least maximum block size
      if(read_bufp>=read_bufe)  // at end of input file
        END(-1)
      if(read_bufp[0]!=0)  // unknown id at outermost level
        ENDEv(-12,"main-element type unknown: "
          "0x%02X 0x%02X.",read_bufp[0],read_bufp[1])
      if(read_bufp[1]!=0 || read_bufp[2]!=0 ||
          read_bufp[3]<11 || read_bufp[3]>17)
        ENDEv(-13,"format blob header %i.",
          read_bufp[1]*65536+read_bufp[2]*256+read_bufp[3])
      read_bufp+= 4;

      // read new block header
      for(;;) {  // read new block
        if(blocktype<0) {  // block type expected
          if(read_bufp[0]!=0x0a)  // not id S 1
            ENDEv(-21,"block type expected at: 0x%02X.",*read_bufp)
          read_bufp++;
          if(memcmp(read_bufp,"\x09OSMHeader",10)==0) {
            blocktype= 1;
            read_bufp+= 10;
      continue;
            }
          if(memcmp(read_bufp,"\x07OSMData",8)==0) {
            blocktype= 2;
            read_bufp+= 8;
      continue;
            }
          blocktype= 0;
          l= pbf_uint32(&read_bufp);
          if(read_bufp+l>read_bufe)  // string too long
            ENDEv(-22,"block type too long: %.40s",read_bufp)
          WARNv("block type unknown: %.40s",read_bufp)
          read_bufp+= l;
      continue;
          }  // end   block type expected
        if(datasize<0) {  // data size expected
          if(read_bufp[0]!=0x18)  // not id V 3
            ENDEv(-23,"block data size "
              "expected at: 0x%02X.",*read_bufp)
          read_bufp++;
          datasize= pbf_uint32(&read_bufp);
          }  // end   data size expected
        if(blocktype==0) {  // block type unknown
          read_bufp+= datasize;  // jump over this block
      continue;
          }  // end   block type unknown
        if(rawsize<0) {  // raw size expected
          if(read_bufp[0]!=0x10)  // not id V 2
            ENDEv(-24,"block raw size "
              "expected at: 0x%02X.",*read_bufp)
          p= read_bufp;
          read_bufp++;
          rawsize= pbf_uint32(&read_bufp);
          datasize-= read_bufp-p;
          }  // end   raw size expected
        if(zdata<0) {  // compressed data expected
          if(read_bufp[0]!=0x1a)  // not id S 3
            ENDEv(-25,"compressed data "
              "expected at: 0x%02X.",*read_bufp)
          p= read_bufp;
          read_bufp++;
          l= pbf_uint32(&read_bufp);
          datasize-= read_bufp-p;
          if(datasize<0 || datasize>pb__blockM ||
              read_bufp+datasize>read_bufe) {
            PERRv("block data size too large: %i",datasize)
            fprintf(stderr,"Pointers: %p %p %p\n",
              read__buf,read_bufp,read_bufe);
            END(-26)
            }
          if(l!=datasize)
            ENDEv(-31,"compressed length: %i->%u.",datasize,l)
          // uncompress
          r= pb__decompress(read_bufp,l,zbuf,sizeof(zbuf),&l);
          if(r!=0)
            ENDEv(-32,"decompression failed: %i.",r)
          if(l!=rawsize)
            ENDEv(-33,"uncompressed length: %i->%u.",rawsize,l)
          zdata= 1;
          zbufp= zbuf; zbufe= zbuf+rawsize;
          pb__stre= pb__str;
          read_bufp+= datasize;
      break;
          }  // end   compressed data expected
        if(read_bufp[0]==0)  // possibly a new block start
      break;
        }  // end   read new block
      if(zbufp<zbufe)  // zbuf data available
  continue;
      // here: still no osm objects to read in zbuf[]
      ENDE(-39,"missing data in pbf block.")
      }  // get new zbuf data
    }  // until we have a new object
  end:
  return pb_type;
  }  // end pb_input()

static int pb_keyval(char** keyp,char** valp,int keyvalmax) {
  // read tags of an osm .pbf object;
  // keyp,valp: start addresses of lists in which the tags
  //            will be stored in;
  // keyvalmax: maximum number of tags which can be stored in the list;
  // return: number of key/val tags which have been read;
  // this procedure should be called after OSM object data have
  // been provided by pb_input();
  // repetitive calls are not allowed because they would result
  // in wrong key/val data;
  int n;

  n= 0;
  if(pb_type==0) {  // node
    int key,val;

    if(pb__nodetags<pb__nodetagse &&
        (key= pbf_uint32(&pb__nodetags))!=0) {
        // there are key/val pairs for this node
      do {  // for every key/val pair
        val= pbf_uint32(&pb__nodetags);
        if(key>=pb__strm || val>=pb__strm) {
          PERRv("node key string index overflow: %u,%u>=%u",
            key,val,pb__strm)
return 0;
          }
        if(++n<=keyvalmax) {  // still space in output list
          *keyp++= pb__str[key];
          *valp++= pb__str[val];
          }  // still space in output list
        key= pbf_uint32(&pb__nodetags);
        } while(key!=0);  // end   for every key/val pair
      }  // end   there are key/val pairs for this node
    }  // node
  else if(pb_type==1) {  // way
    while(pb__waykey<pb__waykeye && pb__wayval<pb__wayvale) {
        // there are still key/val pairs for this way
      uint key,val;

      key= pbf_uint32(&pb__waykey);
      val= pbf_uint32(&pb__wayval);
      if(key>=pb__strm || val>=pb__strm) {
        PERRv("way key string index overflow: %u,%u>=%i",
          key,val,pb__strm)
return 0;
        }
      if(++n<=keyvalmax) {  // still space in output list
        *keyp++= pb__str[key];
        *valp++= pb__str[val];
        }  // still space in output list
      }  // end   there are still key/val pairs for this way
    }  // way
  else if(pb_type==2) {  // relation
    while(pb__relkey<pb__relkeye && pb__relval<pb__relvale) {
        // there are still refs for this relation
      uint key,val;

      key= pbf_uint32(&pb__relkey);
      val= pbf_uint32(&pb__relval);
      if(key>=pb__strm || val>=pb__strm) {
        PERRv("rel key string index overflow: %u,%u>=%i",
          key,val,pb__strm)
return 0;
        }
      if(++n<=keyvalmax) {  // still space in output list
        *keyp++= pb__str[key];
        *valp++= pb__str[val];
        }  // still space in output list
      }  // end   there are still refs for this relation
    }  // relation
  if(n>keyvalmax) {
    WARNv("too many key/val pairs for %s: %i>%i",
      ONAME(pb_id),n,keyvalmax)
    n= keyvalmax;
    }
  return n;
  }  // end   pb_keyval()

static int pb_noderef(int64_t* refidp,int refmax) {
  // read node references of an osm .pbf way object;
  // refidp: start addresses of lists in which the node reference's
  //         ids will be stored in;
  // refmax: maximum number of node references which can be stored
  //         in the lists;
  // return: number of node references which have been read;
  // this procedure should be called after OSM way data have
  // been provided by pb_input();
  // repetitive calls are not allowed because they would result
  // in wrong noderef data;
  int64_t noderef;
  int n;

  n= 0;
  noderef= 0;
  while(pb__waynode<pb__waynodee) {
        // there are still noderefs for this way
    noderef+= pbf_sint64(&pb__waynode);
    if(++n<=refmax) {  // still space in output list
      *refidp++= noderef;
      }  // still space in output list
    }  // there are still noderefs for this way
  if(n>refmax) {
    WARNv("too many noderefs for %s: %i>%i",ONAME(pb_id),n,refmax)
    n= refmax;
    }
  return n;
  }  // end   pb_noderef()

static int pb_ref(int64_t* refidp,
    byte* reftypep,char** refrolep,int refmax) {
  // read references of an osm .pbf object;
  // refidp: start addresses of lists in which the reference's
  //         ids will be stored in;
  // reftypep: same for their types;
  // refrolep: same for their roles;
  // refmax: maximum number of references which can be stored
  //         in the lists;
  // return: number of references which have been read;
  // this procedure should be called after OSM relation data have
  // been provided by pb_input();
  // repetitive calls are not allowed because they would result
  // in wrong ref data;
  int64_t refid;
  int n;

  n= 0;
  refid= 0;
  while(pb__relrefid<pb__relrefide && pb__relreftype<pb__relreftypee &&
      pb__relrefrole<pb__relrefrolee) {
      // there are still refs for this relation
    int reftype,refrole;

    refid+= pbf_sint64(&pb__relrefid);
    reftype= pbf_uint32(&pb__relreftype);
    refrole= pbf_uint32(&pb__relrefrole);
    if(refrole>=pb__strm) {
      PERRv("rel refrole string index overflow: %u>=%u",
        refrole,pb__strm)
return 0;
      }
    if(++n<=refmax) {  // still space in output list
      *refidp++= refid;
      *reftypep++= reftype;
      *refrolep++= pb__str[refrole];
      }  // still space in output list
    }  // end   there are still refs for this relation
  if(n>refmax) {
    WARNv("too many relrefs for %s: %i>%i",ONAME(pb_id),n,refmax)
    n= refmax;
    }
  return n;
  }  // end   pb_ref()

//------------------------------------------------------------
// end   Module pb_   pbf read module
//------------------------------------------------------------



//------------------------------------------------------------
// Module pstw_   pbf string write module
//------------------------------------------------------------

// this module provides procedures for collecting c-formatted
// strings while eliminating string doublets;
// this is needed to create Blobs for writing data in .pbf format;
// as usual, all identifiers of a module have the same prefix,
// in this case 'pstw'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

// string processing
// we need a string table to collect every string of a Blob;
// the data entities do not contain stings, they just refer to
// the strings in the string table; hence string doublets need
// not to be stored physically;

// how this is done
//
// there is a string memory; the pointer pstw__mem poits to the start
// of this memory area; into this area every string is written, each
// starting with 0x0a and the string length in pbf unsigned Varint
// format;
//
// there is a string table which contains pointers to the start of each
// string in the string memory area;
//
// there is a hash table which accelerates access to the string table;

#define kilobytes *1000  // unit "kilo"
#define Kibibytes *1024  // unit "Kibi"
#define Megabytes *1000000  // unit "Mega"
#define Mibibytes *1048576  // unit "Mibi"
#define pstw__memM (30 Megabytes)
  // maximum number of bytes in the string memory
#define pstw__tabM (1500000)
  // maximum number of strings in the table
#define pstw__hashtabM 25000009  // (preferably a prime number)
  // --> 150001, 1500007, 5000011, 10000019, 15000017,
  // 20000003, 25000009, 30000049, 40000003, 50000017
static char* pstw__mem= NULL;  // pointer to the string memory
static char* pstw__meme= NULL, *pstw__memee= NULL;  // pointers to
  // the logical end and to the physical end of string memory
typedef struct pstw__tab_struct {
  int index;  // index of this string table element;
  int len;  // length of the string contents
  char* mem0;  // pointer to the string's header in string memory area,
    // i.e., the byte 0x0a and the string's length in Varint format;
  char* mem;  // pointer to the string contents in string memory area
  int frequency;  // number of occurrences of this string
  int hash;
    // hash value of this element, used as a backlink to the hash table;
  struct pstw__tab_struct* next;
    // for chaining of string table rows which match
    // the same hash value; the last element will point to NULL;
  } pstw__tab_t;
static pstw__tab_t pstw__tab[pstw__tabM];  // string table
static int pstw__tabn= 0;  // number of entries in string table
static pstw__tab_t* pstw__hashtab[pstw__hashtabM];
  // hash table; elements point to matching strings in pstw__tab[];
  // []==NULL: no matching element to this certain hash value;

static inline uint32_t pstw__hash(const char* str,int* hash) {
  // get hash value of a string;
  // str[]: string from whose contents the hash is to be retrieved;
  // return: length of the string;
  // *hash: hash value in the range 0..(pstw__hashtabM-1);
  uint32_t c,h;
  const char* s;

  s= str;
  h= 0;
  for(;;) {
    if((c= *s++)==0) break; h+= c;
    if((c= *s++)==0) break; h+= c<<8;
    if((c= *s++)==0) break; h+= c<<16;
    if((c= *s++)==0) break; h+= c<<24;
    if((c= *s++)==0) break; h+= c<<4;
    if((c= *s++)==0) break; h+= c<<12;
    if((c= *s++)==0) break; h+= c<<20;
    }
  *hash= h % pstw__hashtabM;
  return (uint32_t)(s-str-1);
  }  // end   pstw__hash()

static inline pstw__tab_t* pstw__getref(
    pstw__tab_t* tabp,const char* s) {
  // get the string table reference of a string;
  // tabp: presumed index in string table (we got it from hash table);
  //       must be >=0 and <pstw__tabM, there is no boundary check;
  // s[]: string whose reference is to be determined;
  // return: pointer to string table entry;
  //         ==NULL: this string has not been stored yet
  const char* sp,*tp;
  int len;

  do {
    // compare the string with the tab entry
    tp= tabp->mem;
    len= tabp->len;
    sp= s;
    while(*sp!=0 && len>0 && *sp==*tp) { len--; sp++; tp++; }
    if(*sp==0 && len==0)  // string identical to string in table
  break;
    tabp= tabp->next;
    } while(tabp!=NULL);
  return tabp;
  }  // end   pstw__getref()

static void pstw__end() {
  // clean-up string processing;
  if(pstw__mem!=NULL) {
    free(pstw__mem);
    pstw__mem= pstw__meme= pstw__memee= NULL;
    }
  }  // end   pstw__end()

//------------------------------------------------------------

static int pstw_ini() {
  // initialize this module;
  // must be called before any other procedure is called;
  // return: 0: everything went ok;
  //         !=0: an error occurred;
  static bool firstrun= true;

  if(firstrun) {
    firstrun= false;
    pstw__mem= (char*)malloc(pstw__memM);
    if(pstw__mem==NULL)
return 1;
    atexit(pstw__end);
    pstw__memee= pstw__mem+pstw__memM;
    pstw__meme= pstw__mem;
    }
  return 0;
  }  // end   pstw_ini()

static inline void pstw_reset() {
  // clear string table and string hash table;
  // must be called before the first string is stored;
  memset(pstw__hashtab,0,sizeof(pstw__hashtab));
  pstw__meme= pstw__mem;

  // write string information of zero-string into string table
  pstw__tab->index= 0;
  pstw__tab->len= 0;
  pstw__tab->frequency= 0;
  pstw__tab->next= NULL;
  pstw__tab->hash= 0;

  // write zero-string into string information memory area
  pstw__tab->mem0= pstw__meme;
  *pstw__meme++= 0x0a;  // write string header into string memory
  *pstw__meme++= 0;  // write string length
  pstw__tab->mem= pstw__meme;

  pstw__tabn= 1;  // start with index 1
  }  // end   pstw_reset()

static inline int pstw_store(const char* s) {
  // store a string into string memory and return the string's index;
  // if an identical string has already been stored, omit writing,
  // just return the index of the stored string;
  // s[]: string to write;
  // return: index of the string in string memory;
  //         <0: string could not be written (e.g. not enough memory);
  uint32_t sl;  // length of the string
  int h;  // hash value
  pstw__tab_t* tabp;

  sl= pstw__hash(s,&h);
  tabp= pstw__hashtab[h];
  if(tabp!=NULL)  // string presumably stored already
    tabp= pstw__getref(tabp,s);  // get the right one
      // (if there are more than one with the same hash value)
  if(tabp!=NULL) {  // we found the right string in the table
    tabp->frequency++;  // mark that the string has (another) duplicate
return tabp->index;
    }
  // here: there is no matching string in the table

  // check for string table overflow
  if(pstw__tabn>=pstw__tabM) {  // no entry left in string table
    PERR("PBF write: string table overflow.")
return -1;
    }
  if(sl+10>(pstw__memee-pstw__meme)) {
      // not enough memory left in string memory area
    PERR("PBF write: string memory overflow.")
return -2;
    }

  // write string information into string table
  tabp= pstw__tab+pstw__tabn;
  tabp->index= pstw__tabn++;
  tabp->len= sl;
  tabp->frequency= 1;

  // update hash table references accordingly
  tabp->next= pstw__hashtab[h];
  pstw__hashtab[h]= tabp;  // link the new element to hash table
  tabp->hash= h;  // back-link to hash table element

  // write string into string information memory area
  tabp->mem0= pstw__meme;
  *pstw__meme++= 0x0a;  // write string header into string memory
  /* write the string length into string memory */ {
    uint32_t v,frac;

    v= sl;
    frac= v&0x7f;
    while(frac!=v) {
      *pstw__meme++= frac|0x80;
      v>>= 7;
      frac= v&0x7f;
      }
    *pstw__meme++= frac;
    }  // write the string length into string memory
  tabp->mem= pstw__meme;
  strcpy(pstw__meme,s);  // write string into string memory
  pstw__meme+= sl;
  return tabp->index;
  }  // end   pstw_store()

#if 1
static inline void pstw_write(byte** bufpp) {
  // write the string table in PBF format;
  // *bufpp: start address where to write the string table;
  // return:
  // *bufpp: address of the end of the written string table;
  size_t size;

  if(pstw__tabn==0)  // not a single string in memory
return;
  size= pstw__meme-pstw__mem;
  memcpy(*bufpp,pstw__mem,size);
  *bufpp+= size;
  }  // end   pstw_write()

#else
// remark:
// in the present program structure the bare sorting of the
// string table will lead to false output because string indexes
// in data fields are not updated accordingly;
// there would be an easy way to accomplish this for dense nodes,
// but I don't know if it's worth the effort in the first place;

static int pstw__qsort_write(const void* a,const void* b) {
  // string occurrences comparison for qsort() in pstw_write()
  int ax,bx;

  ax= ((pstw__tab_t**)a)->frequency;
  bx= ((pstw__tab_t**)b)->frequency;
  if(ax>bx)
return 1;
  if(ax==bx)
return 0;
  return -1;
  }  // end   pstw__qsort_write()

static inline int pstw_write(byte** bufpp) {
  // write the string table in PBF format;
  // *bufpp: start address where to write the string table;
  // return: number of bytes written;
  // *bufpp: address of the end of the written string table;
  // not used at present:
  // before the string table is written, it has to be ordered by
  // the number of occurrences of the strings; the most frequently
  // used strings must be written first;
  pstw__tab_t* tabp,*taborder[pstw__tabM],**taborderp;
  int i;
  byte* bufp;
  int l;

  if(pstw__tabn==0)  // not a single string in memory
return;

  // sort the string table, using an index list
  taborderp= taborder;
  tabp= pstw__tab;
  for(i= 0; i<pstw__tabn; i++)  // for every string in string table
    *taborderp++= tabp++;  // create an index list of the string table
  qsort(taborder,pstw__tabn,sizeof(taborder[0]),pstw__qsort_write);

  // write the string table, using the list of sorted indexes
  bufp= *bufpp;
  taborderp= taborder;
  for(i= 0; i<pstw__tabn; i++) {  // for every string in string table
    tabp= *taborder++;
    l= (int)(tabp->mem-tabp->mem0)+tabp->len;
    memcpy(bufp,tabp->mem0,l);
    bufp+= l;
    }  // for every string in string table
  l= bufp-*bufpp;
  *bufpp= bufp;
  return l;
  }  // end   pstw_write()
#endif

static inline int pstw_queryspace() {
  // query how much memory space is presently used by the strings;
  // this is useful before calling pstw_write();
  return (int)(pstw__meme-pstw__mem);
  }  // end   pstw_queryspace()

//------------------------------------------------------------
// end Module pstw_   pbf string write module
//------------------------------------------------------------



//------------------------------------------------------------
// Module pw_   PBF write module
//------------------------------------------------------------

// this module provides procedures which write .pbf objects;
// it uses procedures from module write_;
// as usual, all identifiers of a module have the same prefix,
// in this case 'pw'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static int pw__compress(byte* ibuf,uint isiz,byte* obuf,uint osizm,
  uint* osizp) {
  // compress a block of data;
  // return: 0: compression was successful;
  //         !=0: error number from zlib;
  // *osizp: size of compressed data;
  z_stream strm;
  int r,i;

  // initialization
  strm.zalloc= Z_NULL;
  strm.zfree= Z_NULL;
  strm.opaque= Z_NULL;
  strm.next_in= Z_NULL;
  strm.total_in= 0;
  strm.avail_out= 0;
  strm.next_out= Z_NULL;
  strm.total_out= 0;
  strm.msg= NULL;
  r= deflateInit(&strm,Z_DEFAULT_COMPRESSION);
  if(r!=Z_OK)
return r;

  // read data
  strm.next_in = ibuf;
  strm.avail_in= isiz;

  // compress
  strm.next_out= obuf;
  strm.avail_out= osizm;
  r= deflate(&strm,Z_FINISH);
  if(/*r!=Z_OK &&*/ r!=Z_STREAM_END) {
    deflateEnd(&strm);
    *osizp= 0;
    if(r==0) r= 1000;
return r;
    }

  // clean-up
  deflateEnd(&strm);
  obuf+= *osizp= osizm-(i= strm.avail_out);

  // add some zero bytes
  if(i>4) i= 4;
  while(--i>=0) *obuf++= 0;
  return 0;
  }  // end   pw__compress()

// format description: BlobHeader must be less than 64 kilobytes;
// uncompressed length of a Blob must be less than 32 megabytes;

#define pw__compress_bufM (UINT64_C(35) Megabytes)
static byte* pw__compress_buf= NULL;  // buffer for compressed objects

#define pw__bufM (UINT64_C(186) Megabytes)
static byte* pw__buf= NULL;  // buffer for objects in .pbf format
static byte* pw__bufe= NULL;  // logical end of the buffer
static byte* pw__bufee= NULL;  // physical end of the buffer

typedef struct pw__obj_struct {  // type of a pbf hierarchy object
  //struct pw__obj_struct parent;  // parent object; ==NULL: none;
  byte* buf;  // start address of pbf buffer for this hierarchy object;
    // this is where the header starts too;
  int headerlen;  // usually .bufl-.buf;
  byte* bufl;  // start address of object's length
  byte* bufc;  // start address of object's contents
  byte* bufe;  // write pointer in the pbf buffer
  byte* bufee;  // end address of pbf buffer for this hierarchy object
  } pw__obj_t;

#define pw__objM 20
static pw__obj_t pw__obj[pw__objM];
static pw__obj_t* pw__obje= pw__obj;
  // logical end of the object hierarchy array
static pw__obj_t *pw__objee= pw__obj+pw__objM;
  // physical end of the object hierarchy array
static pw__obj_t* pw__objp= NULL;  // currently active hierarchy object

static inline pw__obj_t* pw__obj_open(const char* header) {
  // open a new hierarchy level
  // header[20]: header which is to be written prior to the
  //           contents length; zero-terminated;
  pw__obj_t* op;

  if(pw__obje==pw__obj) {  // first hierarchy object
    pw__bufe= pw__buf;
    //pw__obje->parent= NULL;
    pw__obje->buf= pw__bufe;
    }
  else {  // not the first hierarchy object
    if(pw__obje>=pw__objee) {  // no space left in hierarchy array
      PERR("PBF write: hierarchy overflow.")
return pw__objp;
      }
    op= pw__obje-1;
    if(op->bufee==pw__bufee) {  // object is not a limited one
      pw__obje->buf= op->bufe;
      }
    else  // object is a limited one
      pw__obje->buf= op->bufee;
    if(pw__obje->buf+50>pw__bufee) {  // no space left PBF object buffer
      PERR("PBF write: object buffer overflow.")
return pw__objp;
      }
    }  // not the first hierarchy object
  pw__objp= pw__obje++;
  // write PBF object's header and pointers
  pw__objp->bufl= (byte*)stpmcpy((char*)pw__objp->buf,header,20);
  pw__objp->headerlen= (int)(pw__objp->bufl-pw__objp->buf);
  pw__objp->bufc= pw__objp->bufl+10;
  pw__objp->bufe= pw__objp->bufc;
  pw__objp->bufee= pw__bufee;
  return pw__objp;
  }  // pw__obj_open()

static inline void pw__obj_limit(int size) {
  // limit the maximum size of an PBF hierarchy object;
  // this is necessary if two or more PBF objects shall be written
  // simultaneously, e.g. when writing dense nodes;

  if(size>pw__objp->bufee-pw__objp->bufc-50) {
    PERRv("PBF write: object buffer limit too large: %i>%i.",
      size,(int)(pw__objp->bufee-pw__objp->bufc-50))
return;
    }
  pw__objp->bufee= pw__objp->bufc+size;
  }  // pw__obj_limit()

static inline void pw__obj_limit_parent(pw__obj_t* parent) {
  // limit the size of a PBF hierarchy parent object to the
  // sum of the maximum sizes of its children;
  // parent: must point to the parent object;
  // pw__objp: must point to the last child of the parent;
  parent->bufee= pw__objp->bufee;
  }  // pw__obj_limit_parent()

static inline void pw__obj_compress() {
  // compress the contents of the current PBF hierarchy object;
  // pw__objp: pointer to current object;
  int r;
  unsigned int osiz;  // size of the compressed contents

  r= pw__compress(pw__objp->bufc,pw__objp->bufe-pw__objp->bufc,
    pw__compress_buf,pw__compress_bufM,&osiz);
  if(r!=0) {  // an error has occurred
    PERRv("PBF write: compression error %i.",r)
return;
    }
  if(osiz>pw__objp->bufee-pw__objp->bufc) {
    PERRv("PBF write: compressed contents too large: %i>%i.",
      osiz,(int)(pw__objp->bufee-pw__objp->bufc))
return;
    }
  memcpy(pw__objp->bufc,pw__compress_buf,osiz);
  pw__objp->bufe= pw__objp->bufc+osiz;
  }  // pw__obj_compress()

static inline void pw__obj_add_id(uint8_t pbfid) {
  // append a one-byte PBF id to PBF write buffer;
  // pbfid: PBF id;
  // pw__objp->bufe: write buffer position (will be
  //                 incremented by this procedure);
  if(pw__objp->bufe>=pw__objp->bufee) {
    PERR("PBF write: id memory overflow.")
return;
    }
  *pw__objp->bufe++= pbfid;
  }  // pw__obj_add_id()

static inline void pw__obj_add_id2(uint16_t pbfid) {
  // append a two-byte PBF id to PBF write buffer;
  // pbfid: PBF id, high byte is stored first;
  // pw__objp->bufe: write buffer position (will be
  //                 incremented by 2 by this procedure);
  if(pw__objp->bufe+2>pw__objp->bufee) {
    PERR("PBF write: id2 memory overflow.")
return;
    }
  *pw__objp->bufe++= (byte)(pbfid>>8);
  *pw__objp->bufe++= (byte)(pbfid&0xff);
  }  // pw__obj_add_id2()

static inline void pw__obj_add_uint32(uint32_t v) {
  // append a numeric value to PBF write buffer;
  // pw__objp->bufe: write buffer position
  //                 (will be updated by this procedure);
  uint32_t frac;

  if(pw__objp->bufe+10>pw__objp->bufee) {
    PERR("PBF write: uint32 memory overflow.")
return;
    }
  frac= v&0x7f;
  while(frac!=v) {
    *pw__objp->bufe++= frac|0x80;
    v>>= 7;
    frac= v&0x7f;
    }
  *pw__objp->bufe++= frac;
  }  // pw__obj_add_uint32()

static inline void pw__obj_add_sint32(int32_t v) {
  // append a numeric value to PBF write buffer;
  // pw__objp->bufe: write buffer position
  //                 (will be updated by this procedure);
  uint32_t u;
  uint32_t frac;

  if(pw__objp->bufe+10>pw__objp->bufee) {
    PERR("PBF write: sint32 memory overflow.")
return;
    }
  if(v<0) {
    u= -v;
    u= (u<<1)-1;
    }
  else
    u= v<<1;
  frac= u&0x7f;
  while(frac!=u) {
    *pw__objp->bufe++= frac|0x80;
    u>>= 7;
    frac= u&0x7f;
    }
  *pw__objp->bufe++= frac;
  }  // pw__obj_add_sint32()

static inline void pw__obj_add_uint64(uint64_t v) {
  // append a numeric value to PBF write buffer;
  // pw__objp->bufe: write buffer position
  //                 (will be updated by this procedure);
  uint32_t frac;

  if(pw__objp->bufe+10>pw__objp->bufee) {
    PERR("PBF write: uint64 memory overflow.")
return;
    }
  frac= v&0x7f;
  while(frac!=v) {
    *pw__objp->bufe++= frac|0x80;
    v>>= 7;
    frac= v&0x7f;
    }
  *pw__objp->bufe++= frac;
  }  // pw__obj_add_uint64()

static inline void pw__obj_add_sint64(int64_t v) {
  // append a numeric value to PBF write buffer;
  // pw__objp->bufe: write buffer position
  //                 (will be updated by this procedure);
  uint64_t u;
  uint32_t frac;

  if(pw__objp->bufe+10>pw__objp->bufee) {
    PERR("PBF write: sint64 memory overflow.")
return;
    }
  if(v<0) {
    u= -v;
    u= (u<<1)-1;
    }
  else
    u= v<<1;
  frac= u&0x7f;
  while(frac!=u) {
    *pw__objp->bufe++= frac|0x80;
    u>>= 7;
    frac= u&0x7f;
    }
  *pw__objp->bufe++= frac;
  }  // pw__obj_add_sint64()

#if 0  // not used at present
static inline void pw__obj_add_mem(byte* s,uint32_t sl) {
  // append data to PBF write buffer;
  // s[]: data which are to append;
  // ls: length of the data;
  // pw__objp->bufe: write buffer position
  //                 (will be updated by this procedure);

  if(pw__objp->bufe+sl>pw__objp->bufee) {
    PERR("PBF write: mem memory overflow.")
return;
    }
  memcpy(pw__objp->bufe,s,sl);
  pw__objp->bufe+= sl;
  }  // pw__obj_add_mem()
#endif

static inline void pw__obj_add_str(const char* s) {
  // append a PBF string to PBF write buffer;
  // pw__objp->bufe: write buffer position
  //                 (will be updated by this procedure);
  uint32_t sl;  // length of the string

  sl= strlen(s);
  if(pw__objp->bufe+10+sl>pw__objp->bufee) {
    PERR("PBF write: string memory overflow.")
return;
    }
  /* write the string length into PBF write buffer */ {
    uint32_t v,frac;

    v= sl;
    frac= v&0x7f;
    while(frac!=v) {
      *pw__objp->bufe++= frac|0x80;
      v>>= 7;
      frac= v&0x7f;
      }
    *pw__objp->bufe++= frac;
    }  // write the string length into PBF write buffer
  memcpy(pw__objp->bufe,s,sl);
  pw__objp->bufe+= sl;
  }  // pw__obj_add_str()

static inline void pw__obj_close() {
  // close an object which had been opened with pw__obj_open();
  // pw__objp: pointer to the object which is to close;
  // return:
  // pw__objp: points to the last opened object;
  pw__obj_t* op;
  int i;
  byte* bp;
  uint32_t len;
  uint32_t v,frac;

  if(pw__objp==pw__obj) {  // this is the anchor object
    // write the object's data to standard output
    write_mem(pw__objp->buf,pw__objp->headerlen);  // write header
    write_mem(pw__objp->bufc,(int)(pw__objp->bufe-pw__objp->bufc));
      // write contents
    // delete hierarchy object
    pw__objp= NULL;
    pw__obje= pw__obj;
return;
    }

  // determine the parent object
  op= pw__objp;
  for(;;) {  // search for the parent object
    if(op<=pw__obj) {  // there is no parent object
      PERR("PBF write: no parent object.")
return;
      }
    op--;
    if(op->buf!=NULL)  // found our parent object
  break;
    }

  // write PBF object's header into parent object
  bp= pw__objp->buf;
  i= pw__objp->headerlen;
  while(--i>=0)
    *op->bufe++= *bp++;

  // write PBF object's length into parent object
  len= v= pw__objp->bufe-pw__objp->bufc;
  frac= v&0x7f;
  while(frac!=v) {
    *op->bufe++= frac|0x80;
    v>>= 7;
    frac= v&0x7f;
    }
  *op->bufe++= frac;

  // write PBF object's contents into parent object
  memmove(op->bufe,pw__objp->bufc,len);
  op->bufe+= len;

  // mark this object as deleted
  pw__objp->buf= NULL;

  // free the unused space in object hierarchy array
  while(pw__obje>pw__obj && pw__obje[-1].buf==NULL) pw__obje--;
  pw__objp= pw__obje-1;
  }  // pw__obj_close()

static inline void pw__obj_dispose() {
  // dispose an object which had been opened with pw__obj_open();
  // pw__objp: pointer to the object which is to close;
  // return:
  // pw__objp: points to the last opened object;
  if(pw__objp==pw__obj) {  // this is the anchor object
    // delete hierarchy object
    pw__objp= NULL;
    pw__obje= pw__obj;
return;
    }

  // mark this object as deleted
  pw__objp->buf= NULL;

  // free the unused space in object hierarchy array
  while(pw__obje>pw__obj && pw__obje[-1].buf==NULL) pw__obje--;
  pw__objp= pw__obje-1;
  }  // pw__obj_dispose()

static pw__obj_t* pw__st= NULL,*pw__dn_id= NULL,*pw__dn_his,
  *pw__dn_hisver= NULL,*pw__dn_histime= NULL,*pw__dn_hiscset= NULL,
  *pw__dn_hisuid= NULL,*pw__dn_hisuser= NULL,
  *pw__dn_lat= NULL,*pw__dn_lon= NULL,*pw__dn_keysvals= NULL;

// some variables for delta coding
static int64_t pw__dc_id= 0;
static int32_t pw__dc_lon= 0,pw__dc_lat= 0;
static int64_t pw__dc_histime= 0;
static int64_t pw__dc_hiscset= 0;
static uint32_t pw__dc_hisuid= 0;
static uint32_t pw__dc_hisuser= 0;
static int64_t pw__dc_noderef= 0;
static int64_t pw__dc_ref= 0;

static void pw__data(int otype) {
  // prepare or complete an 'OSMData fileblock';
  // should be called prior to writing each OSM object;
  // otype: type of the OSM object which is going to be written;
  //        0: node; 1: way; 2: relation; -1: none;
  static int otype_old= -1;
  static const int max_object_size= (250 kilobytes);
    // assumed maximum size of one OSM object
  #define pw__data_spaceM (31 Megabytes)
    // maximum size of one 'fileblock'
  static int used_space= pw__data_spaceM;
    // presently used memory space in present 'OSMData fileblock',
    // not including the strings
  int string_space;  // memory space used by strings
  int remaining_space;
    // remaining memory space in present 'OSMData fileblock'
  int i;

  // determine remaining space in current 'OSMData fileblock';
  // the remaining space is usually guessed in a pessimistic manner;
  // if this estimate shows too less space, then a more exact
  // calculation is made;
  // this strategy has been chosen for performance reasons;
  used_space+= 64000;  // increase used-space variable by the assumed
    // maximum size of one OSM object, not including the strings
  string_space= pstw_queryspace();
  remaining_space= pw__data_spaceM-used_space-string_space;
  if(remaining_space<max_object_size) {  // might be too less space
    // calculate used space more exact
    if(otype_old==0) {  // node
      used_space= (int)((pw__dn_id->bufe-pw__dn_id->buf)+
        (pw__dn_lat->bufe-pw__dn_lat->buf)+
        (pw__dn_lon->bufe-pw__dn_lon->buf)+
        (pw__dn_keysvals->bufe-pw__dn_keysvals->buf));
      if(!global_dropversion) {
        used_space+= (int)(pw__dn_hisver->bufe-pw__dn_hisver->buf);
        if(!global_dropauthor) {
          used_space+= (int)((pw__dn_histime->bufe-pw__dn_histime->buf)+
            (pw__dn_hiscset->bufe-pw__dn_hiscset->buf)+
            (pw__dn_hisuid->bufe-pw__dn_hisuid->buf)+
            (pw__dn_hisuser->bufe-pw__dn_hisuser->buf));
          }
        }
      }
    else if(otype_old>0)  // way or relation
      used_space= (int)(pw__objp->bufe-pw__objp->buf);
    remaining_space= pw__data_spaceM-used_space-string_space;
    }  // might be too less space

  // conclude or start an 'OSMData fileblock'
  if(otype!=otype_old || remaining_space<max_object_size) {
      // 'OSMData fileblock' must be concluded or started
    if(otype_old>=0) {  // there has been object processing
      // complete current 'OSMData fileblock'
      used_space= pw__data_spaceM;  // force new calculation next time
      i= pstw_queryspace();
      if(i>pw__st->bufee-pw__st->bufe)
        PERR("PBF write: string table memory overflow.")
      else
        pstw_write(&pw__st->bufe);
      pw__objp= pw__st; pw__obj_close();  // 'stringtable'
      switch(otype_old) {  // select by OSM object type
      case 0:  // node
        pw__objp= pw__dn_id; pw__obj_close();
        if(!global_dropversion) {  // version number is to be written
          pw__objp= pw__dn_hisver; pw__obj_close();
          if(!global_dropauthor) {  // author information  is to be written
            pw__objp= pw__dn_histime; pw__obj_close();
            pw__objp= pw__dn_hiscset; pw__obj_close();
            pw__objp= pw__dn_hisuid; pw__obj_close();
            pw__objp= pw__dn_hisuser; pw__obj_close();
            }  // author information  is to be written
          pw__objp= pw__dn_his; pw__obj_close();
          }  // version number is to be written
        pw__objp= pw__dn_lat; pw__obj_close();
        pw__objp= pw__dn_lon; pw__obj_close();
        pw__objp= pw__dn_keysvals; pw__obj_close();
        pw__obj_close();  // 'dense'
        break;
      case 1:  // way
        break;
      case 2:  // relation
        break;
        }  // select by OSM object type
      pw__obj_close();  // 'primitivegroup'
      /* write 'raw_size' into hierarchy object's header */ {
        uint32_t v,frac;
        byte* bp;

        v= pw__objp->bufe-pw__objp->bufc;
        bp= pw__objp->buf+1;
        frac= v&0x7f;
        while(frac!=v) {
          *bp++= frac|0x80;
          v>>= 7;
          frac= v&0x7f;
          }
        *bp++= frac;
        *bp++= 0x1a;
        pw__objp->headerlen= bp-pw__objp->buf;
        }
      pw__obj_compress();
      pw__obj_close();  // 'zlib_data'
      pw__obj_close();  // 'datasize'
      /* write 'length of BlobHeader message' into object's header */ {
        byte* bp;

        bp= pw__objp->bufc+pw__objp->bufc[1]+3;
        while((*bp & 0x80)!=0) bp++;
        bp++;
        pw__objp->buf[0]= pw__objp->buf[1]= pw__objp->buf[2]= 0;
        pw__objp->buf[3]= bp-pw__objp->bufc;
        }
      pw__obj_close();  // 'Blobheader'
      otype_old= -1;
      }  // there has been object processing

    // prepare new 'OSMData fileblock' if necessary
    if(otype!=otype_old) {
      pw__obj_open("----");
        // open anchor hierarchy object for 'OSMData fileblock'
        // (every fileblock starts with four zero-bytes;
        // the fourth zero-byte will be overwritten later
        // by the length of the BlobHeader;)
      pw__obj_add_id(0x0a);  // S 1 'type'
      pw__obj_add_str("OSMData");
      pw__obj_open("\x18");  // V 3 'datasize'
      pw__obj_open("\x10----------\x1a");  // S 3 'zlib_data'
        // in the header: V 2 'raw_size'
      pw__st= pw__obj_open("\x0a");  // S 1 'stringtable'
      pw__obj_limit(30 Megabytes);
      pstw_reset();
      pw__obj_open("\x12");  // S 2 'primitivegroup'
      switch(otype) {  // select by OSM object type
      case 0:  // node
        pw__obj_open("\x12");  // S 2 'dense'
        pw__dn_id= pw__obj_open("\x0a");  // S 1 'id'
        pw__obj_limit(10 Megabytes);
        if(!global_dropversion) {  // version number is to be written
          pw__dn_his= pw__obj_open("\x2a");  // S 5 'his'
          pw__dn_hisver= pw__obj_open("\x0a");  // S 1 'his.ver'
          pw__obj_limit(10 Megabytes);
          if(!global_dropauthor) {  // author information  is to be written
            pw__dn_histime= pw__obj_open("\x12");  // S 2 'his.time'
            pw__obj_limit(10 Megabytes);
            pw__dn_hiscset= pw__obj_open("\x1a");  // S 3 'his.cset'
            pw__obj_limit(10 Megabytes);
            pw__dn_hisuid= pw__obj_open("\x22");  // S 4 'his.uid'
            pw__obj_limit(8 Megabytes);
            pw__dn_hisuser= pw__obj_open("\x2a");  // S 5 'his.user'
            pw__obj_limit(6 Megabytes);
            }  // author information  is to be written
          pw__obj_limit_parent(pw__dn_his);
          }  // version number is to be written
        pw__dn_lat= pw__obj_open("\x42");  // S 8 'lat'
        pw__obj_limit(30 Megabytes);
        pw__dn_lon= pw__obj_open("\x4a");  // S 9 'lon'
        pw__obj_limit(30 Megabytes);
        pw__dn_keysvals= pw__obj_open("\x52");  // S 10 'tags'
        pw__obj_limit(40 Megabytes);
        // reset variables for delta coding
        pw__dc_id= 0;
        pw__dc_lat= pw__dc_lon= 0;
        pw__dc_histime= 0;
        pw__dc_hiscset= 0;
        pw__dc_hisuid= 0;
        pw__dc_hisuser= 0;
        break;
      case 1:  // way
        break;
      case 2:  // relation
        break;
        }  // select by OSM object type
      otype_old= otype;
      }  // prepare new 'OSMData fileblock' if necessary
    }  // 'OSMData fileblock' must be concluded or started
  }  // pw__data()

static void pw__end() {
  // clean-up this module;
  if(pw__obje!=pw__obj)
    PERR("PBF write: object hierarchy still open.")
  if(pw__buf!=NULL) {
    free(pw__buf);
    pw__buf= pw__bufe= pw__bufee= NULL;
    }
  pw__obje= pw__obj;
  pw__objp= NULL;
  if(pw__compress_buf!=NULL) {
    free(pw__compress_buf);
    pw__compress_buf= NULL;
    }
  }  // end   pw__end()

//------------------------------------------------------------

static inline int pw_ini() {
  // initialize this module;
  // must be called before any other procedure is called;
  // return: 0: everything went ok;
  //         !=0: an error occurred;
  static bool firstrun= true;
  int r;

  if(firstrun) {
    firstrun= false;
    atexit(pw__end);
    pw__buf= (byte*)malloc(pw__bufM);
    pw__bufe= pw__buf;
    pw__bufee= pw__buf+pw__bufM;
    pw__compress_buf= (byte*)malloc(pw__compress_bufM);
    r= pstw_ini();
    if(pw__buf==NULL || pw__compress_buf==NULL || r!=0) {
      PERR("PBF write: not enough memory.")
return 1;
      }
    }
  return 0;
  }  // end   pw_ini()

static void pw_header(bool bboxvalid,
    int32_t x1,int32_t y1,int32_t x2,int32_t y2,int64_t timestamp) {
  // start writing PBF objects, i.e., write the 'OSMHeader fileblock';
  // bboxvalid: the following bbox coordinates are valid;
  // x1,y1,x2,y2: bbox coordinates (base 10^-7);
  // timestamp: file timestamp; ==0: no timestamp given;
  pw__obj_open("----");
    // open anchor hierarchy object for 'OSMHeader fileblock'
    // (every fileblock starts with four zero-bytes;
    // the fourth zero-byte will be overwritten later
    // by the length of the BlobHeader;)
  pw__obj_add_id(0x0a);  // S 1 'type'
  pw__obj_add_str("OSMHeader");
  pw__obj_open("\x18");  // V 3 'datasize'
  pw__obj_open("\x10----------\x1a");  // S 3 'zlib_data'
    // in the header: V 2 'raw_size'
  if(bboxvalid) {
    pw__obj_open("\x0a");  // S 1 'bbox'
    pw__obj_add_id(0x08);  // V 1 'minlon'
    pw__obj_add_sint64((int64_t)x1*100);
    pw__obj_add_id(0x10);  // V 2 'maxlon'
    pw__obj_add_sint64((int64_t)x2*100);
    pw__obj_add_id(0x18);  // V 3 'maxlat'
    pw__obj_add_sint64((int64_t)y2*100);
    pw__obj_add_id(0x20);  // V 4 'minlat'
    pw__obj_add_sint64((int64_t)y1*100);
    pw__obj_close();
    }
  pw__obj_add_id(0x22);  // S 4 'required_features'
  pw__obj_add_str("OsmSchema-V0.6");
  pw__obj_add_id(0x22);  // S 4 'required_features'
  pw__obj_add_str("DenseNodes");
  pw__obj_add_id(0x2a);  // S 5 'optional_features'
  pw__obj_add_str("Sort.Type_then_ID");
  if(timestamp!=0) {  // file timestamp given
    char s[40],*sp;

    sp= stpcpy0(s,"timestamp=");
    write_createtimestamp(timestamp,sp);
    pw__obj_add_id(0x2a);  // S 5 'optional_features'
    pw__obj_add_str(s);
    }  // file timestamp given
  pw__obj_add_id2(0x8201);  // S 16 'writingprogram'
  pw__obj_add_str("osmconvert "VERSION);
  pw__obj_add_id2(0x8a01);  // S 17 'source'
  pw__obj_add_str("http://www.openstreetmap.org/api/0.6");
  if(timestamp!=0) {  // file timestamp given
    pw__obj_add_id2(0x8002);  // V 32 osmosis_replication_timestamp
    pw__obj_add_uint64(timestamp);
    }  // file timestamp given
  /* write 'raw_size' into hierarchy object's header */ {
    uint32_t v,frac;
    byte* bp;

    v= pw__objp->bufe-pw__objp->bufc;
    bp= pw__objp->buf+1;
    frac= v&0x7f;
    while(frac!=v) {
      *bp++= frac|0x80;
      v>>= 7;
      frac= v&0x7f;
      }
    *bp++= frac;
    *bp++= 0x1a;
    pw__objp->headerlen= bp-pw__objp->buf;
    }
  pw__obj_compress();
  pw__obj_close();  // 'zlib_data'
  pw__obj_close();  // 'datasize'
  /* write 'length of BlobHeader message' into object's header */ {
    byte* bp;

    bp= pw__objp->bufc+pw__objp->bufc[1]+3;
    while((*bp & 0x80)!=0) bp++;
    bp++;
    pw__objp->buf[0]= pw__objp->buf[1]= pw__objp->buf[2]= 0;
    pw__objp->buf[3]= bp-pw__objp->bufc;
    }
  pw__obj_close();  // 'Blobheader'
  }  // end   pw_header()

static inline void pw_foot() {
  // end writing a PBF file;
  pw__data(-1);
  }  // end   pw_foot()

static inline void pw_flush() {
  // end writing a PBF dataset;
  pw__data(-1);
  }  // end   pw_flush()

static inline void pw_node(int64_t id,
    int32_t hisver,int64_t histime,int64_t hiscset,
    uint32_t hisuid,const char* hisuser,int32_t lon,int32_t lat) {
  // start writing a PBF dense node dataset;
  // id: id of this object;
  // hisver: version; 0: no author information is to be written
  // histime: time (seconds since 1970)
  // hiscset: changeset
  // hisuid: uid
  // hisuser: user name
  // lon: latitude in 100 nanodegree;
  // lat: latitude in 100 nanodegree;
  int stid;  // string id

  pw__data(0);
  pw__objp= pw__dn_id; pw__obj_add_sint64(id-pw__dc_id);
  pw__dc_id= id;
  if(!global_dropversion) {  // version number is to be written
    if(hisver==0) hisver= 1;
    pw__objp= pw__dn_hisver; pw__obj_add_uint32(hisver);
    if(!global_dropauthor) {  // author information is to be written
      if(histime==0) { histime= 1; hiscset= 1; hisuser= 0; }
      pw__objp= pw__dn_histime;
      pw__obj_add_sint64(histime-pw__dc_histime);
      pw__dc_histime= histime;
      pw__objp= pw__dn_hiscset;
      pw__obj_add_sint64(hiscset-pw__dc_hiscset);
      pw__dc_hiscset= hiscset;
      pw__objp= pw__dn_hisuid;
      pw__obj_add_sint32(hisuid-pw__dc_hisuid);
      pw__dc_hisuid= hisuid;
      pw__objp= pw__dn_hisuser;
      if(hisuid==0) hisuser= "";
      stid= pstw_store(hisuser);
      pw__obj_add_sint32(stid-pw__dc_hisuser);
      pw__dc_hisuser= stid;
      }  // author information  is to be written
    }  // version number is to be written
  pw__objp= pw__dn_lat; pw__obj_add_sint64(lat-pw__dc_lat);
  pw__dc_lat= lat;
  pw__objp= pw__dn_lon;
    pw__obj_add_sint64((int64_t)lon-pw__dc_lon);
  pw__dc_lon= lon;
  }  // end   pw_node()

static inline void pw_node_keyval(const char* key,const char* val) {
  // write node object's keyval;
  int stid;  // string id

  pw__objp= pw__dn_keysvals;
  stid= pstw_store(key);
  pw__obj_add_uint32(stid);
  stid= pstw_store(val);
  pw__obj_add_uint32(stid);
  }  // end   pw_node_keyval()

static inline void pw_node_close() {
  // close writing node object;
  pw__objp= pw__dn_keysvals;
  pw__obj_add_uint32(0);
  }  // end   pw_node_close()

static pw__obj_t* pw__wayrel_keys= NULL,*pw__wayrel_vals= NULL,
  *pw__wayrel_his= NULL,*pw__way_noderefs= NULL,
  *pw__rel_roles= NULL,*pw__rel_refids= NULL,*pw__rel_types= NULL;

static inline void pw_way(int64_t id,
    int32_t hisver,int64_t histime,int64_t hiscset,
    uint32_t hisuid,const char* hisuser) {
  // start writing a PBF way dataset;
  // id: id of this object;
  // hisver: version; 0: no author information is to be written;
  // histime: time (seconds since 1970)
  // hiscset: changeset
  // hisuid: uid
  // hisuser: user name;
  int stid;  // string id

  pw__data(1);
  pw__obj_open("\x1a");  // S 3 'ways'
  pw__obj_add_id(0x08);  // V 1 'id'
  pw__obj_add_uint64(id);
  pw__wayrel_keys= pw__obj_open("\x12");  // S 2 'keys'
  pw__obj_limit(20 Megabytes);
  pw__wayrel_vals= pw__obj_open("\x1a");  // S 3 'vals'
  pw__obj_limit(20 Megabytes);
  pw__wayrel_his= pw__obj_open("\x22");  // S 4 'his'
  pw__obj_limit(2000);
  pw__way_noderefs= pw__obj_open("\x42");  // S 8 'noderefs'
  pw__obj_limit(30 Megabytes);
  if(!global_dropversion) {  // version number is to be written
    pw__objp= pw__wayrel_his;
    if(hisver==0) hisver= 1;
    pw__obj_add_id(0x08);  // V 1 'hisver'
    pw__obj_add_uint32(hisver);
    if(!global_dropauthor) {  // author information  is to be written
      if(histime==0) {
        histime= 1; hiscset= 1; hisuser= 0; }
      pw__obj_add_id(0x10);  // V 2 'histime'
      pw__obj_add_uint64(histime);
      pw__obj_add_id(0x18);  // V 3 'hiscset'
      pw__obj_add_uint64(hiscset);
      pw__obj_add_id(0x20);  // V 4 'hisuid'
      pw__obj_add_uint32(hisuid);
      pw__obj_add_id(0x28);  // V 5 'hisuser'
      if(hisuid==0) hisuser= "";
      stid= pstw_store(hisuser);
      pw__obj_add_uint32(stid);
      }  // author information  is to be written
    }  // version number is to be written
  pw__dc_noderef= 0;
  }  // end   pw_way()

static inline void pw_wayrel_keyval(const char* key,const char* val) {
  // write a ways or a relations object's keyval;
  int stid;  // string id

  pw__objp= pw__wayrel_keys;
  stid= pstw_store(key);
  pw__obj_add_uint32(stid);
  pw__objp= pw__wayrel_vals;
  stid= pstw_store(val);
  pw__obj_add_uint32(stid);
  }  // end   pw_wayrel_keyval()

static inline void pw_way_ref(int64_t noderef) {
  // write a ways object's noderefs;
  pw__objp= pw__way_noderefs;
  pw__obj_add_sint64(noderef-pw__dc_noderef);
  pw__dc_noderef= noderef;
  }  // end   pw_way_ref()

static inline void pw_way_close() {
  // close writing way object;
  pw__objp= pw__wayrel_keys;
  if(pw__objp->bufe==pw__objp->bufc)  // object is empty
    pw__obj_dispose();
  else
    pw__obj_close();
  pw__objp= pw__wayrel_vals;
  if(pw__objp->bufe==pw__objp->bufc)  // object is empty
    pw__obj_dispose();
  else
    pw__obj_close();
  pw__objp= pw__wayrel_his;
  if(pw__objp->bufe==pw__objp->bufc)  // object is empty
    pw__obj_dispose();
  else
    pw__obj_close();
  pw__objp= pw__way_noderefs;
  if(pw__objp->bufe==pw__objp->bufc)  // object is empty
    pw__obj_dispose();
  else
    pw__obj_close();
  pw__obj_close();
  }  // end   pw_way_close()

static inline void pw_relation(int64_t id,
    int32_t hisver,int64_t histime,int64_t hiscset,
    uint32_t hisuid,const char* hisuser) {
  // start writing a PBF way dataset;
  // id: id of this object;
  // hisver: version; 0: no author information is to be written;
  // histime: time (seconds since 1970)
  // hiscset: changeset
  // hisuid: uid
  // hisuser: user name;
  int stid;  // string id

  pw__data(2);
  pw__obj_open("\x22");  // S 4 'relations'
  pw__obj_add_id(0x08);  // V 1 'id'
  pw__obj_add_uint64(id);
  pw__wayrel_keys= pw__obj_open("\x12");  // S 2 'keys'
  pw__obj_limit(20 Megabytes);
  pw__wayrel_vals= pw__obj_open("\x1a");  // S 3 'vals'
  pw__obj_limit(20 Megabytes);
  pw__wayrel_his= pw__obj_open("\x22");  // S 4 'his'
  pw__obj_limit(2000);
  pw__rel_roles= pw__obj_open("\x42");  // S 8 'role'
  pw__obj_limit(20 Megabytes);
  pw__rel_refids= pw__obj_open("\x4a");  // S 9 'refid'
  pw__obj_limit(20 Megabytes);
  pw__rel_types= pw__obj_open("\x52");  // S 10 'type'
  pw__obj_limit(20 Megabytes);
  if(!global_dropversion) {  // version number is to be written
    pw__objp= pw__wayrel_his;
    if(hisver==0) hisver= 1;
    pw__obj_add_id(0x08);  // V 1 'hisver'
    pw__obj_add_uint32(hisver);
    if(!global_dropauthor) {  // author information  is to be written
      if(histime==0) {
        histime= 1; hiscset= 1; hisuser= 0; }
      pw__obj_add_id(0x10);  // V 2 'histime'
      pw__obj_add_uint64(histime);
      pw__obj_add_id(0x18);  // V 3 'hiscset'
      pw__obj_add_uint64(hiscset);
      pw__obj_add_id(0x20);  // V 4 'hisuid'
      pw__obj_add_uint32(hisuid);
      pw__obj_add_id(0x28);  // V 5 'hisuser'
      if(hisuid==0) hisuser= "";
      stid= pstw_store(hisuser);
      pw__obj_add_uint32(stid);
      }  // author information  is to be written
    }  // version number is to be written
  pw__dc_ref= 0;
  }  // end   pw_relation()

static inline void pw_relation_ref(int64_t refid,int reftype,
    const char* refrole) {
  // write a relations object's refs
  int stid;  // string id

  pw__objp= pw__rel_roles;
  stid= pstw_store(refrole);
  pw__obj_add_uint32(stid);
  pw__objp= pw__rel_refids;
  pw__obj_add_sint64(refid-pw__dc_ref);
  pw__dc_ref= refid;
  pw__objp= pw__rel_types;
  pw__obj_add_uint32(reftype);
  }  // end   pw_relation_ref()

static inline void pw_relation_close() {
  // close writing relation object;
  pw__objp= pw__wayrel_keys;
  if(pw__objp->bufe==pw__objp->bufc)  // object is empty
    pw__obj_dispose();
  else
    pw__obj_close();
  pw__objp= pw__wayrel_vals;
  if(pw__objp->bufe==pw__objp->bufc)  // object is empty
    pw__obj_dispose();
  else
    pw__obj_close();
  pw__objp= pw__wayrel_his;
  if(pw__objp->bufe==pw__objp->bufc)  // object is empty
    pw__obj_dispose();
  else
    pw__obj_close();
  pw__objp= pw__rel_roles;
  if(pw__objp->bufe==pw__objp->bufc)  // object is empty
    pw__obj_dispose();
  else
    pw__obj_close();
  pw__objp= pw__rel_refids;
  if(pw__objp->bufe==pw__objp->bufc)  // object is empty
    pw__obj_dispose();
  else
    pw__obj_close();
  pw__objp= pw__rel_types;
  if(pw__objp->bufe==pw__objp->bufc)  // object is empty
    pw__obj_dispose();
  else
    pw__obj_close();
  pw__obj_close();
  }  // end   pw_relation_close()

//------------------------------------------------------------
// end   Module pw_   PBF write module
//------------------------------------------------------------



//------------------------------------------------------------
// Module posi_   OSM position module
//------------------------------------------------------------

// this module provides a geocoordinate table for to store
// the coordinates of all OSM objects;
// the procedures posi_set() (resp. posi_setbbox()) and
// posi_get() allow access to this tables;
// as usual, all identifiers of a module have the same prefix,
// in this case 'posi'; an underline will follow for a global
// accessible identifier, two underlines if the identifier
// is not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

struct posi__mem_struct {  // element of position array
  int64_t id;
  int32_t data[];
  } __attribute__((__packed__));
  // (do not change this structure; the search algorithm expects
  // the size of this structure to be 16 or 32 bytes)
  // data[] stands for either
  //   int32_t x,y;
  // or
  //   int32_t x,y,x1,y1,x2,y2;  // (including bbox)
  // remarks to .x:
  // if you get posi_nil as value for x, you may assume that
  // the object has been stored, but its geoposition is unknown;
  // remarks to .id:
  // the caller of posi_set() and posi_get() has to care about adding
  // global_otypeoffset10 to the id if the object is a way and
  // global_otypeoffset20 to the id if the object is a relation;
typedef struct posi__mem_struct posi__mem_t;
static posi__mem_t* posi__mem= NULL;  // start address of position array
static posi__mem_t* posi__meme= NULL;  // logical end address
static posi__mem_t* posi__memee= NULL;  // physical end address

static void posi__end() {
  // clean-up for posi module;
  // will be called at program's end;
  if(posi__mem==NULL)
    PERR("not enough memory. Reduce --max-objects=")
  else {  // was initialized
    if(posi__meme>=posi__memee)  // not enough space in position array
      PERR("not enough space. Increase --max-objects=")
    else {
      int64_t siz;

      siz= (char*)posi__memee-(char*)posi__mem;
      siz= siz/4*3;
      if((char*)posi__meme-(char*)posi__mem>siz)
          // low space in position array
        WARN("low space. Try to increase --max-objects=")
      }
    free(posi__mem);
    posi__mem= NULL;
    }
  }  // end   posi__end()

//------------------------------------------------------------

static size_t posi__mem_size= 0;  // size of structure
static size_t posi__mem_increment= 0;
  // how many increments to ++ when allocating
static size_t posi__mem_mask= 0;
  // bitmask to start at base of structure

static int posi_ini() {
  // initialize the posi module;
  // return: 0: OK; 1: not enough memory;
  int64_t siz;

  global_otypeoffset05= global_otypeoffset10/2;
  global_otypeoffset15= global_otypeoffset10+global_otypeoffset05;
  global_otypeoffset20= global_otypeoffset10*2;
  if(global_otypeoffsetstep!=0)
    global_otypeoffsetstep= global_otypeoffset10;
  if(posi__mem!=NULL)  // already initialized
return 0;
  atexit(posi__end);  // chain-in the clean-up procedure
  // allocate memory for the positions array
  if (global_calccoords<0) {
    posi__mem_size = 32;
    posi__mem_mask = ~0x1f;
    posi__mem_increment = 4;
    }
  else {
    posi__mem_size = 16;
    posi__mem_mask = ~0x0f;
    posi__mem_increment = 2;
  }
  siz= posi__mem_size*global_maxobjects;
  posi__mem= (posi__mem_t*)malloc(siz);
  if(posi__mem==NULL)  // not enough memory
return 1;
  posi__meme= posi__mem;
  posi__memee= (posi__mem_t*)((char*)posi__mem+siz);
  return 0;
  }  // end   posi_ini()

static inline void posi_set(int64_t id,int32_t x,int32_t y) {
  // set geoposition for a specific object ID;
  // id: id of the object;
  // x,y: geocoordinates in 10^-7 degrees;
  if(posi__meme>=posi__memee)  // not enough space in position array
    exit(70001);
  posi__meme->id= id;
  posi__meme->data[0]= x;
  posi__meme->data[1]= y;
  if (global_calccoords<0) {
    posi__meme->data[2]= x; // x_min
    posi__meme->data[3]= y; // y_min
    posi__meme->data[4]= x; // x_max
    posi__meme->data[5]= y; // y_max
    }
  posi__meme+= posi__mem_increment;
  }  // end   posi_set()

static inline void posi_setbbox(int64_t id,int32_t x,int32_t y,
    int32_t xmin,int32_t ymin,int32_t xmax,int32_t ymax) {
  // same as posi_set(), however provide a bbox as well;
  // important: the caller must ensure that this module has been
  // initialized with global_calccoords= -1;
  if(posi__meme>=posi__memee)  // not enough space in position array
    exit(70001);
  posi__meme->id= id;
  posi__meme->data[0]= x;
  posi__meme->data[1]= y;
  posi__meme->data[2]= xmin;
  posi__meme->data[3]= ymin;
  posi__meme->data[4]= xmax;
  posi__meme->data[5]= ymax;
  posi__meme+= posi__mem_increment;
  }  // end   posi_setbbox()

static const int32_t posi_nil= 2000000000L;
static int32_t* posi_xy= NULL;  // position of latest read coordinates;
  // posi_xy[0]: x; posi_xy[1]: y;
  // posi_xy==NULL: no geoposition available for this id;

static inline void posi_get(int64_t id) {
  // get the previously stored geopositions of an object;
  // id: id of the object;
  // return: posi_xy[0]: x; posi_xy[1]: y;
  //         the caller may change the values for x and y;
  //         posi_xy==NULL: no geoposition available for this id;
  //         if this module has been initialized with global_calccoords
  //         set to -1, the bbox addresses will be returned too:
  //         posi_xy[2]: xmin; posi_xy[3]: ymin;
  //         posi_xy[4]: xmax; posi_xy[5]: ymax;
  char* min,*max,*middle;
  int64_t middle_id;

  min= (char*)posi__mem;
  max= (char*)posi__meme;
  while(max>min) {  // binary search
    middle= (((max-min-posi__mem_size)/2)&(posi__mem_mask))+min;
    middle_id= *(int64_t*)middle;
    if(middle_id==id) {  // we found the right object
      posi_xy= (int32_t*)(middle+8);
return;
      }
    if(middle_id>id)
      max= middle;
    else
      min= middle+posi__mem_size;
    }  // binary search
  // here: did not find the geoposition of the object in question
  posi_xy= NULL;
  }  // end   posi_geti();

//------------------------------------------------------------
// end   Module posi_   OSM position module
//------------------------------------------------------------



//------------------------------------------------------------
// Module posr_   object ref temporary module
//------------------------------------------------------------

// this module provides procedures to use a temporary file for
// storing relations' references when --all-to-nodes is used;
// as usual, all identifiers of a module have the same prefix,
// in this case 'posr'; an underline will follow for a global
// accessible identifier, two underlines if the identifier
// is not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static char posr__filename[400]= "";
static int posr__fd= -1;  // file descriptor for temporary file
#define posr__bufM 400000
static int64_t posr__buf[posr__bufM],
  *posr__bufp,*posr__bufe,*posr__bufee;
  // buffer - used for write, and later for read;
static bool posr__writemode;  // buffer is used for writing

static inline void posr__flush() {
  if(!posr__writemode || posr__bufp==posr__buf)
return;
  UR(write(posr__fd,posr__buf,(char*)posr__bufp-(char*)posr__buf))
  posr__bufp= posr__buf;
  }  // end   posr__flush()

static inline void posr__write(int64_t i) {
  // write an int64 to tempfile, use a buffer;
//DPv(posr__write %lli,i)
  if(posr__bufp>=posr__bufee) posr__flush();
  *posr__bufp++= i;
  }  // end   posr__write()

static void posr__end() {
  // clean-up for temporary file access;
  // will be called automatically at program end;
  if(posr__fd>2) {
    close(posr__fd);
    posr__fd= -1;
    }
  if(loglevel<2) unlink(posr__filename);
  }  // end   posr__end()

//------------------------------------------------------------

static int posr_ini(const char* filename) {
  // open a temporary file with the given name for random r/w access;
  // return: ==0: ok; !=0: error;
  strcpy(stpmcpy(posr__filename,filename,sizeof(posr__filename)-2),".2");
  if(posr__fd>=0)  // file already open
return 0;  // ignore this call
  unlink(posr__filename);
  posr__fd= open(posr__filename,O_RDWR|O_CREAT|O_TRUNC|O_BINARY,00600);
  if(posr__fd<0) {
    PERRv("could not open temporary file: %.80s",posr__filename)
return 1;
    }
  atexit(posr__end);
  posr__bufee= posr__buf+posr__bufM;
  posr__bufp= posr__bufe= posr__buf;
  posr__writemode= true;
  return 0;
  }  // end   posr_ini()

static inline void posr_rel(int64_t relid,bool is_area) {
  // store the id of a relation in tempfile;
  // relid: id of this relation;
  // is_area: this relation describes an area;
  //          otherwise: it describes a way;
  posr__write(0);
  posr__write(relid);
  posr__write(is_area);
  } // end   posr_rel()

static inline void posr_ref(int64_t refid) {
  // store the id of a reference in tempfile;
  posr__write(refid);
  } // end   posr_ref()

static int posr_rewind() {
  // rewind the file pointer;
  // return: ==0: ok; !=0: error;
  if(posr__writemode) {
    posr__flush(); posr__writemode= false; }
  if(lseek(posr__fd,0,SEEK_SET)<0) {
    PERRv("osmconvert Error: could not rewind temporary file %.80s",
      posr__filename)
return 1;
    }
  posr__bufp= posr__bufe= posr__buf;
  return 0;
  } // end   posr_rewind()

static inline int posr_read(int64_t* ip) {
  // read one integer; meaning of the values of these integers:
  // every value is an interrelation reference id, with one exception:
  // integers which follow a 0-integer directly are relation ids;
  // return: ==0: ok; !=0: eof;
  int r,r2;

  if(posr__bufp>=posr__bufe) {
    r= read(posr__fd,posr__buf,sizeof(posr__buf));
    if(r<=0)
return 1;
    posr__bufe= (int64_t*)((char*)posr__buf+r);
    if((r%8)!=0) { // odd number of bytes
      r2= read(posr__fd,posr__bufe,8-(r%8));
        // request the missing bytes
      if(r2<=0)  // did not get the missing bytes
        posr__bufe= (int64_t*)((char*)posr__bufe-(r%8));
      else
        posr__bufe= (int64_t*)((char*)posr__bufe+r2);
      }
    posr__bufp= posr__buf;
    }
  *ip= *posr__bufp++;
  return 0;
  }  // end   posr_read()

static void posr_processing(int* maxrewindp,int32_t** refxy) {
  // process temporary relation reference file;
  // the file must already have been written; this procedure
  // processes the interrelation references of this file and updates
  // the georeference table of module posi_ accordingly;
  // maxrewind: maximum number of rewinds;
  // refxy: memory space provided by the caller;
  //        this is a temporarily used space for the coordinates
  //        of the relations' members;
  // return:
  // maxrewind: <0: maximum number of rewinds was not sufficient;
  int changed;
    // number of relations whose flag has been changed, i.e.,
    // the recursive processing will continue;
    // if none of the relations' flags has been changed,
    // this procedure will end;
  int h;  // counter for interrelational hierarchies
  int64_t relid;  // relation id;
  int64_t refid;  // interrelation reference id;
  bool jump_over;  // jump over the presently processed relation
  int32_t* xy_rel;  // geocoordinates of the processed relation;
  int32_t x_min,x_max,y_min,y_max;
  int32_t x_middle,y_middle,xy_distance,new_distance;
  int n;  // number of referenced objects with coordinates
  int64_t temp64;
  bool is_area;  // the relation describes an area
  int32_t** refxyp;  // pointer in refxy array
  int r;

  h= 0; n=0;
  jump_over= true;
  relid= 0;
  while(*maxrewindp>=0) {  // for every recursion
    changed= 0;
    if(posr_rewind())  // could not rewind
  break;
    for(;;) {  // for every reference
      for(;;) {  // get next id
        r= posr_read(&refid);
        if((r || refid==0) && n>0) {  // (EOF OR new relation) AND
            // there have been coordinates for this relation
          x_middle= x_max/2+x_min/2;
          y_middle= (y_max+y_min)/2;
          // store the coordinates for this relation
//DPv(is_area %i refxy %i,is_area,refxyp==refxy)
          if(global_calccoords<0) {
            xy_rel[2]= x_min;
            xy_rel[3]= y_min;
            xy_rel[4]= x_max;
            xy_rel[5]= y_max;
            }
          if(is_area || refxyp==refxy) {
            // take the center as position for this relation
            xy_rel[0]= x_middle;
            xy_rel[1]= y_middle;
            }
          else {  // not an area
            int32_t x,y;

            // get the member position which is the nearest
            // to the center
            posi_xy= *--refxyp;
            x= posi_xy[0];
            y= posi_xy[1];
            xy_distance= abs(x-x_middle)+abs(y-y_middle);
            while(refxyp>refxy) {
              refxyp--;
              new_distance= abs(posi_xy[0]-x_middle)+
                abs(posi_xy[1]-y_middle);
              if(new_distance<xy_distance) {
                x= posi_xy[0];
                y= posi_xy[1];
                xy_distance= new_distance;
                }
              }
            xy_rel[0]= x;
            xy_rel[1]= y;
            }  // not an area
          n= 0;
          changed++;  // memorize that we calculated
            // at least one relation's position
          }
        if(r)
          goto rewind;  // if at file end, rewind
        if(refid!=0)
      break;
        // here: a relation id will follow
        posr_read(&relid);  // get the relation's id
        posr_read(&temp64);  // get the relation's area flag
        is_area= temp64;
        posi_get(relid+global_otypeoffset20);
          // get the relation's geoposition
        xy_rel= posi_xy;  // save address of relation's coordinate
        refxyp= refxy;  // restart writing coordinate buffer
        jump_over= xy_rel==NULL || xy_rel[0]!=posi_nil;
        }  // end   get next id
      if(jump_over)  // no element allocated for this very relation OR
        // position of this relation already known
    continue;  // go on until next relation
      posi_get(refid);  // get the reference's geoposition
      if(posi_xy==NULL || posi_xy[0]==posi_nil) {
          // position is unknown
        if(refid>global_otypeoffset15) {  // refers to a relation
          n= 0;  // ignore previously collected coordinates
          jump_over= true;  // no yet able to determine the position
          }
    continue;  // go on and examine next reference of this relation
        }
      *refxyp++= posi_xy;  // store coordinate for reprocessing later
      if(n==0) {  // first coordinate
        if(global_calccoords<0) {
          x_min = posi_xy[2];
          y_min = posi_xy[3];
          x_max = posi_xy[4];
          y_max = posi_xy[5];
          }
        else {
          // just store it as min and max
          x_min= x_max= posi_xy[0];
          y_min= y_max= posi_xy[1];
          }
        }
      else if(global_calccoords<0) {
        // adjust extrema
        if(posi_xy[2]<x_min && x_min-posi_xy[2]<900000000)
          x_min= posi_xy[2];
        else if(posi_xy[4]>x_max && posi_xy[4]-x_max<900000000)
          x_max= posi_xy[4];
        if(posi_xy[3]<y_min)
          y_min= posi_xy[3];
        else if(posi_xy[5]>y_max)
          y_max= posi_xy[5];
        }
      else {  // additional coordinate
        // adjust extrema
        if(posi_xy[0]<x_min && x_min-posi_xy[0]<900000000)
          x_min= posi_xy[0];
        else if(posi_xy[0]>x_max && posi_xy[0]-x_max<900000000)
          x_max= posi_xy[0];
        if(posi_xy[1]<y_min)
          y_min= posi_xy[1];
        else if(posi_xy[1]>y_max)
          y_max= posi_xy[1];
        }
      n++;
      }  // end   for every reference
    rewind:
    if(loglevel>0) fprintf(stderr,
      "Interrelational hierarchy %i: %i dependencies.\n",++h,changed);
    if(changed==0)  // no changes have been made in last recursion
  break;  // end the processing
    (*maxrewindp)--;
    }  // end   for every recursion
  }  // end   posr_processing()

//------------------------------------------------------------
// end   Module posr_   object ref temporary module
//------------------------------------------------------------



//------------------------------------------------------------
// Module rr_   relref temporary module
//------------------------------------------------------------

// this module provides procedures to use a temporary file for
// storing relation's references;
// as usual, all identifiers of a module have the same prefix,
// in this case 'rr'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static char rr__filename[400]= "";
static int rr__fd= -1;  // file descriptor for temporary file
#define rr__bufM 400000
static int64_t rr__buf[rr__bufM],*rr__bufp,*rr__bufe,*rr__bufee;
  // buffer - used for write, and later for read;
static bool rr__writemode;  // buffer is used for writing

static inline void rr__flush() {
  if(!rr__writemode || rr__bufp==rr__buf)
return;
  UR(write(rr__fd,rr__buf,(char*)rr__bufp-(char*)rr__buf))
  rr__bufp= rr__buf;
  }  // end   rr__flush()

static inline void rr__write(int64_t i) {
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
    fprintf(stderr,
      "osmconvert Error: could not open temporary file: %.80s\n",
      rr__filename);
return 1;
    }
  atexit(rr__end);
  rr__bufee= rr__buf+rr__bufM;
  rr__bufp= rr__bufe= rr__buf;
  rr__writemode= true;
  return 0;
  }  // end   rr_ini()

static inline void rr_rel(int64_t relid) {
  // store the id of a relation in tempfile;
  rr__write(0);
  rr__write(relid);
  } // end   rr_rel()

static inline void rr_ref(int64_t refid) {
  // store the id of an interrelation reference in tempfile;
  rr__write(refid);
  } // end   rr_ref()

static int rr_rewind() {
  // rewind the file pointer;
  // return: ==0: ok; !=0: error;
  if(rr__writemode) {
    rr__flush(); rr__writemode= false; }
  if(lseek(rr__fd,0,SEEK_SET)<0) {
    fprintf(stderr,"osmconvert Error: could not rewind temporary file"
      " %.80s\n",rr__filename);
return 1;
    }
  rr__bufp= rr__bufe= rr__buf;
  return 0;
  } // end   rr_rewind()

static inline int rr_read(int64_t* ip) {
  // read one integer; meaning of the values of these integers:
  // every value is an interrelation reference id, with one exception:
  // integers which follow a 0-integer directly are relation ids;
  // note that we take 64-bit-integers although the number of relations
  // will never exceed 2^31; the reason is that Free OSM ("FOSM") uses
  // IDs > 2^16 for new data which adhere the cc-by-sa license;
  // return: ==0: ok; !=0: eof;
  int r,r2;

  if(rr__bufp>=rr__bufe) {
    r= read(rr__fd,rr__buf,sizeof(rr__buf));
    if(r<=0)
return 1;
    rr__bufe= (int64_t*)((char*)rr__buf+r);
    if((r%8)!=0) { // odd number of bytes
      r2= read(rr__fd,rr__bufe,8-(r%8));  // request the missing bytes
      if(r2<=0)  // did not get the missing bytes
        rr__bufe= (int64_t*)((char*)rr__bufe-(r%8));
      else
        rr__bufe= (int64_t*)((char*)rr__bufe+r2);
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
// Module cwn_   complete way ref temporary module
//------------------------------------------------------------

// this module provides procedures to use a temporary file for
// storing a list of nodes which have to be marked as 'inside';
// this is used if option --complete-ways is invoked;
// as usual, all identifiers of a module have the same prefix,
// in this case 'posi'; an underline will follow for a global
// accessible identifier, two underlines if the identifier
// is not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static char cwn__filename[400]= "";
static int cwn__fd= -1;  // file descriptor for temporary file
#define cwn__bufM 400000
static int64_t cwn__buf[cwn__bufM],
  *cwn__bufp,*cwn__bufe,*cwn__bufee;
  // buffer - used for write, and later for read;
static bool cwn__writemode;  // buffer is used for writing

static inline void cwn__flush() {
  if(!cwn__writemode || cwn__bufp==cwn__buf)
return;
  UR(write(cwn__fd,cwn__buf,(char*)cwn__bufp-(char*)cwn__buf))
  cwn__bufp= cwn__buf;
  }  // end   cwn__flush()

static inline void cwn__write(int64_t i) {
  // write an int64 to tempfile, use a buffer;
  if(cwn__bufp>=cwn__bufee) cwn__flush();
  *cwn__bufp++= i;
  }  // end   cwn__write()

static void cwn__end() {
  // clean-up for temporary file access;
  // will be called automatically at program end;
  if(cwn__fd>2) {
    close(cwn__fd);
    cwn__fd= -1;
    }
  if(loglevel<2) unlink(cwn__filename);
  }  // end   cwn__end()

//------------------------------------------------------------

static int cwn_ini(const char* filename) {
  // open a temporary file with the given name for random r/w access;
  // return: ==0: ok; !=0: error;
  strcpy(stpmcpy(cwn__filename,filename,sizeof(cwn__filename)-2),".3");
  if(cwn__fd>=0)  // file already open
return 0;  // ignore this call
  unlink(cwn__filename);
  cwn__fd= open(cwn__filename,O_RDWR|O_CREAT|O_TRUNC|O_BINARY,00600);
  if(cwn__fd<0) {
    PERRv("could not open temporary file: %.80s",cwn__filename)
return 1;
    }
  atexit(cwn__end);
  cwn__bufee= cwn__buf+cwn__bufM;
  cwn__bufp= cwn__bufe= cwn__buf;
  cwn__writemode= true;
  return 0;
  }  // end   cwn_ini()

static inline void cwn_ref(int64_t refid) {
  // store the id of a referenced node in tempfile;
  cwn__write(refid);
  } // end   cwn_ref()

static int cwn_rewind() {
  // rewind the file pointer;
  // return: ==0: ok; !=0: error;
  if(cwn__writemode) {
    cwn__flush(); cwn__writemode= false; }
  if(lseek(cwn__fd,0,SEEK_SET)<0) {
    PERRv("osmconvert Error: could not rewind temporary file %.80s",
      cwn__filename)
return 1;
    }
  cwn__bufp= cwn__bufe= cwn__buf;
  return 0;
  } // end   cwn_rewind()

static inline int cwn_read(int64_t* ip) {
  // read the id of next referenced node;
  // return: ==0: ok; !=0: eof;
  int r,r2;

  if(cwn__bufp>=cwn__bufe) {
    r= read(cwn__fd,cwn__buf,sizeof(cwn__buf));
    if(r<=0)
return 1;
    cwn__bufe= (int64_t*)((char*)cwn__buf+r);
    if((r%8)!=0) { // odd number of bytes
      r2= read(cwn__fd,cwn__bufe,8-(r%8));
        // request the missing bytes
      if(r2<=0)  // did not get the missing bytes
        cwn__bufe= (int64_t*)((char*)cwn__bufe-(r%8));
      else
        cwn__bufe= (int64_t*)((char*)cwn__bufe+r2);
      }
    cwn__bufp= cwn__buf;
    }
  *ip= *cwn__bufp++;
  return 0;
  }  // end   cwn_read()

static void cwn_processing() {
  // process temporary node reference file;
  // the file must already have been written; this procedure
  // sets the a flag in hash table (module hash_) for each node
  // which is referred to by an entry in the temporary file;
  int64_t id;  // node id;

  if(cwn_rewind())  // could not rewind
return;
  for(;;) {  // get next id
    if(cwn_read(&id))
  break;
    hash_seti(0,id);
    }
  }  // end   cwn_processing()

//------------------------------------------------------------
// end   Module cwn_   complete way ref temporary module
//------------------------------------------------------------



//------------------------------------------------------------
// Module cww_   complex way ref temporary module
//------------------------------------------------------------

// this module provides procedures to use a temporary file for
// storing a list of ways which have to be marked as 'inside';
// this is used if option --complex-ways is invoked;
// as usual, all identifiers of a module have the same prefix,
// in this case 'posi'; an underline will follow for a global
// accessible identifier, two underlines if the identifier
// is not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static char cww__filename[400]= "";
static int cww__fd= -1;  // file descriptor for temporary file
#define cww__bufM 400000
static int64_t cww__buf[cww__bufM],
  *cww__bufp,*cww__bufe,*cww__bufee;
  // buffer - used for write, and later for read;
static bool cww__writemode;  // buffer is used for writing

static inline void cww__flush() {
  if(!cww__writemode || cww__bufp==cww__buf)
return;
  UR(write(cww__fd,cww__buf,(char*)cww__bufp-(char*)cww__buf))
  cww__bufp= cww__buf;
  }  // end   cww__flush()

static inline void cww__write(int64_t i) {
  // write an int64 to tempfile, use a buffer;
  if(cww__bufp>=cww__bufee) cww__flush();
  *cww__bufp++= i;
  }  // end   cww__write()

static void cww__end() {
  // clean-up for temporary file access;
  // will be called automatically at program end;
  if(cww__fd>2) {
    close(cww__fd);
    cww__fd= -1;
    }
  if(loglevel<2) unlink(cww__filename);
  }  // end   cww__end()

//------------------------------------------------------------

static int cww_ini(const char* filename) {
  // open a temporary file with the given name for random r/w access;
  // return: ==0: ok; !=0: error;
  strcpy(stpmcpy(cww__filename,filename,sizeof(cww__filename)-2),".5");
  if(cww__fd>=0)  // file already open
return 0;  // ignore this call
  unlink(cww__filename);
  cww__fd= open(cww__filename,O_RDWR|O_CREAT|O_TRUNC|O_BINARY,00600);
  if(cww__fd<0) {
    PERRv("could not open temporary file: %.80s",cww__filename)
return 1;
    }
  atexit(cww__end);
  cww__bufee= cww__buf+cww__bufM;
  cww__bufp= cww__bufe= cww__buf;
  cww__writemode= true;
  return 0;
  }  // end   cww_ini()

static inline void cww_ref(int64_t refid) {
  // store the id of a referenced way in tempfile;
  cww__write(refid);
  } // end   cww_ref()

static int cww_rewind() {
  // rewind the file pointer;
  // return: ==0: ok; !=0: error;
  if(cww__writemode) {
    cww__flush(); cww__writemode= false; }
  if(lseek(cww__fd,0,SEEK_SET)<0) {
    PERRv("osmconvert Error: could not rewind temporary file %.80s",
      cww__filename)
return 1;
    }
  cww__bufp= cww__bufe= cww__buf;
  return 0;
  } // end   cww_rewind()

static inline int cww_read(int64_t* ip) {
  // read the id of next referenced node;
  // return: ==0: ok; !=0: eof;
  int r,r2;

  if(cww__bufp>=cww__bufe) {
    r= read(cww__fd,cww__buf,sizeof(cww__buf));
    if(r<=0)
return 1;
    cww__bufe= (int64_t*)((char*)cww__buf+r);
    if((r%8)!=0) { // odd number of bytes
      r2= read(cww__fd,cww__bufe,8-(r%8));
        // request the missing bytes
      if(r2<=0)  // did not get the missing bytes
        cww__bufe= (int64_t*)((char*)cww__bufe-(r%8));
      else
        cww__bufe= (int64_t*)((char*)cww__bufe+r2);
      }
    cww__bufp= cww__buf;
    }
  *ip= *cww__bufp++;
  return 0;
  }  // end   cww_read()

static void cww_processing_set() {
  // process temporary way reference file;
  // the file must already have been written; this procedure
  // sets the a flag in hash table (module hash_) for each way
  // which is referred to by an entry in the temporary file;
  int64_t id;  // way id;

  if(cww__filename[0]==0)  // not initialized
return;
  if(cww_rewind())  // could not rewind
return;
  for(;;) {  // get next id
    if(cww_read(&id))
  break;
    hash_seti(1,id);
    }
  }  // end   cww_processing_set()

static void cww_processing_clear() {
  // process temporary way reference file;
  // the file must already have been written; this procedure
  // clears the a flag in hash table (module hash_) for each way
  // which is referred to by an entry in the temporary file;
  int64_t id;  // way id;

  if(cww__filename[0]==0)  // not initialized
return;
  if(cww_rewind())  // could not rewind
return;
  for(;;) {  // get next id
    if(cww_read(&id))
  break;
    hash_cleari(1,id);
    }
  }  // end   cww_processing_clear()

//------------------------------------------------------------
// end   Module cww_   complex way ref temporary module
//------------------------------------------------------------



//------------------------------------------------------------
// Module o5_   o5m conversion module
//------------------------------------------------------------

// this module provides procedures which convert data to
// o5m format;
// as usual, all identifiers of a module have the same prefix,
// in this case 'o5'; an underline will follow in case of a
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

static inline void o5__resetvars() {
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

static inline void o5_reset() {
  // perform and write an o5m Reset;
  o5__resetvars();
  write_char(0xff);  // write .o5m Reset
  }  // end   o5_reset()

static int o5_ini() {
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
    fprintf(stderr,"osmconvert Error: .o5m memory overflow.\n");
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
    fprintf(stderr,"osmconvert Error: .o5m memory overflow.\n");
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
    fprintf(stderr,"osmconvert Error: .o5m memory overflow.\n");
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
    fprintf(stderr,"osmconvert Error: .o5m memory overflow.\n");
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

static inline void o5_type(int type) {
  // mark object type we are going to process now;
  // should be called every time a new object is started to be
  // written into o5_buf[];
  // type: object type; 0: node; 1: way; 2: relation;
  //       if object type has changed, a 0xff byte ("reset")
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
  byte lens[30],reflens[30];  // lengths as pbf numbers
  int len;  // object length
  int reflen;  // reference area length
  int reflenslen;  // length of pbf number of reflen

  // get some length information
  len= o5__bufp-o5__buf;
  if(len<=0) goto o5_write_end;
  reflen= 0;  // (default)
  if(o5__bufr1<o5__bufr0) o5__bufr1= o5__bufr0;
  if(o5__bufr0>o5__buf) {
      // reference area contains at least 1 byte
    reflen= o5__bufr1-o5__bufr0;
    reflenslen= o5_uvar32buf(reflens,reflen);
    len+= reflenslen;
    }  // end   reference area contains at least 1 byte

  // write header
  if(--len>=0) {
    write_char(o5__buf[0]);
    write_mem(lens,o5_uvar32buf(lens,len));
    }

  // write body
  if(o5__bufr0<=o5__buf)  // no reference area
    write_mem(o5__buf+1,o5__bufp-(o5__buf+1));
  else {  // valid reference area
    write_mem(o5__buf+1,o5__bufr0-(o5__buf+1));
    write_mem(reflens,reflenslen);
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
// in this case 'stw'; an underline will follow in case of a
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

  #if 0  // not used because strings would not be transparent anymore
  if(*s1==(char)0xff)  // string is marked as 'do-not-store';
return -1;
    #endif
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
  }  // end   stw__hash()

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
          "osmconvert: String table index restart %i\n",++rs);
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

static inline void str_switch(str_info_t* sh) {
  // switch to another string unit
  // sh: string unit handle;
  str__infop= sh;
  }  // end str_switch()

static inline void str_reset() {
  // clear string table;
  // must be called before any other procedure of this module
  // and may be called every time the string processing shall
  // be restarted;
  if(str__infop!=NULL)
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
// in this case 'wo'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static int wo__format= 0;  // output format;
  // 0: o5m; 11: native XML; 12: pbf2osm; 13: Osmosis; 14: Osmium;
  // 21: csv; -1: PBF;
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
  // write osm object author;
  // must not be called if writing PBF format;
  // hisver: version; 0: no author is to be written
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
  // PBF and csv: this procedure is not called;
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
  // must only be called if writing XML format;
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
    int32_t x1,int32_t y1,int32_t x2,int32_t y2,int64_t timestamp) {
  // start writing osm objects;
  // format: 0: o5m; 11: native XML;
  //         12: pbf2osm; 13: Osmosis; 14: Osmium; 21:csv; -1: PBF;
  // bboxvalid: the following bbox coordinates are valid;
  // x1,y1,x2,y2: bbox coordinates (base 10^-7);
  // timestamp: file timestamp; ==0: no timestamp given;
  if(format<-1 || (format >0 && format<11) ||
      (format >14 && format<21) || format>21) format= 0;
  wo__format= format;
  wo__logaction= global_outosc || global_outosh;
  if(wo__format==0) {  // o5m
    static const byte o5mfileheader[]= {0xff,0xe0,0x04,'o','5','m','2'};
    static const byte o5cfileheader[]= {0xff,0xe0,0x04,'o','5','c','2'};

    if(global_outo5c)
      write_mem(o5cfileheader,sizeof(o5cfileheader));
    else
      write_mem(o5mfileheader,sizeof(o5mfileheader));
    if(timestamp!=0) {  // timestamp has been supplied
      o5_byte(0xdc);  // timestamp
      o5_svar64(timestamp);
      o5_write();  // write this object
      }
    if(border_active)  // borders are to be applied
      border_querybox(&x1,&y1,&x2,&y2);
    if(border_active || bboxvalid) {
        // borders are to be applied OR bbox has been supplied
      o5_byte(0xdb);  // border box
      o5_svar32(x1); o5_svar32(y1);
      o5_svar32(x2); o5_svar32(y2);
      o5_write();  // write this object
      }
return;
    }  // end   o5m
  if(wo__format<0) {  // PBF
    if(border_active)  // borders are to be applied
      border_querybox(&x1,&y1,&x2,&y2);
    bboxvalid= bboxvalid || border_active;
    pw_ini();
    pw_header(bboxvalid,x1,y1,x2,y2,timestamp);
return;
    }
  if(wo__format==21) {  // csv
    csv_headline();
return;
    }
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
    write_str(" generator=\"osmconvert "VERSION"\"");
    break;
  case 12:  // pbf2osm XML
    write_str(" generator=\"pbf2osm\"");
    break;
  case 13:  // Osmosis XML
    write_str(" generator=\"Osmosis 0.40\"");
    break;
  case 14:  // Osmium XML
    write_str(" generator="
      "\"Osmium (http://wiki.openstreetmap.org/wiki/Osmium)\"");
    break;
    }  // end   depending on output format
  if(timestamp!=0) {
    write_str(" timestamp=\""); write_timestamp(timestamp);
    write_char('\"');
    }
  write_str(">"NL);
  if(wo__format!=12) {  // bbox may be written
    if(border_active)  // borders are to be applied
      border_querybox(&x1,&y1,&x2,&y2);
    if(border_active || bboxvalid) {  // borders are to be applied OR
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
  case 21:  // csv
    csv_write();
      // (just in case the last object has not been terminated)
    break;
  case -1:  // PBF
    pw_foot();
    break;
    }  // end   depending on output format
  }  // end   wo_end()

static inline void wo_flush() {
  // write temporarily stored object information;
  if(wo__format==0)  // o5m
    o5_write();  // write last object - if any
  else if(wo__format<0)  // PBF format
    pw_flush();
  else if(wo__format==21)  // csv
    csv_write();
  else  // any XML output format
    wo__CLOSE
  write_flush();
  }  // end   wo_flush()

static int wo_format(int format) {
  // get or change output format;
  // format: -9: return the currently used format, do not change it;
  if(format==-9)  // do not change the format
return wo__format;
  wo_flush();
  if(format<-1 || (format >0 && format<11) ||
      (format >14 && format<21) || format>21) format= 0;
  wo__format= format;
  wo__logaction= global_outosc || global_outosh;
  return wo__format;
  }  // end   wo_format()

static inline void wo_reset() {
  // in case of o5m format, write a Reset;
  // note that this is done automatically at every change of
  // object type; this procedure offers to write additional Resets
  // at every time you want;
  if(wo__format==0)
    o5_reset();
  }  // end   wo_reset()

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
  if(wo__format<0) {  // PBF
    pw_node(id,hisver,histime,hiscset,hisuid,hisuser,lon,lat);
return;
    }
  if(wo__format==21) {  // csv
    char s[25];

    if(csv_key_otype)
      csv_add("@otype","0");
    if(csv_key_oname)
      csv_add("@oname",ONAME(0));
    if(csv_key_id) {
      int64toa(id,s);
      csv_add("@id",s);
      }
    if(csv_key_version) {
      uint32toa(hisver,s);
      csv_add("@version",s);
      }
    if(csv_key_timestamp) {
      write_createtimestamp(histime,s);
      csv_add("@timestamp",s);
      }
    if(csv_key_changeset) {
      int64toa(hiscset,s);
      csv_add("@changeset",s);
      }
    if(csv_key_uid) {
      uint32toa(hisuid,s);
      csv_add("@uid",s);
      }
    if(csv_key_user)
      csv_add("@user",hisuser);
    if(csv_key_lon) {
      write_createsfix7o(lon,s);
      csv_add("@lon",s);
      }
    if(csv_key_lat) {
      write_createsfix7o(lat,s);
      csv_add("@lat",s);
      }
return;
    }
  // here: XML format
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

static inline void wo_node_close() {
  // complete writing an OSM node;
  if(wo__format<0)
    pw_node_close();
  else if(wo__format==21)
    csv_write();
  }  // end   wo_node_close()

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
  if(wo__format<0) {  // PBF
    pw_way(id,hisver,histime,hiscset,hisuid,hisuser);
return;
    }
  if(wo__format==21) {  // csv
    char s[25];

    if(csv_key_otype)
      csv_add("@otype","1");
    if(csv_key_oname)
      csv_add("@oname",ONAME(1));
    if(csv_key_id) {
      int64toa(id,s);
      csv_add("@id",s);
      }
    if(csv_key_version) {
      uint32toa(hisver,s);
      csv_add("@version",s);
      }
    if(csv_key_timestamp) {
      write_createtimestamp(histime,s);
      csv_add("@timestamp",s);
      }
    if(csv_key_changeset) {
      int64toa(hiscset,s);
      csv_add("@changeset",s);
      }
    if(csv_key_uid) {
      uint32toa(hisuid,s);
      csv_add("@uid",s);
      }
    if(csv_key_user)
      csv_add("@user",hisuser);
return;
    }
  // here: XML format
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

static inline void wo_way_close() {
  // complete writing an OSM way;
  if(wo__format<0)
    pw_way_close();
  else if(wo__format==21)
    csv_write();
  }  // end   wo_way_close()

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
  if(wo__format<0) {  // PBF
    pw_relation(id,hisver,histime,hiscset,hisuid,hisuser);
return;
    }
  if(wo__format==21) {  // csv
    char s[25];

    if(csv_key_otype)
      csv_add("@otype","2");
    if(csv_key_oname)
      csv_add("@oname",ONAME(2));
    if(csv_key_id) {
      int64toa(id,s);
      csv_add("@id",s);
      }
    if(csv_key_version) {
      uint32toa(hisver,s);
      csv_add("@version",s);
      }
    if(csv_key_timestamp) {
      write_createtimestamp(histime,s);
      csv_add("@timestamp",s);
      }
    if(csv_key_changeset) {
      int64toa(hiscset,s);
      csv_add("@changeset",s);
      }
    if(csv_key_uid) {
      uint32toa(hisuid,s);
      csv_add("@uid",s);
      }
    if(csv_key_user)
      csv_add("@user",hisuser);
return;
    }
  // here: XML format
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

static inline void wo_relation_close() {
  // complete writing an OSM relation;
  if(wo__format<0)
    pw_relation_close();
  else if(wo__format==21)
    csv_write();
  }  // end   wo_relation_close()

static void wo_delete(int otype,int64_t id,
    int32_t hisver,int64_t histime,int64_t hiscset,
    uint32_t hisuid,const char* hisuser) {
  // write osm delete request;
  // this is possible for o5m format only;
  // for any other output format, this procedure does nothing;
  // otype: 0: node; 1: way; 2: relation;
  // id: id of this object;
  // hisver: version; 0: no author informaton is to be written
  //                     (necessary if o5m format);
  // histime: time (seconds since 1970)
  // hiscset: changeset
  // hisuid: uid
  // hisuser: user name
  if(otype<0 || otype>2 || wo__format<0)
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
    if(global_fakelonlat)
      write_str("\" lat=\"0\" lon=\"0");
    wo__author(hisver,histime,hiscset,hisuid,hisuser);
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
  if(wo__format<0) {  // PBF
    pw_way_ref(noderef);
return;
    }
  if(wo__format==21)  // csv
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
  if(wo__format<0) {  // PBF
    pw_relation_ref(refid,reftype,refrole);
return;
    }
  if(wo__format==21)  // csv
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

static inline void wo_node_keyval(const char* key,const char* val) {
  // write an OSM node object's keyval;
  if(wo__format==0) {  // o5m
    #if 0  // not used because strings would not be transparent anymore
    if(key[1]=='B' && strcmp(key,"bBox")==0 && strchr(val,',')!=0)
        // value is assumed to be dynamic, hence it should not be
        // stored in string list;
      // mark string pair as 'do-not-store';
      key= "\xff""bBox";  // 2012-10-14
      #endif
    stw_write(key,val);
return;
    }  // end   o5m
  if(wo__format<0) {  // PBF
    pw_node_keyval(key,val);
return;
    }
  if(wo__format==21) {  // csv
    csv_add(key,val);
return;
    }
  // here: XML format
  wo__CONTINUE
  switch(wo__format) {  // depending on output format
  case 11:  // native XML
    write_str("\t\t<tag k=\""); write_xmlstr(key);
    write_str("\" v=\"");
write_xmlstr(val);
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
  }  // end   wo_node_keyval()

static inline void wo_wayrel_keyval(const char* key,const char* val) {
  // write an OSM way or relation object's keyval;
  if(wo__format==0) {  // o5m
    stw_write(key,val);
return;
    }  // end   o5m
  if(wo__format<0) {  // PBF
    pw_wayrel_keyval(key,val);
return;
    }
  if(wo__format==21) {  // csv
    csv_add(key,val);
return;
    }
  // here: XML format
  wo__CONTINUE
  switch(wo__format) {  // depending on output format
  case 11:  // native XML
    write_str("\t\t<tag k=\""); write_xmlstr(key);
    write_str("\" v=\"");
write_xmlstr(val);
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
  }  // end   wo_wayrel_keyval()


static inline void wo_addbboxtags(bool fornode,
    int32_t x_min, int32_t y_min,int32_t x_max, int32_t y_max) {
  // adds tags for bbox and box area if requested by
  // global_addbbox, global_addbboxarea resp. global_addbboxweight;
  // fornode: add the tag(s) to a node, not to way/rel;
  char s[84],*sp;
  int64_t area;

  if(global_addbbox) {  // add bbox tags
    sp= s;
    sp= write_createsfix7o(x_min,sp);
    *sp++= ',';
    sp= write_createsfix7o(y_min,sp);
    *sp++= ',';
    sp= write_createsfix7o(x_max,sp);
    *sp++= ',';
    sp= write_createsfix7o(y_max,sp);
    if(fornode)
      wo_node_keyval("bBox",s);
    else
      wo_wayrel_keyval("bBox",s);
    }  // add bbox tags
  if(global_addbboxarea|global_addbboxweight) {
      // add bbox area tags OR add bbox weight tags
    area= (int64_t)(x_max-x_min)*(int64_t)(y_max-y_min)/
      cosrk((y_min+y_max)/2);
    if(global_addbboxarea) {  // add bbox area tags
      write_createsint64(area,s);
      if(fornode)
        wo_node_keyval("bBoxArea",s);
      else
        wo_wayrel_keyval("bBoxArea",s);
      }  // add bbox area tags
    if(global_addbboxweight) {  // add bbox weight tags
      write_createsint64(msbit(area),s);
      if(fornode)
        wo_node_keyval("bBoxWeight",s);
      else
        wo_wayrel_keyval("bBoxWeight",s);
      }  // add bbox weight tags
    }  // add bbox area tags OR add bbox weight tags
  } // end wo_addbboxtags()

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
// in this case 'oo'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static void oo__rrprocessing(int* maxrewindp) {
  // process temporary relation reference file;
  // the file must have been written; this procedure processes
  // the interrelation references of this file and updates
  // the hash table of module hash_ accordingly;
  // maxrewind: maximum number of rewinds;
  // return:
  // maxrewind: <0: maximum number of rewinds was not sufficient;
  int changed;
    // number of relations whose flag has been changed, i.e.,
    // the recursive processing will continue;
    // if none of the relations' flags has been changed,
    // this procedure will end;
  int h;
  int64_t relid;  // relation id;
  int64_t refid;  // interrelation reference id;
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
        flag= hash_geti(2,relid);  // get the related flag
        }  // end   get next id
      if(flag)  // flag already set
    continue;  // go on until next relation
      if(!hash_geti(2,refid))  // flag of reference is not set
    continue;  // go on and examine next reference of this relation
      hash_seti(2,relid);  // set flag of this relation
      flag= true;
      changed++;  // memorize that we changed a flag
      }  // end   for every reference
    rewind:
    if(loglevel>0) fprintf(stderr,
      "Interrelational hierarchy %i: %i dependencies.\n",++h,changed);
    if(changed==0)  // no changes have been made in last recursion
  break;  // end the processing
    (*maxrewindp)--;
    }  // end   for every recursion
  }  // end   oo__rrprocessing()

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

static inline uint32_t oo__strtouint32(const char* s) {
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
  }  // end   oo__strtouint32()

#if 0  // presently unused
static inline int32_t oo__strtosint32(const char* s) {
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

static inline int64_t oo__strtosint64(const char* s) {
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

static inline int32_t oo__strtodeg(char* s) {
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
return border__nil;
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

static inline int64_t oo__strtimetosint64(const char* s) {
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
return time(NULL)+oo__strtosint64(s);
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
    // use replcement for timegm() because Windows does not know it
      #if 0
      if(original_timezone==&original_timezone_none) {
        original_timezone= getenv("TZ");
        putenv("TZ=");
        tzset();
        }
      #endif
    //DPv(timezone %lli,timezone)
return mktime(&tm)-timezone;
    #else
return timegm(&tm);
    #endif
    }  // regular timestamp
  }  // end   oo__strtimetosint64()

static inline void oo__xmltostr(char* s) {
  // read an xml string and convert is into a regular UTF-8 string,
  // for example: "Mayer&apos;s" -> "Mayer's";
  char* t;  // pointer in overlapping target string
  char c;
  uint32_t u;

  for(;;) {  // for all characters, until first '&' or string end;
    c= *s;
    if(c==0)  // no character to convert
return;
    if(c=='&')
  break;
    s++;
    }
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

static inline bool oo__xmltag() {
  // read the next xml key/val and return them both;
  // due to performance reasons, global and module global variables
  // are used;
  // read_bufp: address at which the reading begins;
  // oo__xmlheadtag: see above;
  // return: no more xml keys/vals to read inside the outer xml tag;
  // oo__xmlkey,oo__xmlval: newest xml key/val which have been read;
  //                        "","": encountered the end of an
  //                               enclosed xml tag;
  char xmldelim;
  char c;

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
      if(c=='>') {  // short tag ends here
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

static int oo__error= 0;  // error number which will be returned when
  // oo_main() terminates normal;
typedef struct {
  read_info_t* ri;  // file handle for input files
  read_info_t* riph;  // file handle for input files;
    // this is a copy of .ri because it may be necessary to reaccess
    // a file which has already been logically closed;
    // used by the procedures oo__rewind() and oo__closeall();
  int format;  // input file format;
    // ==-9: unknown; ==0: o5m; ==10: xml; ==-1: pbf;
  str_info_t* str;  // string unit handle (if o5m format)
  uint64_t tyid;  // type/id of last read osm object of this file
  uint32_t hisver;  // OSM object version; needed for creating diff file
  const char* filename;
  bool endoffile;
  int deleteobject;  // replacement for .osc <delete> tag
    // 0: not to delete; 1: delete this object; 2: delete from now on;
  int deleteobjectjump;  // same as before but as save value for jumps
    // 0: not to delete; 1: delete this object; 2: delete from now on;
  bool subtract;  // this file is to be subtracted, i.e., the
    // meaning of 'deleteobject' will be treated inversely;
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

static bool oo__bbvalid= false;
  // the following bbox coordinates are valid;
static int32_t oo__bbx1= 0,oo__bby1= 0,oo__bbx2= 0,oo__bby2= 0;
  // bbox coordinates (base 10^-7) of the first input file;
static int64_t oo__timestamp= 0;
  // file timestamp of the last input file which has a timestamp;
  // ==0: no file timestamp given;
static bool oo__alreadyhavepbfobject= false;

static void oo__mergebbox(int32_t bbx1,int32_t bby1,
    int32_t bbx2,int32_t bby2) {
  // merge new bbox coordinates to existing ones;
  // if there are no bbox coordinates at present,
  // just store the new coordinates;
  // bbx1 .. bby2: border box coordinates to merge;
  // return:
  // oo__bbvalid: following border box information is valid;
  // oo__bbx1 .. oo__bby2: border box coordinates;

  if(!oo__bbvalid) {  // not yet any bbox stored
    // just store the new coordinates as bbox
    oo__bbx1= bbx1;
    oo__bby1= bby1;
    oo__bbx2= bbx2;
    oo__bby2= bby2;
    oo__bbvalid= true;
    }  // not yet any bbox stored
  else {  // there is already a bbox
    // merge the new coordinates with the existing bbox
    if(bbx1<oo__bbx1) oo__bbx1= bbx1;
    if(bby1<oo__bby1) oo__bby1= bby1;
    if(bbx2>oo__bbx2) oo__bbx2= bbx2;
    if(bby2>oo__bby2) oo__bby2= bby2;
    }  // there is already a bbox
  }  // oo__mergebbox()

static void oo__findbb() {
  // find timestamp and border box in input file;
  // return:
  // oo__bbvalid: following border box information is valid;
  // oo__bbx1 .. oo__bby2: border box coordinates;
  // read_bufp will not be changed;
  byte* bufp,*bufe,*bufe1;
  int32_t bbx1= 0,bby1= 0,bbx2= 0,bby2= 0;
    // bbox coordinates (base 10^-7)

  bbx1= bby1= bbx2= bby2= 0;
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
      if(b==0xdc) {  // timestamp
        bufp++;
        l= pbf_uint32(&bufp);
        bufe1= bufp+l; if(bufe1>=bufe) bufe1= bufe;
        if(bufp<bufe1) oo__timestamp= pbf_sint64(&bufp);
        bufp= bufe1;
    continue;
        }  // timestamp
      if(b==0xdb) {  // border box
        bufp++;
        l= pbf_uint32(&bufp);
        bufe1= bufp+l; if(bufe1>=bufe) bufe1= bufe;
        if(bufp<bufe1) bbx1= pbf_sint32(&bufp);
        if(bufp<bufe1) bby1= pbf_sint32(&bufp);
        if(bufp<bufe1) bbx2= pbf_sint32(&bufp);
        if(bufp<bufe1) {
          bby2= pbf_sint32(&bufp);
          oo__mergebbox(bbx1,bby1,bbx2,bby2);
          }
        bufp= bufe1;
    continue;
        }  // border box
      bufp++;
      l= pbf_uint32(&bufp);  // jump over this dataset
      bufp+= l;  // jump over this dataset
      }  // end   for all bytes
    }  // end   o5m
  else if(oo__ifp->format>0) {  // osm xml
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
      else if(c1=='o' && c2=='s' && c3=='m') {
          // "<osm"
        // timestamp must be supplied in this format:
        // <osm version="0.6" generator="OpenStreetMap planet.c"
        // timestamp="2010-08-18T00:11:04Z">
        int l;
        char c;

        sp++;  // jump over '<'
        if(strzcmp(sp,"osmAugmentedDiff")==0)
          global_mergeversions= true;
        for(;;) {  // jump over "osm ", "osmChange ", "osmAugmentedDiff"
          c= *sp;
          if(oo__wsnul(c))
        break;
          sp++;
          }
        for(;;) {  // for every word in 'osm'
          c= *sp;
          if(c=='/' || c=='<' || c=='>' || c==0)
        break;
          if(oo__ws(c)) {
            sp++;
        continue;
            }
          if((l= strzlcmp(sp,"timestamp="))>0 &&
              (sp[10]=='\"' || sp[10]=='\'') && isdig(sp[11])) {
            sp+= l+1;
            oo__timestamp= oo__strtimetosint64(sp);
            }
          for(;;) {  // find next whitespace or '<'
            c= *sp;
            if(oo__wsnul(c))
          break;
            sp++;
            }
          }  // end   for every word in 'osm'
        bufp++;
    continue;
        }  // "<osm"
      else if(c1=='b' && c2=='o' && c3=='u') {  // bounds
        // bounds may be supplied in one of these formats:
        // <bounds minlat="53.01104" minlon="8.481593"
        //   maxlat="53.61092" maxlon="8.990601"/>
        // <bound box="49.10868,6.35017,49.64072,7.40979"
        //   origin="http://www.openstreetmap.org/api/0.6"/>
        uint32_t bboxcomplete;  // flags for bbx1 .. bby2
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
          if(c=='/' || c=='<' || c=='>' || c==0)
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
            bby1= oo__strtodeg(sp);
            if(bby1!=oo__nildeg) bboxcomplete|= 2;
            }
          else if((l= strzlcmp(sp,"minlon=\""))>0 ||
              (l= strzlcmp(sp,"minlon=\'"))>0 ||
              ((isdig(c) || c=='-' || c=='.') && (bboxcomplete&1)==0)) {
            sp+= l;
            bbx1= oo__strtodeg(sp);
            if(bbx1!=oo__nildeg) bboxcomplete|= 1;
            }
          else if((l= strzlcmp(sp,"maxlat=\""))>0 ||
              (l= strzlcmp(sp,"maxlat=\'"))>0 ||
              ((isdig(c) || c=='-' || c=='.') && (bboxcomplete&8)==0)) {
            sp+= l;
            bby2= oo__strtodeg(sp);
            if(bby2!=oo__nildeg) bboxcomplete|= 8;
            }
          else if((l= strzlcmp(sp,"maxlon=\""))>0 ||
              (l= strzlcmp(sp,"maxlon=\'"))>0 ||
              ((isdig(c) || c=='-' || c=='.') && (bboxcomplete&4)==0)) {
            sp+= l;
            bbx2= oo__strtodeg(sp);
            if(bbx2!=oo__nildeg) bboxcomplete|= 4;
            }
          for(;;) {  // find next blank or comma
            c= *sp;
            if(oo__wsnul(c) || c==',')
          break;
            sp++;
            }
          }  // end   for every word in 'bounds'
        if(bboxcomplete==15)
          oo__mergebbox(bbx1,bby1,bbx2,bby2);
        bufp++;
    continue;
        }  // bounds
      else {
        bufp++;
    continue;
        }
      }  // end   for all bytes of the file
    }  // end   osm xml
  else if(oo__ifp->format==-1) {  // pbf
    //pb_input();
    if(pb_type==8) {  // pbf header
      if(pb_bbvalid)
        oo__mergebbox(pb_bbx1,pb_bby1,pb_bbx2,pb_bby2);
      if(pb_filetimestamp!=0)
        oo__timestamp= pb_filetimestamp;
      }  // end   pbf header
    else
      oo__alreadyhavepbfobject= true;
    }  // end   pbf
  }  // end   oo__findbb()

static inline int oo__gettyid() {
  // get tyid of the next object in the currently processed input file;
  // tyid is a combination of object type and id: we take the id and
  // add UINT64_C(0x0800000000000000) for nodes,
  // UINT64_C(0x1800000000000000) for ways, and
  // UINT64_C(0x2800000000000000) for relations;
  // if global_diff is set, besides tyid the hisver is retrieved too;
  // oo__ifp: handle of the currently processed input file;
  // return: ==0: ok; !=0: could not get tyid because starting object
  //         is not an osm object;
  // oo__ifp->tyid: tyid of the starting osm object;
  //                if there is not an osm object starting at
  //                read_bufp, *iyidp remains unchanged;
  // oo__ifp->hisver: only if global_diff; version of next object;
  static const uint64_t idoffset[]= {UINT64_C(0x0800000000000000),
    UINT64_C(0x1800000000000000),UINT64_C(0x2800000000000000),
    0,0,0,0,0,0,0,0,0,0,0,0,0,UINT64_C(0x0800000000000000),
    UINT64_C(0x1800000000000000),UINT64_C(0x2800000000000000)};
  int format;

  format= oo__ifp->format;
  if(format==0) {  // o5m
    int64_t o5id;
    byte* p,b;
    int l;

    o5id= oo__ifp->o5id;
    p= read_bufp;
    while(p<read_bufe) {
      b= *p++;
      if(b>=0x10 && b<=0x12) {  // osm object is starting here
        oo__ifp->tyid= idoffset[b];
        pbf_intjump(&p);  // jump over length information
        oo__ifp->tyid+= o5id+pbf_sint64(&p);
        if(global_diff)
          oo__ifp->hisver= pbf_uint32(&p);
return 0;
        }
      if(b>=0xf0) {  // single byte
        if(b==0xff)  // this is an o5m Reset object
          o5id= 0;
    continue;
        }
      // here: unknown o5m object
      l= pbf_uint32(&p);  // get length of this object
      p+= l;  // jump over this object;
      }
return 1;
    }
  else if(format>0) {  // 10: osm xml
    char* s;
    uint64_t r;

    s= (char*)read_bufp;
    for(;;) {  // for every byte in XML object
      while(*s!='<' && *s!=0) s++;
      if(*s==0)
    break;
      s++;
      if(*s=='n' && s[1]=='o') r= UINT64_C(0x0800000000000000);
      else if(*s=='w'&& s[1]=='a') r= UINT64_C(0x1800000000000000);
      else if(*s=='r'&& s[1]=='e') r= UINT64_C(0x2800000000000000);
      else
    continue;
      do {
        s++;
        if(*s==0)
    break;
        } while(*s!=' ');
      while(*s==' ') s++;
      if(s[0]=='i' && s[1]=='d' && s[2]=='=' &&
          (s[3]=='\"' || s[3]=='\'')) {  // found id
        oo__ifp->tyid= r+oo__strtosint64(s+4);
        if(!global_diff)
return 0;
        oo__ifp->hisver= 0;
        for(;;) {
          if(*s=='>' || *s==0)
return 0;
          if(s[0]==' ' && s[1]=='v' && s[2]=='e' && s[3]=='r' &&
              s[4]=='s' && s[5]=='i' && s[6]=='o' && s[7]=='n' &&
              s[8]=='=' && (s[9]=='\"' || s[9]=='\'') && isdig(s[10])) {
              // found version
            oo__ifp->hisver= oo__strtouint32(s+10);
return 0;
            }
          s++;
          }
return 0;
        }  // end  found id
      }  // end   for every byte in XML object
return 1;
    }
  else if(format==-1) {  // pbf
    while(pb_type>2) {  // not an OSM object
      pb_input(false);
      oo__alreadyhavepbfobject= true;
      }
    if((pb_type & 3)!=pb_type)  // still not an osm object
return 1;
    oo__ifp->tyid= idoffset[pb_type]+pb_id;
    oo__ifp->hisver= pb_hisver;
return 0;
    }
return 2;  // (unknown format)
  }  // end   oo__gettyid()

static inline int oo__getformat() {
  // determine the formats of all opened files of unknown format
  // and store these determined formats;
  // do some intitialization for the format, of necessary;
  // oo__if[].format: !=-9: do nothing for this file;
  // return: 0: ok; !=0: error;
  //         5: too many pbf files;
  //            this is, because the module pbf (see above)
  //            does not have multi-client capabilities;
  // oo__if[].format: input file format; ==0: o5m; ==10: xml; ==-1: pbf;
  static int pbffilen= 0;  // number of pbf input files;
  oo__if_t* ifptemp;
  byte* bufp;
  #define bufsp ((char*)bufp)  // for signed char

  ifptemp= oo__ifp;
  oo__ifp= oo__if;
  while(oo__ifp<oo__ife) {  // for all input files
    if(oo__ifp->ri!=NULL && oo__ifp->format==-9) {
        // format not yet determined
      read_switch(oo__ifp->ri);
      if(read_bufp[0]==0xef && read_bufp[1]==0xbb &&
          read_bufp[2]==0xbf && read_bufp[3]=='<')
          // UTF-8 BOM detected
        read_bufp+= 3;  // jump over BOM
      if(read_bufp>=read_bufe) {  // file empty
        PERRv("file empty: %.80s",oo__ifp->filename)
return 2;
        }
      bufp= read_bufp;
      if(bufp[0]==0 && bufp[1]==0 && bufp[2]==0 &&
          bufp[3]>8 && bufp[3]<20) {  // presumably .pbf format
        if(++pbffilen>1) {   // pbf
          PERR("more than one .pbf input file is not allowed.");
return 5;
          }
        oo__ifp->format= -1;
        pb_ini();
        pb_input(false);
        oo__alreadyhavepbfobject= true;
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
          fprintf(stderr,"osmconvert: Not a standard .o5m file header "
            "%.80s\n",oo__ifp->filename);
        oo__ifp->format= 0;
        oo__ifp->str= str_open();
          // call some initialization of string read module
        }
      else {  // unknown file format
        PERRv("unknown file format: %.80s",oo__ifp->filename)
return 3;
        }
      oo__findbb();
      oo__ifp->tyid= 0;
      oo__ifp->hisver= 0;
      oo__gettyid();
        // initialize tyid of the currently used input file
      }  // format not yet determined
    oo__ifp++;
    }  // for all input files
  oo__ifp= ifptemp;
  if(loglevel>0 && oo__timestamp!=0) {
    char s[30];  // timestamp as string

    write_createtimestamp(oo__timestamp,s);
    fprintf(stderr,"osmconvert: File timestamp: %s\n",s);
    }
  if(global_timestamp!=0)  // user wants a new file timestamp
    oo__timestamp= global_timestamp;
  return 0;
  #undef bufsp
  }  // end oo__getformat()

static uint64_t oo__tyidold= 0;  // tyid of the last written object;

static inline void oo__switch() {
  // determine the input file with the lowest tyid
  // and switch to it
  oo__if_t* ifp,*ifpmin;
  uint64_t tyidmin,tyidold,tyid;

  // update tyid of the currently used input file and check sequence
  if(oo__ifp!=NULL) {  // handle of current input file is valid
    tyidold= oo__ifp->tyid;
    if(oo__gettyid()==0) {  // new tyid is valid
//DPv(got   %llx %s,oo__ifp->tyid,oo__ifp->filename)
      if(oo__ifp->tyid<tyidold) {  // wrong sequence
        int64_t id; int ty;

        oo__error= 91;
        ty= tyidold>>60;
        id= ((int64_t)(tyidold & UINT64_C(0xfffffffffffffff)))-
          INT64_C(0x800000000000000);
        WARNv("wrong order at %s %"PRIi64" in file %s",
          ONAME(ty),id,oo__ifp->filename)
        ty= oo__ifp->tyid>>60;
        id= ((int64_t)(oo__ifp->tyid & UINT64_C(0xfffffffffffffff)))-
          INT64_C(0x800000000000000);
        WARNv("next object is %s %"PRIi64,ONAME(ty),id)
        }  // wrong sequence
      }  // new tyid is valid
    }  // end   handle of current input file is valid

  // find file with smallest tyid
  tyidmin= UINT64_C(0xffffffffffffffff);
  ifpmin= oo__ifp;
    // default; therefore we do not switch in cases we do not
    // find a minimum
  ifp= oo__ife;
  while(ifp>oo__if) {
    ifp--;
    if(ifp->ri!=NULL) {  // file handle is valid
//DPv(have  %llx %s,ifp->tyid,ifp->filename)
      tyid= ifp->tyid;
      if(tyid<tyidmin) {
        tyidmin= tyid;
        ifpmin= ifp;
        }
      }  // file handle valid
    }

  // switch to that file
  if(ifpmin!=oo__ifp) {
      // that file is not the file we're already reading from
    oo__ifp= ifpmin;
    read_switch(oo__ifp->ri);
    str_switch(oo__ifp->str);
    }
//DPv(chose %llx %s,oo__ifp->tyid,oo__ifp->filename)
  }  // end oo__switch()

static int oo_sequencetype= -1;
  // type of last object which has been processed;
  // -1: no object yet; 0: node; 1: way; 2: relation;
static int64_t oo_sequenceid= INT64_C(-0x7fffffffffffffff);
  // id of last object which has been processed;

static void oo__reset(oo__if_t* ifp) {
  // perform a reset of output procedures and variables;
  // this is mandatory if reading .o5m or .pbf and jumping
  // within the input file;
  ifp->o5id= 0;
  ifp->o5lat= ifp->o5lon= 0;
  ifp->o5hiscset= 0;
  ifp->o5histime= 0;
  ifp->o5rid[0]= ifp->o5rid[1]= ifp->o5rid[2]= 0;
  str_reset();
  if(ifp->format==-1)
    pb_input(true);
  }  // oo__reset()

static int oo__rewindall() {
  // rewind all input files;
  // return: 0: ok; !=0: error;
  oo__if_t* ifp,*ifp_sav;

  ifp_sav= oo__ifp;  // save original info pointer
  ifp= oo__if;
  while(ifp<oo__ife) {
    if(ifp->riph!=NULL) {
      if(ifp->ri==NULL && ifp->riph!=NULL) {
          // file has been logically closed
        // logically reopen it
        ifp->ri= ifp->riph;
        oo_ifn++;
        }
      read_switch(ifp->ri);
      if(read_rewind())
return 1;
      ifp->tyid= 1;
      ifp->endoffile= false;
      ifp->deleteobject= 0;
      oo__reset(ifp);
      }
    ifp++;
    }
  oo__ifp= ifp_sav;  // restore original info pointer
  if(oo__ifp!=NULL && oo__ifp->ri!=NULL) {
    read_switch(oo__ifp->ri);
    str_switch(oo__ifp->str);
    }
  else
    oo__switch();
  oo__tyidold= 0;
  oo_sequencetype= -1;
  oo_sequenceid= INT64_C(-0x7fffffffffffffff);
  return 0;
  }  // end oo__rewindall()

static int oo__jumpall() {
  // jump in all input files to the previously stored position;
  // return: 0: ok; !=0: error;
  oo__if_t* ifp,*ifp_sav;
  int r;

  ifp_sav= oo__ifp;  // save original info pointer
  ifp= oo__if;
  while(ifp<oo__ife) {  // for all files
    if(ifp->riph!=NULL) {  // file is still physically open
      if(ifp->ri==NULL && ifp->riph!=NULL) {
          // file has been logically closed
        // logically reopen it
        ifp->ri= ifp->riph;
        oo_ifn++;
        }
      read_switch(ifp->ri);
      r= read_jump();
      if(r<0)  // jump error
return 1;
      if(r==0) {  // this was a real jump
        ifp->tyid= 1;
        ifp->endoffile= false;
        ifp->deleteobject= ifp->deleteobjectjump;
        oo__reset(ifp);
        }
      }  // file is still physically open
    ifp++;
    }  // for all files
  oo__ifp= ifp_sav;  // restore original info pointer
  if(oo__ifp!=NULL && oo__ifp->ri!=NULL) {
    read_switch(oo__ifp->ri);
    str_switch(oo__ifp->str);
    }
  else {
    oo__switch();
    if(oo__ifp==NULL) {  // no file chosen
      oo_ifn= 0;
      ifp= oo__if;
      while(ifp<oo__ife) {  // for all files
        ifp->ri= NULL;  // mark file as able to be logically reopened
        ifp++;
        }
      }
    }
  oo__tyidold= 0;
  oo_sequencetype= -1;
  oo_sequenceid= INT64_C(-0x7fffffffffffffff);
  return 0;
  }  // end oo__jumpall()

static void oo__close() {
  // logically close an input file;
  // oo__ifp: handle of currently active input file;
  // if this file has already been closed, nothing happens;
  // after calling this procedure, the handle of active input file
  // will be invalid; you may call oo__switch() to select the
  // next file in the sequence;
  if(oo__ifp!=NULL && oo__ifp->ri!=NULL) {
    if(!oo__ifp->endoffile  && oo_ifn>0)  // missing logical end of file
      fprintf(stderr,"osmconvert Warning: "
        "unexpected end of input file: %.80s\n",oo__ifp->filename);
    read_switch(oo__ifp->ri);
    //read_close();
    oo__ifp->ri= NULL;
    oo__ifp->tyid= UINT64_C(0xffffffffffffffff);
      // (to prevent this file being selected as next file
      // in the sequence)
    oo_ifn--;
    }
  oo__ifp= NULL;
  }  // end oo__close()

static void oo__closeall() {
  // close all input files;
  // after calling this procedure, the handle of active input file
  // will be invalid;
  oo_ifn= 0;  // mark end of program;
    // this is used to suppress warning messages in oo__close()
  while(oo__ife>oo__if) {
    oo__ifp= --oo__ife;
    oo__ifp->endoffile= true;  // suppress warnings (see oo__close())
    if(oo__ifp->riph!=NULL) {
      read_switch(oo__ifp->riph);
      read_close();
      }
    oo__ifp->ri= oo__ifp->riph= NULL;
    oo__ifp->tyid= UINT64_C(0xffffffffffffffff);
    }
  }  // end oo__closeall()

static void* oo__malloc_p[50];
  // pointers for controlled memory allocation
static int oo__malloc_n= 0;
  // number of elements used in oo__malloc_p[]

static void* oo__malloc(size_t size) {
  // same as malloc(), but the allocated memory will be
  // automatically freed at program end;
  void* mp;

  mp= malloc(size);
  if(mp==NULL) {
    PERRv("cannot allocate %"PRIi64" bytes of memory.",(int64_t)size);
    exit(1);
    }
  oo__malloc_p[oo__malloc_n++]= mp;
  return mp;
  }  // oo__malloc()

static void oo__end() {
  // clean-up this module;
  oo__closeall();
  while(oo__malloc_n>0)
    free(oo__malloc_p[--oo__malloc_n]);
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
    fprintf(stderr,"osmconvert Error: too many input files.\n");
    fprintf(stderr,"osmconvert Error: too many input files: %d>%d\n",
      (int)(oo__ife-oo__if),global_fileM);
return 2;
    }
  if(read_open(filename)!=0)
return 1;
  oo__ife->ri= oo__ife->riph= read_infop;
  oo__ife->str= NULL;
  oo__ife->format= -9;  // 'not yet determined'
  oo__ife->tyid= 0;
  if(filename==NULL)
    oo__ife->filename= "standard input";
  else
    oo__ife->filename= filename;
  oo__ife->endoffile= false;
  oo__ife->deleteobject= 0;
  oo__ife->subtract= global_subtract;
  oo__ifp= oo__ife++;
  oo_ifn++;
  if(firstrun) {
    firstrun= false;
    atexit(oo__end);
    }
  return 0;
  }  // end   oo_open()

int dependencystage;
  // stage of the processing of interobject dependencies:
  // interrelation dependencies, --complete-ways or --complex-ways;
  // processing in stages allows us to reprocess parts of the data;
  // abbrevation "ht" means hash table (module hash_);
  //
  // 0: no recursive processing at all;
  //
  // option --complex-ways:
  // 11:     no output;
  //         for each node which is inside the borders,
  //           set flag in ht;
  //         store start of ways in read_setjump();
  //         for each way which has a member with flag in ht,
  //           set the way's flag in ht;
  //         for each relation with a member with flag in ht,
  //           store the relation's flag and write the ids
  //           of member ways which have no flag in ht
  //           (use cww_);
  // 11->12: at all files' end:
  //         let all files jump to start of ways,
  //         use read_jump();
  //         set flags for ways, use cww_processing_set();
  // 12:     no output;
  //         for each way with a member with a flag in ht,
  //           set the way's flag in ht and write the ids
  //           of all the way's members (use cwn_);
  // 12->22: as soon as first relation shall be written:
  //         rewind all files;
  //         set flags for nodes, use cwn_processing();
  //
  // option --complete-ways:
  // 21:     no output;
  //         for each node inside the borders,
  //           set flag in ht;
  //         for each way with a member with a flag in ht,
  //           set the way's flag in ht and write the ids
  //           of all the way's members (use cwn_);
  // 21->22: as soon as first relation shall be written:
  //         rewind all files;
  //         set flags for nodes, use cwn_processing();
  // 22:     write each node which has a flag in ht to output;
  //         write each way which has a flag in ht to output;
  // 22->32: as soon as first relation shall be written:
  //         clear flags for ways, use cww_processing_clear();
  //         switch output to temporary file;
  //
  // regular procedure:
  // 31:     for each node inside the borders,
  //           set flag in ht;
  //         for each way with a member with a flag in ht,
  //           set the way's flag in ht;
  // 31->32: as soon as first relation shall be written:
  //         switch output to temporary .o5m file;
  // 32:     for each relation with a member with a flag
  //           in ht, set the relation's flag in ht;
  //         for each relation,
  //           write its id and its members' ids
  //           into a temporary file (use rr_);
  //         if option --all-to-nodes is set, then
  //           for each relation, write its members'
  //             geopositions into a temporary file (use posr_);
  // 32->33: at all files' end:
  //         process all interrelation references (use rr_);
  //         if option --all-to-nodes is set, then
  //           process position array (use posr_);
  //         switch input to the temporary .o5m file;
  //         switch output to regular output file;
  // 33:     write each relation which has a flag in ht
  //           to output; use temporary .o5m file as input;
  // 33->99: at all files' end: end this procedure;
  //
  // out-of-date:
  // 1: (this stage is applied only with --complete-ways option)
  //    read nodes and ways, do not write anything; change to next
  //    stage as soon as the first relation has been encountered;
  //    now: 21;
  // 1->2: at this moment, rewind all input files;
  //    now: 21->22;
  // 2: write nodes and ways, change to next stage as soon as
  //    the first relation has been encountered;
  //    now: 22 or 31;
  // 2->3: at this moment, change the regular output file to a
  //       tempfile, and switch output format to .o5m;
  //    now: 22->32 or 31->32;
  // 3: write interrelation references into a second to tempfile,
  //    use modules rr_ or posr_ for this purpose;
  //    now: 32;
  // 3->4: at this moment, change output back to standard output,
  //       and change input to the start of the tempfile;
  //       in addition to this, process temporarily stored
  //       interrelation data;
  //    now: 32->33;
  // 4: write only relations, use tempfile as input;
  //    now: 33;
static void oo__dependencystage(int ds) {
  // change the dependencystage;
  if(loglevel>=2)
    PINFOv("changing dependencystage from %i to %i.",dependencystage,ds)
  dependencystage= ds;
  }  // oo__dependencystage()

static int oo_main() {
  // start reading osm objects;
  // return: ==0: ok; !=0: error;
  // this procedure must only be called once;
  // before calling this procedure you must open an input file
  // using oo_open();
  int wformat;  // output format;
    // 0: o5m; 11,12,13,14: some different XML formats;
    // 21: csv; -1: PBF;
  bool hashactive;
    // must be set to true if border_active OR global_dropbrokenrefs;
  static char o5mtempfile[400];  // must be static because
    // this file will be deleted by an at-exit procedure;
  #define oo__maxrewindINI 12
  int maxrewind;  // maximum number of relation-relation dependencies
  int maxrewind_posr;  // same as before, but for --all-to-nodes
  bool writeheader;  // header must be written
  int otype;  // type of currently processed object;
    // 0: node; 1: way; 2: relation;
  int64_t id;
  int32_t lon,lat;
  uint32_t hisver;
  int64_t histime;
  int64_t hiscset;
  uint32_t hisuid;
  char* hisuser;
  int64_t* refid;  // ids of referenced object
  int64_t* refidee;  // end address of array
  int64_t* refide,*refidp;  // pointer in array
  int32_t** refxy;  // coordinates of referenced object
  int32_t** refxyp;  // pointer in array
  byte* reftype;  // types of referenced objects
  byte* reftypee,*reftypep;  // pointer in array
  char** refrole;  // roles of referenced objects
  char** refrolee,**refrolep;  // pointer in array
  #define oo__keyvalM 8000  // changed from 4000 to 8000
    // because there are old ways with this many key/val pairs
    // in full istory planet due to malicious Tiger import
  char* key[oo__keyvalM],*val[oo__keyvalM];
  char** keyee;  // end address of first array
  char** keye,**keyp;  // pointer in array
  char** vale,**valp;  // pointer in array
  byte* bufp;  // pointer in read buffer
  #define bufsp ((char*)bufp)  // for signed char
  byte* bufe;  // pointer in read buffer, end of object
  char c;  // latest character which has been read
  byte b;  // latest byte which has been read
  int l;
  byte* bp;
  char* sp;
  struct {
    int64_t nodes,ways,relations;  // number of objects
    int64_t node_id_min,node_id_max;
    int64_t way_id_min,way_id_max;
    int64_t relation_id_min,relation_id_max;
    int64_t timestamp_min,timestamp_max;
    int32_t lon_min,lon_max;
    int32_t lat_min,lat_max;
    int32_t keyval_pairs_max;
    int keyval_pairs_otype;
    int64_t keyval_pairs_oid;
    int32_t noderefs_max;
    int64_t noderefs_oid;
    int32_t relrefs_max;
    int64_t relrefs_oid;
    } statistics;
  bool diffcompare;  // the next object shall be compared
    // with the object which has just been read;
  bool diffdifference;
    // there was a difference in object comparison

  // procedure initialization
  atexit(oo__end);
  memset(&statistics,0,sizeof(statistics));
  oo__bbvalid= false;
  hashactive= border_active || global_dropbrokenrefs;
  dependencystage= 0;  // 0: no recursive processing at all;
  maxrewind= maxrewind_posr= oo__maxrewindINI;
  writeheader= true;
  if(global_outo5m) wformat= 0;
  else if(global_outpbf) wformat= -1;
  else if(global_emulatepbf2osm) wformat= 12;
  else if(global_emulateosmosis) wformat= 13;
  else if(global_emulateosmium) wformat= 14;
  else if(global_outcsv) wformat= 21;
  else wformat= 11;
  refid= (int64_t*)oo__malloc(sizeof(int64_t)*global_maxrefs);
  refidee= refid+global_maxrefs;
  refxy= (int32_t**)oo__malloc(sizeof(int32_t*)*global_maxrefs);
  reftype= (byte*)oo__malloc(global_maxrefs);
  refrole= (char**)oo__malloc(sizeof(char*)*global_maxrefs);
  keyee= key+oo__keyvalM;
  diffcompare= false;
  diffdifference= false;

  // get input file format and care about tempfile name
  if(oo__getformat())
return 5;
  if((hashactive && !global_droprelations) ||
      global_calccoords!=0) {
      // (borders to apply AND relations are required) OR
      // user wants ways and relations to be converted to nodes
    // initiate recursive processing;
    if(global_complexways) {
      oo__dependencystage(11);
        // 11:     no output;
        //         for each node which is inside the borders,
        //           set flag in ht;
        //         store start of ways in read_setjump();
        //         for each way which has a member with flag in ht,
        //           set the way's flag in ht;
        //         for each relation with a member with flag in ht,
        //           store the relation's flag and write the ids
        //           of member ways which have no flag in ht
        //           (use cww_);
      if(cwn_ini(global_tempfilename))
return 28;
      if(cww_ini(global_tempfilename))
return 28;
      }
    else if(global_completeways) {
      oo__dependencystage(21);
        // 21:     no output;
        //         for each node inside the borders,
        //           set flag in ht;
        //         for each way with a member with a flag in ht,
        //           set the way's flag in ht and write the ids
        //           of all the way's members (use cwn_);
      if(cwn_ini(global_tempfilename))
return 28;
      }
    else
      oo__dependencystage(31);
        // 31:     for each node inside the borders,
        //           set flag in ht;
        //         for each way with a member with a flag in ht,
        //           set the way's flag in ht;
    strcpy(stpmcpy(o5mtempfile,global_tempfilename,
      sizeof(o5mtempfile)-2),".1");
    }
  else {
    oo__dependencystage(0);  // no recursive processing
    global_completeways= false;
    global_complexways= false;
    }

  // print file timestamp and nothing else if requested
  if(global_outtimestamp) {
    if(oo__timestamp!=0)  // file timestamp is valid
      write_timestamp(oo__timestamp);
    else
      write_str("(invalid timestamp)");
    write_str(NL);
return 0;  // nothing else to do here
    }

  // process the file
  for(;;) {  // read all input files

    if(oo_ifn>0) {  // at least one input file open

      // get next object - if .pbf
      //read_input(); (must not be here because of diffcompare)
      if(oo__ifp->format==-1) {
        if(!oo__alreadyhavepbfobject)
          pb_input(false);
        while(pb_type>2)  // unknown pbf object
          pb_input(false);  // get next object
        }

      // merging - if more than one file
      if((oo_ifn>1 || oo__tyidold>0) && dependencystage!=33)
          // input file switch necessary;
          // not:
          // 33:     write each relation which has a flag in ht
          //           to output;
        oo__switch();
      else if(global_mergeversions)
        oo__gettyid();
      else
        oo__ifp->tyid= 1;
      if(diffcompare && oo__ifp!=oo__if) {
          // comparison must be made with the first file but presently
          // the second file is active
        // switch to the first file
        oo__ifp= oo__if;
        read_switch(oo__ifp->ri);
        str_switch(oo__ifp->str);
        }

      // get next data
      read_input();

      }  // at least one input file open

    // care about end of input file
    if(oo_ifn==0 || (read_bufp>=read_bufe && oo__ifp->format>=0) ||
        (oo__ifp->format==-1 && pb_type<0)) {  // at end of input file
      if(oo_ifn>0) {
        if(oo__ifp->format==-1 && pb_type<0) {
          if(pb_type<-1)  // error
return 1000-pb_type;
          oo__ifp->endoffile= true;
          }
        oo__close();
        }
      if(oo_ifn>0)  // still input files
        oo__switch();
      else {  // at end of all input files
        // care about recursive processing
        if(dependencystage==11) {
            // 11:     no output;
            //         for each node which is inside the borders,
            //           set flag in ht;
            //         store start of ways in read_setjump();
            //         for each way which has a member with flag in ht,
            //           set the way's flag in ht;
            //         for each relation with a member with flag in ht,
            //           store the relation's flag and write the ids
            //           of member ways which have no flag in ht
            //           (use cww_);
          // 11->12: at all files' end:
          //         let all files jump to start of ways,
          //         use read_jump();
          //         set flags for ways, use cww_processing_set();
          if(oo__jumpall())
return 28;
          cww_processing_set();
          oo__dependencystage(12);
            // 12:     no output;
            //         for each way with a member with a flag in ht,
            //           set the way's flag in ht and write the ids
            //           of all the way's members (use cwn_);
  continue;  // do not write this object
          }
        if(dependencystage==21 || dependencystage==12) {
            // 12:     no output;
            //         for each way with a member with a flag in ht,
            //           set the way's flag in ht and write the ids
            //           of all the way's members (use cwn_);
            // 21:     no output;
            //         for each node inside the borders,
            //           set flag in ht;
            //         for each way with a member with a flag in ht,
            //           set the way's flag in ht and write the ids
            //           of all the way's members (use cwn_);
          // 12->22: as soon as first relation shall be written:
          //         rewind all files;
          //         set flags for nodes, use cwn_processing();
          // 21->22: as soon as first relation shall be written:
          //         rewind all files;
          //         set flags for nodes, use cwn_processing();
          if(oo__rewindall())
return 28;
          cwn_processing();
          oo__dependencystage(22);
            // 22:     write each node which has a flag in ht to output;
            //         write each way which has a flag in ht to output;
  continue;  // do not write this object
          }
        if(dependencystage!=32) {
            // not:
            // 32:     for each relation with a member with a flag
            //           in ht, set the relation's flag in ht;
            //         for each relation,
            //           write its id and its members' ids
            //           into a temporary file (use rr_);
            //         if option --all-to-nodes is set, then
            //           for each relation, write its members'
            //             geopositions into a temporary file
            //             (use posr_);
          if(dependencystage==33) {
              // 33:     write each relation which has a flag in ht
              //           to output; use temporary .o5m file as input;
            if(oo__ifp!=NULL)
              oo__ifp->endoffile= true;
                // this is because the file we have read
                // has been created as temporary file by the program
                // and does not contain an eof object;
            if(maxrewind_posr<maxrewind) maxrewind= maxrewind_posr;
            if(loglevel>0) fprintf(stderr,
              "Relation hierarchies: %i of maximal %i.\n",
              oo__maxrewindINI-maxrewind,oo__maxrewindINI);
            if(maxrewind<0)
              fprintf(stderr,
                "osmconvert Warning: relation dependencies too complex\n"
                "         (more than %i hierarchy levels).\n"
                "         A few relations might have been excluded\n"
                "         although lying within the borders.\n",
                oo__maxrewindINI);
            }
  break;
          }  // end   dependencystage!=32
        // here: dependencystage==32
        // 32:     for each relation with a member with a flag
        //           in ht, set the relation's flag in ht;
        //         for each relation,
        //           write its id and its members' ids
        //           into a temporary file (use rr_);
        //         if option --all-to-nodes is set, then
        //           for each relation, write its members'
        //             geopositions into a temporary file (use posr_);
        // 32->33: at all files' end:
        //         process all interrelation references (use rr_);
        //         if option --all-to-nodes is set, then
        //           process position array (use posr_);
        //         switch input to the temporary .o5m file;
        //         switch output to regular output file;
        if(!global_outnone) {
          wo_flush();
          wo_reset();
          wo_end();
          wo_flush();
          }
        if(write_newfile(NULL))
return 21;
        if(!global_outnone) {
          wo_format(wformat);
          wo_reset();
          }
        if(hashactive)
          oo__rrprocessing(&maxrewind);
        if(global_calccoords!=0)
          posr_processing(&maxrewind_posr,refxy);
        oo__dependencystage(33);  // enter next stage
        oo__tyidold= 0;  // allow the next object to be written
        if(oo_open(o5mtempfile))
return 22;
        if(oo__getformat())
return 23;
        read_input();
  continue;
        }  // at end of all input files
      }  // at end of input file

    // care about unexpected contents at file end
    if(dependencystage<=31)
        // 31:     for each node inside the borders,
        //           set flag in ht;
        //         for each way with a member with a flag in ht,
        //           set the way's flag in ht;
    if(oo__ifp->endoffile) {  // after logical end of file
      WARNv("osmconvert Warning: unexpected contents "
        "after logical end of file: %.80s",oo__ifp->filename)
  break;
      }

    readobject:
    bufp= read_bufp;
    b= *bufp; c= (char)b;

    // care about header and unknown objects
    if(oo__ifp->format<0) {  // -1, pbf
      if(pb_type<0 || pb_type>2)  // not a regular dataset id
  continue;
      otype= pb_type;
      oo__alreadyhavepbfobject= false;
      }  // end   pbf
    else if(oo__ifp->format==0) {  // o5m
      if(b<0x10 || b>0x12) {  // not a regular dataset id
        if(b>=0xf0) {  // single byte dataset
          if(b==0xff) {  // file start, resp. o5m reset
            if(read_setjump())
              oo__ifp->deleteobjectjump= oo__ifp->deleteobject;
            oo__reset(oo__ifp);
            }
          else if(b==0xfe)
            oo__ifp->endoffile= true;
          else if(write_testmode)
            WARNv("unknown .o5m short dataset id: 0x%02x",b)
          read_bufp++;
  continue;
          }  // end   single byte dataset
        else {  // unknown multibyte dataset
          if(b!=0xdb && b!=0xdc && b!=0xe0)
              // not border box AND not header
            WARNv("unknown .o5m dataset id: 0x%02x",b)
          read_bufp++;
          l= pbf_uint32(&read_bufp);  // length of this dataset
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
      if(c=='n' && bufsp[2]=='o' && bufsp[3]=='d')  // node
        otype= 0;
      else if(c=='w' && bufsp[2]=='a')  // way
        otype= 1;
      else if(c=='r' && bufsp[2]=='e')  // relation
        otype= 2;
      else if(c=='c' || (c=='m' && bufsp[2]=='o') || c=='d' ||
          c=='i' || c=='k' || c=='e' ) {
          // create, modify or delete,
          // insert, keep or erase
        if(c=='d' || c=='e')
          oo__ifp->deleteobject= 2;
        read_bufp= bufp+1;
  continue;
        }   // end   create, modify or delete,
            // resp. insert, keep or erase
      else if(c=='/') {  // xml end object
        if(bufsp[2]=='d' || bufsp[2]=='e')  // end of delete or erase
          oo__ifp->deleteobject= 0;
        else if(strzcmp(bufsp+2,"osm>")==0) {  // end of file
          oo__ifp->endoffile= true;
          read_bufp= bufp+6;
          while(oo__ws(*read_bufp)) read_bufp++;
  continue;
          }   // end   end of file
        else if(strzcmp(bufsp+2,"osmChange>")==0) {  // end of file
          oo__ifp->endoffile= true;
          read_bufp= bufp+6+6;
          while(oo__ws(*read_bufp)) read_bufp++;
  continue;
          }   // end   end of file
        else if(strzcmp(bufsp+2,"osmAugmentedDiff>")==0) {
            // end of file
          oo__ifp->endoffile= true;
          read_bufp= bufp+6+13;
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
      // here: regular OSM XML object
      if(read_setjump())
        oo__ifp->deleteobjectjump= oo__ifp->deleteobject;
      read_bufp= bufp;
      }  // end   xml

    // write header
    if(writeheader) {
      writeheader= false;
      if(!global_outnone)
        wo_start(wformat,oo__bbvalid,
          oo__bbx1,oo__bby1,oo__bbx2,oo__bby2,oo__timestamp);
      }

    // object initialization
    if(!diffcompare) {  // regularly read the object
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
      }  // regularly read the object
    if(oo__ifp->deleteobject==1) oo__ifp->deleteobject= 0;


    // read one osm object
    if(oo__ifp->format<0) {  // pbf
      // read id
      id= pb_id;
      // read coordinates (for nodes only)
      if(otype==0) {  // node
        lon= pb_lon; lat= pb_lat;
        }  // node
      // read author
      hisver= pb_hisver;
      if(hisver!=0) {  // author information available
        histime= pb_histime;
        if(histime!=0) {
          hiscset= pb_hiscset;
          hisuid= pb_hisuid;
          hisuser= pb_hisuser;
          }
        }  // end   author information available
      oo__ifp->deleteobject= pb_hisvis==0? 1: 0;
      // read noderefs (for ways only)
      if(otype==1)  // way
        refide= refid+pb_noderef(refid,global_maxrefs);
      // read refs (for relations only)
      else if(otype==2) {  // relation
        l= pb_ref(refid,reftype,refrole,global_maxrefs);
        refide= refid+l;
        reftypee= reftype+l;
        refrolee= refrole+l;
        }  // end   relation
      // read node key/val pairs
      l= pb_keyval(key,val,oo__keyvalM);
      keye= key+l; vale= val+l;
      }  // end   pbf
    else if(oo__ifp->format==0) {  // o5m
      bufp++;
      l= pbf_uint32(&bufp);
      read_bufp= bufe= bufp+l;
      if(diffcompare) {  // just compare, do not store the object
        uint32_t hisverc;
        int64_t histimec;
        char* hisuserc;
        int64_t* refidc;  // pointer for object contents comparison
        byte* reftypec;  // pointer for object contents comparison
        char** refrolec;  // pointer for object contents comparison
        char** keyc,**valc;  // pointer for object contents comparison

        // initialize comparison variables
        hisverc= 0;
        histimec= 0;
        hisuserc= "";
        refidc= refid;
        reftypec= reftype;
        refrolec= refrole;
        keyc= key;
        valc= val;

        // compare object id
        if(id!=(oo__ifp->o5id+= pbf_sint64(&bufp)))
          diffdifference= true;

        // compare author
        hisverc= pbf_uint32(&bufp);
        if(hisverc!=hisver)
          diffdifference= true;
        if(hisverc!=0) {  // author information available
          histimec= oo__ifp->o5histime+= pbf_sint64(&bufp);
          if(histimec!=0) {
            if(histimec!=histime)
              diffdifference= true;
            if(hiscset!=(oo__ifp->o5hiscset+= pbf_sint32(&bufp)))
              diffdifference= true;
            str_read(&bufp,&sp,&hisuserc);
            if(strcmp(hisuserc,hisuser)!=0)
              diffdifference= true;
            if(hisuid!=pbf_uint64((byte**)&sp))
              diffdifference= true;
            }
          }  // end   author information available

        if(bufp>=bufe) {
            // just the id and author, i.e. this is a delete request
          oo__ifp->deleteobject= 1;
          diffdifference= true;
          }
        else {  // not a delete request
          oo__ifp->deleteobject= 0;

          // compare coordinates (for nodes only)
          if(otype==0) {  // node
            // read node body
            if(lon!=(oo__ifp->o5lon+= pbf_sint32(&bufp)))
              diffdifference= true;
            if(lat!=(oo__ifp->o5lat+= pbf_sint32(&bufp)))
              diffdifference= true;
            }  // end   node

          // compare noderefs (for ways only)
          if(otype==1) {  // way
            l= pbf_uint32(&bufp);
            bp= bufp+l;
            if(bp>bufe) bp= bufe;  // (format error)
            while(bufp<bp && refidc<refidee) {
              if(*refidc!=(oo__ifp->o5rid[0]+= pbf_sint64(&bufp)))
                diffdifference= true;
              refidc++;
              }
            }  // end   way

          // compare refs (for relations only)
          else if(otype==2) {  // relation
            int64_t ri;  // temporary, refid
            int rt;  // temporary, reftype
            char* rr;  // temporary, refrole

            l= pbf_uint32(&bufp);
            bp= bufp+l;
            if(bp>bufe) bp= bufe;  // (format error)
            while(bufp<bp && refidc<refidee) {
              ri= pbf_sint64(&bufp);
              str_read(&bufp,&rr,NULL);
              if(*reftypec!=(rt= (*rr++ -'0')%3))
                diffdifference= true;
              if(*refidc!=(oo__ifp->o5rid[rt]+= ri))
                diffdifference= true;
              if(refrolec>=refrolee || strcmp(*refrolec,rr)!=0)
                diffdifference= true;
              reftypec++;
              refidc++;
              refrolec++;
              }
            }  // end   relation

          // compare node key/val pairs
          while(bufp<bufe && keyc<keyee) {
            char* k,*v;

            k= v= "";
            str_read(&bufp,&k,&v);
            if(keyc>=keye || strcmp(k,*keyc)!=0 || strcmp(v,*valc)!=0)
              diffdifference= true;
            keyc++; valc++;
            }
          }  // end   not a delete request

        // compare indexes
        if(keyc!=keye || (otype>0 && refidc!=refide))
          diffdifference= true;

        }  // just compare, do not store the object
      else {  // regularly read the object
        // read object id
        id= oo__ifp->o5id+= pbf_sint64(&bufp);
        // read author
        hisver= pbf_uint32(&bufp);
        if(hisver!=0) {  // author information available
          histime= oo__ifp->o5histime+= pbf_sint64(&bufp);
          if(histime!=0) {
            hiscset= oo__ifp->o5hiscset+= pbf_sint32(&bufp);
            str_read(&bufp,&sp,&hisuser);
            hisuid= pbf_uint64((byte**)&sp);
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
        }  // regularly read the object
      }  // end   o5m
    else {  // xml
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
          if(oo__xmlkey[0]=='i' && oo__xmlkey[1]=='d') // id
            id= oo__strtosint64(oo__xmlval);
          else if(oo__xmlkey[0]=='l') {  // letter l
            if(oo__xmlkey[1]=='o') // lon
              lon= oo__strtodeg(oo__xmlval);
            else if(oo__xmlkey[1]=='a') // lon
              lat= oo__strtodeg(oo__xmlval);
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
            if(oo__xmlkey[0]=='v' && oo__xmlkey[1]=='e') // hisver
              hisver= oo__strtouint32(oo__xmlval);
            if(!global_dropauthor) {  // author not to drop
              if(oo__xmlkey[0]=='t') // histime
                histime= oo__strtimetosint64(oo__xmlval);
              else if(oo__xmlkey[0]=='c') // hiscset
                hiscset= oo__strtosint64(oo__xmlval);
              else if(oo__xmlkey[0]=='u' && oo__xmlkey[1]=='i') // hisuid
                hisuid= oo__strtouint32(oo__xmlval);
              else if(oo__xmlkey[0]=='u' && oo__xmlkey[1]=='s') //hisuser
                hisuser= oo__xmlval;
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

    // care about multiple occurrences of one object within one file
    if(global_mergeversions) {
        // user allows duplicate objects in input file; that means
        // we must take the last object only of each duplicate
        // because this is assumed to be the newest;
      uint64_t tyidold;

      tyidold= oo__ifp->tyid;
      if(oo__gettyid()==0) {
        if(oo__ifp->tyid==tyidold)  // next object has same type and id
          goto readobject;  // dispose this object and take the next
        }
      oo__ifp->tyid= tyidold;
      }

    // care about possible array overflows
    if(refide>=refidee)
      PERRv("%s %"PRIi64" has too many refs.",ONAME(otype),id)
    if(keye>=keyee)
      PERRv("%s %"PRIi64" has too many key/val pairs.",
        ONAME(otype),id)

    // care about diffs and sequence
    if(global_diffcontents) {  // diff contents is to be considered
      // care about identical contents if calculating a diff
      if(oo__ifp!=oo__if && oo__ifp->tyid==oo__if->tyid) {
          // second file and there is a similar object in the first file
          // and version numbers are different
        diffcompare= true;  // compare with next object, do not read
        diffdifference= false;  // (default)
  continue;  // no check the first file
        }
      }  // diff contents is to be considered
    else {  // no diff contents is to be considered
      // stop processing if object is to ignore because of duplicates
      // in same or other file(s)
      if(oo__ifp->tyid<=oo__tyidold)
  continue;
      oo__tyidold= 0;
      if(oo_ifn>1)
        oo__tyidold= oo__ifp->tyid;
      // stop processing if in wrong stage for nodes or ways
      if(dependencystage>=32 && otype<=1)
          // 32:     for each relation with a member with a flag
          //           in ht, set the relation's flag in ht;
          //         for each relation,
          //           write its id and its members' ids
          //           into a temporary file (use rr_);
          //         if option --all-to-nodes is set, then
          //           for each relation, write its members'
          //             geopositions into a temporary file (use posr_);
          // 33:     write each relation which has a flag in ht
          //           to output; use temporary .o5m file as input;
  continue;  // ignore this object
      // check sequence, if necessary
      if(oo_ifn==1 && dependencystage!=33) {
          // not:
          // 33:     write each relation which has a flag in ht
          //           to output; use temporary .o5m file as input;
        if(otype<=oo_sequencetype &&
            (otype<oo_sequencetype || id<oo_sequenceid ||
            (oo_ifn>1 && id<=oo_sequenceid))) {
          oo__error= 92;
          WARNv("wrong sequence at %s %"PRIi64,
            ONAME(oo_sequencetype),oo_sequenceid)
          WARNv("next object is %s %"PRIi64,ONAME(otype),id)
          }
        }  // dependencystage>=32
      }  // no diff contents is to be considered
    oo_sequencetype= otype; oo_sequenceid= id;

    // care about calculating a diff file
    if(global_diff) {  // diff
      if(oo__ifp==oo__if) {  // first file has been chosen
        if(diffcompare) {
          diffcompare= false;
          if(!diffdifference)
  continue;  // there has not been a change in object's contents
          oo__ifp->deleteobject= 0;
          }
        else {
          //if(global_diffcontents && oo_ifn<2) continue;
          oo__ifp->deleteobject= 1;  // add "delete request";
          }
        }
      else {  // second file has been chosen
        if(oo__ifp->tyid==oo__if->tyid &&
            oo__ifp->hisver==oo__if->hisver)
            // there is a similar object in the first file
  continue;  // ignore this object
        }  // end   second file has been chosen
      }  // end   diff

    // care about dependency stages
    if(dependencystage==11) {
        // 11:     no output;
        //         for each node which is inside the borders,
        //           set flag in ht;
        //         store start of ways in read_setjump();
        //         for each way which has a member with flag in ht,
        //           set the way's flag in ht;
        //         for each relation with a member with flag in ht,
        //           store the relation's flag and write the ids
        //           of member ways which have no flag in ht
        //           (use cww_);
      if(otype>=1)  // way or relation
        read_lockjump();
      if((oo__ifp->deleteobject==0) ^ oo__ifp->subtract) {
          // object is not to delete
        if(otype==0) {  // node
          if(!border_active || border_queryinside(lon,lat))
              // no border to be applied OR node lies inside
            hash_seti(0,id);  // mark this node id as 'inside'
          }  // node
        else if(otype==1) {  // way
          refidp= refid;
          while(refidp<refide) {  // for every referenced node
            if(hash_geti(0,*refidp))
          break;
            refidp++;
            }  // end   for every referenced node
          if(refidp<refide)  // at least on node lies inside
            hash_seti(1,id);  // memorize that this way lies inside
          }  // way
        else {  // relation
          int64_t ri;  // temporary, refid
          int rt;  // temporary, reftype
          char* rr;  // temporary, refrole
          bool relinside;  // this relation lies inside
          bool wayinside;  // at least one way lies inside
          bool ismp;  // this relation is a multipolygon

          relinside= wayinside= ismp= false;
          refidp= refid; reftypep= reftype; refrolep= refrole;
          while(refidp<refide) {  // for every referenced object
            ri= *refidp; rt= *reftypep; rr= *refrolep;
            if(!relinside && hash_geti(rt,ri))
              relinside= true;
            if(!wayinside && rt==1 && (strcmp(rr,"outer")==0 ||
                strcmp(rr,"inner")==0) && hash_geti(1,ri))
                // referenced object is a way and part of
                // a multipolygon AND lies inside
              wayinside= true;
            refidp++; reftypep++; refrolep++;
            }  // end   for every referenced object
          if(relinside) {  // relation lies inside
            hash_seti(2,id);
            if(wayinside) {  // at least one way lies inside
              keyp= key; valp= val;
              while(keyp<keye) {  // for all key/val pairs of this object
                if(strcmp(*keyp,"type")==0 &&
                    strcmp(*valp,"multipolygon")==0) {
                  ismp= true;
              break;
                  }
                keyp++; valp++;
                }  // for all key/val pairs of this object
              if(ismp) {  // is multipolygon
                refidp= refid; reftypep= reftype; refrolep= refrole;
                while(refidp<refide) {  // for every referenced object
                  ri= *refidp; rt= *reftypep; rr= *refrolep;
                  if(rt==1 && (strcmp(rr,"outer")==0 ||
                      strcmp(rr,"inner")==0) &&
                      !hash_geti(1,ri)) {  // referenced object
                      // is a way and part of the multipolygon AND
                      // has not yet a flag in ht
                    cww_ref(ri);  // write id of the way
                    }
                  refidp++; reftypep++; refrolep++;
                  }  // end   for every referenced object
                }  // is multipolygon
              }  // at least one way lies inside
            }  // relation lies inside
          }  // relation
        }  // object is not to delete
continue;  // do not write this object
      }  // dependencystage 11
    else if(dependencystage==12) {
      // 12:     no output;
      //         for each way with a member with a flag in ht,
      //           set the way's flag in ht and write the ids
      //           of all the way's members (use cwn_);
      if((oo__ifp->deleteobject==0) ^ oo__ifp->subtract) {
          // object is not to delete
        if(otype==1 && hash_geti(1,id)) {
            // way AND is marked in ht
          // store ids of all referenced nodes of this way
          refidp= refid;
          while(refidp<refide) {  // for every referenced node
            cwn_ref(*refidp);
            refidp++;
            }  // end   for every referenced node
          }  // way
        }  // object is not to delete
continue;  // do not write this object
      }  // dependencystage 12
    else if(dependencystage==21) {
        // 21:     no output;
        //         for each node inside the borders,
        //           set flag in ht;
        //         for each way with a member with a flag in ht,
        //           set the way's flag in ht and write the ids
        //           of all the way's members (use cwn_);
      if((oo__ifp->deleteobject==0) ^ oo__ifp->subtract) {
          // object is not to delete
        if(otype==0) {  // node
          if(!border_active || border_queryinside(lon,lat))
              // no border to be applied OR node lies inside
            hash_seti(0,id);  // mark this node id as 'inside'
          }  // node
        else if(otype==1) {  // way
          refidp= refid;
          while(refidp<refide) {  // for every referenced node
            if(hash_geti(0,*refidp))
          break;
            refidp++;
            }  // end   for every referenced node
          if(refidp<refide) {  // at least on node lies inside
            hash_seti(1,id);  // memorize that this way lies inside
            // store ids of all referenced nodes of this way
            refidp= refid;
            while(refidp<refide) {  // for every referenced node
              cwn_ref(*refidp);
              refidp++;
              }  // end   for every referenced node
            }  // at least on node lies inside
          }  // way
        else {  // relation
          oo__ifp->endoffile= true;  // suppress warnings
          oo__close();  // the next stage will be entered as soon as
            // all files have been closed;
            // 21->22: as soon as first relation shall be written:
            //         rewind all files;
            //         set flags for nodes, use cwn_processing();
          }  // relation
        }  // object is not to delete
continue;  // do not write this object
      }  // dependencystage 21
    else if(otype==2) {  // relation
      if(!global_droprelations &&
          (dependencystage==31 || dependencystage==22)) {
          // not relations to drop AND
          // 22:     write each node which has a flag in ht to output;
          //         write each way which has a flag in ht to output;
          // 31:     for each node inside the borders,
          //           set flag in ht;
          //         for each way with a member with a flag in ht,
          //           set the way's flag in ht;
        // 22->32: as soon as first relation shall be written:
        //         clear flags for ways, use cww_processing_clear();
        //         switch output to temporary file;
        // 31->32: as soon as first relation shall be written:
        //         switch output to temporary .o5m file;
        wo_flush();
        if(write_newfile(o5mtempfile))
return 24;
        wo_format(0);
        if(hashactive)
          if(rr_ini(global_tempfilename))
return 25;
        if(dependencystage==22)
          cww_processing_clear();
        if(global_calccoords!=0)
          if(posr_ini(global_tempfilename))
return 26;
        oo__dependencystage(32);
          // 32:     for each relation with a member with a flag
          //           in ht, set the relation's flag in ht;
          //         for each relation,
          //           write its id and its members' ids
          //           into a temporary file (use rr_);
          //         if option --all-to-nodes is set, then
          //           for each relation, write its members'
          //             geopositions into a temporary file (use posr_);
        }  // dependencystage was 31
      }  // relation
    else {  // node or way
      }  // node or way
    // end   care about dependency stages

    // process object deletion
    if((oo__ifp->deleteobject!=0) ^ oo__ifp->subtract) {
        // object is to delete
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

    // care about object statistics
    if(global_statistics &&
        dependencystage!=32) {
        // not:
        // 32:     for each relation with a member with a flag
        //           in ht, set the relation's flag in ht;
        //         for each relation,
        //           write its id and its members' ids
        //           into a temporary file (use rr_);
        //         if option --all-to-nodes is set, then
        //           for each relation, write its members'
        //             geopositions into a temporary file (use posr_);

      if(otype==0) {  // node
        if(statistics.nodes==0) {  // this is the first node
          statistics.lon_min= statistics.lon_max= lon;
          statistics.lat_min= statistics.lat_max= lat;
          }
        statistics.nodes++;
        if(statistics.node_id_min==0 || id<statistics.node_id_min)
          statistics.node_id_min= id;
        if(statistics.node_id_max==0 || id>statistics.node_id_max)
          statistics.node_id_max= id;
        if(lon<statistics.lon_min)
          statistics.lon_min= lon;
        if(lon>statistics.lon_max)
          statistics.lon_max= lon;
        if(lat<statistics.lat_min)
          statistics.lat_min= lat;
        if(lat>statistics.lat_max)
          statistics.lat_max= lat;
        }
      else if(otype==1) {  // way
        statistics.ways++;
        if(statistics.way_id_min==0 || id<statistics.way_id_min)
          statistics.way_id_min= id;
        if(statistics.way_id_max==0 || id>statistics.way_id_max)
          statistics.way_id_max= id;
        if(refide-refid>statistics.noderefs_max) {
          statistics.noderefs_oid= id;
          statistics.noderefs_max= refide-refid;
          }
        }
      else if(otype==2) {  // relation
        statistics.relations++;
        if(statistics.relation_id_min==0 ||
            id<statistics.relation_id_min)
          statistics.relation_id_min= id;
        if(statistics.relation_id_max==0 ||
            id>statistics.relation_id_max)
          statistics.relation_id_max= id;
        if(refide-refid>statistics.relrefs_max) {
          statistics.relrefs_oid= id;
          statistics.relrefs_max= refide-refid;
          }
        }
      if(histime!=0) {  // timestamp valid
        if(statistics.timestamp_min==0 ||
            histime<statistics.timestamp_min)
          statistics.timestamp_min= histime;
        if(statistics.timestamp_max==0 ||
            histime>statistics.timestamp_max)
          statistics.timestamp_max= histime;
        }
      if(keye-key>statistics.keyval_pairs_max) {
        statistics.keyval_pairs_otype= otype;
        statistics.keyval_pairs_oid= id;
        statistics.keyval_pairs_max= keye-key;
        }
      }  // object statistics

    // abort writing if user does not want any standard output
    if(global_outnone)
  continue;

    // write the object
    if(otype==0) {  // write node
      bool inside;  // node lies inside borders, if appl.

      if(!border_active)  // no borders shall be applied
        inside= true;
      else if(dependencystage==22)
          // 22:     write each node which has a flag in ht to output;
          //         write each way which has a flag in ht to output;
        inside= hash_geti(0,id);
      else {
        inside= border_queryinside(lon,lat);  // node lies inside
        if(inside)
          hash_seti(0,id);  // mark this node id as 'inside'
        }
      if(inside) {  // node lies inside
        if(global_calccoords!=0) {
          // check id range
          if(id>=global_otypeoffset05 || id<=-global_otypeoffset05)
            WARNv("node id %"PRIi64
              " out of range. Increase --object-type-offset",id)
          posi_set(id,lon,lat);  // store position
          }
        if(!global_dropnodes) {  // not to drop
          wo_node(id,
            hisver,histime,hiscset,hisuid,hisuser,lon,lat);
          keyp= key; valp= val;
          while(keyp<keye)  // for all key/val pairs of this object
            wo_node_keyval(*keyp++,*valp++);
          wo_node_close();
          }  // end   not to drop
        }  // end   node lies inside
      }  // write node
    else if(otype==1) {  // write way
      bool inside;  // way lies inside borders, if appl.

      if(!hashactive)  // no borders shall be applied
        inside= true;
      else if(dependencystage==22)
          // 22:     write each node which has a flag in ht to output;
          //         write each way which has a flag in ht to output;
        inside= hash_geti(1,id);
      else {  // borders are to be applied
        inside= false;  // (default)
        refidp= refid;
        while(refidp<refide) {  // for every referenced node
          if(hash_geti(0,*refidp)) {
            inside= true;
        break;
            }
          refidp++;
          }  // end   for every referenced node
        }  // end   borders are to be applied
      if(inside) {  // no borders OR at least one node inside
        if(hashactive)
          hash_seti(1,id);  // memorize that this way lies inside
        if(!global_dropways) {  // not ways to drop
          if(global_calccoords!=0) {
              // coordinates of ways shall be calculated
            int32_t x_min,x_max,y_min,y_max;
            int32_t x_middle,y_middle,xy_distance,new_distance;
            bool is_area;
            int n;  // number of referenced nodes with coordinates

            // check id range
            if(id>=global_otypeoffset05 || id<=-global_otypeoffset05)
              WARNv("way id %"PRIi64
                " out of range. Increase --object-type-offset",id)

            // determine the center of the way's bbox
            n= 0;
            refidp= refid; refxyp= refxy;
            while(refidp<refide) {  // for every referenced node
              *refxyp= NULL;  // (default)
              if(!global_dropbrokenrefs || hash_geti(0,*refidp)) {
                  // referenced node lies inside the borders
                posi_get(*refidp);  // get referenced node's coordinates
                *refxyp= posi_xy;
                if(posi_xy!=NULL) {  // coordinate is valid
                  if(n==0) {  // first coordinate
                    // just store it as min and max
                    x_min= x_max= posi_xy[0];
                    y_min= y_max= posi_xy[1];
                    }
                  else {  // additional coordinate
                    // adjust extrema
                    if(posi_xy[0]<x_min && x_min-posi_xy[0]<900000000)
                      x_min= posi_xy[0];
                    else if(posi_xy[0]>
                        x_max && posi_xy[0]-x_max<900000000)
                      x_max= posi_xy[0];
                    if(posi_xy[1]<y_min)
                      y_min= posi_xy[1];
                    else if(posi_xy[1]>y_max)
                      y_max= posi_xy[1];
                    }
                  n++;
                  }  // coordinate is valid
                }  // referenced node lies inside the borders
              refidp++; refxyp++;
              }  // end   for every referenced node

            // determine if the way is an area
            is_area= refide!=refid && refide[-1]==refid[0];
                // first node is the same as the last one

            // determine the valid center of the way
            x_middle= x_max/2+x_min/2;
            y_middle= (y_max+y_min)/2;
            if(is_area) {
              lon= x_middle;
              lat= y_middle;
              }
            else {  // the way is not an area
            // determine the node which has the smallest distance
            // to the center of the bbox
              n= 0;
              refidp= refid; refxyp= refxy;
              while(refidp<refide) {  // for every referenced node
                posi_xy= *refxyp;
                if(posi_xy!=NULL) {
                    // there is a coordinate for this reference
                  if(n==0) {  // first coordinate
                    // just store it as min and max
                    lon= posi_xy[0];
                    lat= posi_xy[1];
                    xy_distance= abs(lon-x_middle)+abs(lat-y_middle);
                    }
                  else {  // additional coordinate
                    new_distance= abs(posi_xy[0]-x_middle)+
                      abs(posi_xy[1]-y_middle);
                    if(new_distance<xy_distance) {
                      lon= posi_xy[0];
                      lat= posi_xy[1];
                      xy_distance= new_distance;
                      }
                    }  // additional coordinate
                  n++;
                  }  // there is a coordinate for this reference
                refidp++; refxyp++;
              //break; //<- uncomment to use the first node of each way
                }  // end   for every referenced node
              }  // the way is not an area
            if(global_calccoords>0)
              posi_set(id+global_otypeoffset10,lon,lat);
            else
              posi_setbbox(id+global_otypeoffset10,lon,lat,
                x_min,y_min,x_max,y_max);
            if(global_alltonodes) {  // convert all objects to nodes
              // write a node as a replacement for the way
              if(n>0) {  // there is at least one coordinate available
                int64_t id_new;

                if(global_otypeoffsetstep!=0)
                  id_new= global_otypeoffsetstep++;
                else
                  id_new= id+global_otypeoffset10;
                wo_node(id_new,
                  hisver,histime,hiscset,hisuid,hisuser,lon,lat);
                if(global_add)
                  wo_addbboxtags(true,x_min,y_min,x_max,y_max);
                keyp= key; valp= val;
                while(keyp<keye)  // for all key/val pairs of this object
                  wo_node_keyval(*keyp++,*valp++);
                wo_node_close();
                }  // there is at least one coordinate available
              }  // convert all objects to nodes
            else {  // objects are not to be converted to nodes
              wo_way(id,hisver,histime,hiscset,hisuid,hisuser);
              refidp= refid;
              while(refidp<refide) {  // for every referenced node
                if(!global_dropbrokenrefs || hash_geti(0,*refidp))
                    // referenced node lies inside the borders
                  wo_noderef(*refidp);
                refidp++;
                }  // end   for every referenced node
              if(global_add)
                wo_addbboxtags(false,x_min,y_min,x_max,y_max);
              keyp= key; valp= val;
              while(keyp<keye)  // for all key/val pairs of this object
                wo_wayrel_keyval(*keyp++,*valp++);
              wo_way_close();
              }  // objects are not to be converted to nodes
            }  // coordinates of ways shall be calculated
          else  {  // coordinates of ways need not to be calculated
            wo_way(id,hisver,histime,hiscset,hisuid,hisuser);
            refidp= refid;
            while(refidp<refide) {  // for every referenced node
              if(!global_dropbrokenrefs || hash_geti(0,*refidp))
                  // referenced node lies inside the borders
                wo_noderef(*refidp);
              refidp++;
              }  // end   for every referenced node
            keyp= key; valp= val;
            while(keyp<keye)  // for all key/val pairs of this object
              wo_wayrel_keyval(*keyp++,*valp++);
            wo_way_close();
            }  // coordinates of ways need not to be calculated
          }  // end   not ways to drop
        }  // end   no border OR at least one node inside
      }  // write way
    else if(otype==2) {  // write relation
      if(!global_droprelations) {  // not relations to drop
        bool inside;  // relation may lie inside borders, if appl.
        bool in;  // relation lies inside borders
        int64_t ri;  // temporary, refid
        int rt;  // temporary, reftype
        char* rr;  // temporary, refrole

        in= hash_geti(2,id);
        if(dependencystage==32) {
          // 32:     for each relation with a member with a flag
          //           in ht, set the relation's flag in ht;
          //         for each relation,
          //           write its id and its members' ids
          //           into a temporary file (use rr_);
          //         if option --all-to-nodes is set, then
          //           for each relation, write its members'
          //             geopositions into a temporary file (use posr_);
          bool has_highway,has_area;  // relation has certain tags
          bool is_area;  // the relation is assumed to represent an area
          bool idwritten,posridwritten;

          // determine if this relation is assumed to represent
          // an area or not
          has_highway= has_area= false;
          keyp= key; valp= val;
          while(keyp<keye) {  // for all key/val pairs of this object
            if(strcmp(*keyp,"highway")==0 ||
                strcmp(*keyp,"waterway")==0 ||
                strcmp(*keyp,"railway")==0 ||
                strcmp(*keyp,"aerialway")==0 ||
                strcmp(*keyp,"power")==0 ||
                strcmp(*keyp,"route")==0
                )
              has_highway= true;
            else if(strcmp(*keyp,"area")==0 &&
                strcmp(*valp,"yes")==0)
              has_area= true;
            keyp++,valp++;
            }
          is_area= !has_highway || has_area;

          // write the id of this relation and its members
          // to a temporary file
          idwritten= posridwritten= false;
          refidp= refid; reftypep= reftype;
          while(refidp<refide) {  // for every referenced object
            ri= *refidp;
            rt= *reftypep;
            if(hashactive) {
              if(rt==2) {  // referenced object is a relation
                if(!idwritten) {  // did not yet write our relation's id
                  rr_rel(id);  // write it now
                  idwritten= true;
                  }
                rr_ref(ri);
                }
              }
            if(global_calccoords!=0) {
              if(!posridwritten) {
                  // did not yet write our relation's id
                // write it now
                posr_rel(id,is_area);
                posi_set(id+global_otypeoffset20,posi_nil,0);
                  // reserve space for this relation's coordinates
                posridwritten= true;
                }
              if(rt==1)  // way
                ri+= global_otypeoffset10;
              else if(rt==2)  // relation
                ri+= global_otypeoffset20;
              posr_ref(ri);
              }
            refidp++; reftypep++;
            }  // end   for every referenced object
          inside= true;
          }
        else if(dependencystage==33) {
            // 33:     write each relation which has a flag in ht
            //           to output; use temporary .o5m file as input;
          inside= in;
          }
        else
          inside= true;
        if(inside) {  // no borders OR at least one node inside
          if(global_alltonodes && dependencystage==33) {
              // all relations are to be converted to nodes AND
              // 33:     write each relation which has a flag in ht
              //           to output; use temporary .o5m file as input;
            if(id>=global_otypeoffset05 || id<=-global_otypeoffset05)
              WARNv("relation id %"PRIi64
                " out of range. Increase --object-type-offset",id)
            posi_get(id+global_otypeoffset20);  // get coordinates
            if(posi_xy!=NULL && posi_xy[0]!=posi_nil) {
                // stored coordinates are valid
              int64_t id_new;

              if(global_otypeoffsetstep!=0)
                id_new= global_otypeoffsetstep++;
              else
                id_new= id+global_otypeoffset20;
              // write a node as a replacement for the relation
              wo_node(id_new,
                hisver,histime,hiscset,hisuid,hisuser,
                posi_xy[0],posi_xy[1]);
              if(global_add)
                wo_addbboxtags(true,
                  posi_xy[2],posi_xy[3],posi_xy[4],posi_xy[5]);
              keyp= key; valp= val;
              while(keyp<keye)  // for all key/val pairs of this object
                wo_node_keyval(*keyp++,*valp++);
              wo_node_close();
              }  // stored coordinates are valid
            }  // relations are to be converted to nodes
          else {  // dependencystage!=33 OR not --all-to-nodes
            wo_relation(id,hisver,histime,hiscset,hisuid,hisuser);
            refidp= refid; reftypep= reftype; refrolep= refrole;
            while(refidp<refide) {  // for every referenced object
              ri= *refidp;
              rt= *reftypep;
              rr= *refrolep;

              if(dependencystage<33) {
                  // not:
                  // 33:     write each relation which has a flag in ht
                  //           to output;
                  //         use temporary .o5m file as input;
                if(rt==2 || hash_geti(rt,ri)) {
                    // referenced object is a relation OR
                    // lies inside the borders
                  wo_ref(ri,rt,rr);
                  if(rt!=2 && !in) {
                    hash_seti(2,id); in= true; }
                  }
                else {  // referenced object lies outside the borders
                  if(!global_dropbrokenrefs) {
                    wo_ref(ri,rt,rr);
                    }
                  }
                }
              else {  // dependencystage==33
                // 33:     write each relation which has a flag in ht
                //           to output;
                //         use temporary .o5m file as input;
                if(!global_dropbrokenrefs || hash_geti(rt,ri)) {
                    // broken refs are to be listed anyway OR
                    // referenced object lies inside the borders
                  wo_ref(ri,rt,rr);
                  }
                }
              refidp++; reftypep++; refrolep++;
              }  // end   for every referenced object
            if(global_add) {
              posi_get(id+global_otypeoffset20);  // get coordinates
              if(posi_xy!=NULL && posi_xy[0]!=posi_nil)
                  // stored coordinates are valid
                wo_addbboxtags(false,
                  posi_xy[2],posi_xy[3],posi_xy[4],posi_xy[5]);
              }
            keyp= key; valp= val;
            while(keyp<keye)  // for all key/val pairs of this object
              wo_wayrel_keyval(*keyp++,*valp++);
            wo_relation_close();
            }  // stage!=3 OR not --all-to-nodes
          }  // end   no borders OR at least one node inside
        }  // end   not relations to drop
      }  // write relation
    }  // end   read all input files
  if(!global_outnone) {
    if(writeheader)
      wo_start(wformat,oo__bbvalid,
        oo__bbx1,oo__bby1,oo__bbx2,oo__bby2,oo__timestamp);
    wo_end();
    }
  if(global_statistics) {  // print statistics
    FILE* fi;
    if(global_outstatistics) fi= stdout;
    else fi= stderr;

    if(statistics.timestamp_min!=0) {
      char timestamp[30];

      write_createtimestamp(statistics.timestamp_min,timestamp);
      fprintf(fi,"timestamp min: %s\n",timestamp);
      }
    if(statistics.timestamp_max!=0) {
      char timestamp[30];

      write_createtimestamp(statistics.timestamp_max,timestamp);
      fprintf(fi,"timestamp max: %s\n",timestamp);
      }
    if(statistics.nodes>0) {  // at least one node
      char coord[20];

      write_createsfix7o(statistics.lon_min,coord);
      fprintf(fi,"lon min: %s\n",coord);
      write_createsfix7o(statistics.lon_max,coord);
      fprintf(fi,"lon max: %s\n",coord);
      write_createsfix7o(statistics.lat_min,coord);
      fprintf(fi,"lat min: %s\n",coord);
      write_createsfix7o(statistics.lat_max,coord);
      fprintf(fi,"lat max: %s\n",coord);
      }
    fprintf(fi,"nodes: %"PRIi64"\n",statistics.nodes);
    fprintf(fi,"ways: %"PRIi64"\n",statistics.ways);
    fprintf(fi,"relations: %"PRIi64"\n",statistics.relations);
    if(statistics.node_id_min!=0)
      fprintf(fi,"node id min: %"PRIi64"\n",statistics.node_id_min);
    if(statistics.node_id_max!=0)
      fprintf(fi,"node id max: %"PRIi64"\n",statistics.node_id_max);
    if(statistics.way_id_min!=0)
      fprintf(fi,"way id min: %"PRIi64"\n",statistics.way_id_min);
    if(statistics.way_id_max!=0)
      fprintf(fi,"way id max: %"PRIi64"\n",statistics.way_id_max);
    if(statistics.relation_id_min!=0)
      fprintf(fi,"relation id min: %"PRIi64"\n",
        statistics.relation_id_min);
    if(statistics.relation_id_max!=0)
      fprintf(fi,"relation id max: %"PRIi64"\n",
        statistics.relation_id_max);
    if(statistics.keyval_pairs_max!=0) {
      fprintf(fi,"keyval pairs max: %"PRIi32"\n",
        statistics.keyval_pairs_max);
      fprintf(fi,"keyval pairs max object: %s %"PRIi64"\n",
        ONAME(statistics.keyval_pairs_otype),
        statistics.keyval_pairs_oid);
      }
    if(statistics.noderefs_max!=0) {
      fprintf(fi,"noderefs max: %"PRIi32"\n",
        statistics.noderefs_max);
      fprintf(fi,"noderefs max object: way %"PRIi64"\n",
        statistics.noderefs_oid);
      }
    if(statistics.relrefs_max!=0) {
      fprintf(fi,"relrefs max: %"PRIi32"\n",
        statistics.relrefs_max);
      fprintf(fi,"relrefs max object: relation %"PRIi64"\n",
        statistics.relrefs_oid);
      }
    }  // print statistics
  return oo__error;
  }  // end   oo_main()

//------------------------------------------------------------
// end   Module oo_   osm to osm module
//------------------------------------------------------------



static void assistant_end();

static bool assistant(int* argcp,char*** argvp) {
  // interactively guide the user through basic functions;
  // argcp==NULL AND argvp==NULL: to confirm that the calculation
  //                              has been terminated correctly;
  // argcp==NULL AND argvp!=NULL:
  // display 'bye message', do nothing else (third call);
  // usually, this procedure must be called twice: first, before
  // parsing the command line arguments, and second, after
  // the regular processing has been done without any error;
  // the third call will be done by atexit();
  // return: user wants to terminate the program;
  #define langM 2
  static int lang= 0;
  static const char* talk_lang1[langM]= { "", "de_" };
  static const char* talk_lang2[langM]= { "", "German_" };
  static const char* talk_section[langM]= {
    "-----------------------------------------------------------------\n"
    };
  static const char* talk_intro[langM]= {
    "\n"
    "osmconvert "VERSION"\n"
    "\n"
    "Converts .osm, .o5m, .pbf, .osc, .osh files, applies changes\n"
    "of .osc, .o5c, .osh files and sets limiting borders.\n"
    "Use command line option -h to get a parameter overview,\n"
    "or --help to get detailed help.\n"
    "\n"
    "If you are familiar with the command line, press <Return>.\n"
    "\n"
    "If you do not know how to operate the command line, please\n"
    "enter \"a\" (press key E and hit <Return>).\n"
    ,
    "\n"
    "osmconvert "VERSION"\n"
    "\n"
    "Konvertiert .osm-, .o5m-, .pbf-, .osc- und .osh-Dateien,\n"
    "spielt Updates von .osc-, .o5c- und .osh-Dateien ein und\n"
    "setzt geografische Grenzen.\n"
    "Die Kommandozeilenoption -h zeigt eine Parameteruebersicht,\n"
    "--help bringt eine detaillierte Hilfe (in Englisch).\n"
    "\n"
    "Wenn Sie mit der Kommandozeile vertraut sind, druecken Sie\n"
    "bitte <Return>.\n"
    "\n"
    "Falls Sie sich mit der Kommandozeile nicht auskennen, druecken\n"
    "Sie bitte \"a\" (Taste A und dann die Eingabetaste).\n"
    };
  static const char* talk_hello[langM]= {
    "Hi, I am osmconBert - just call me Bert.\n"
    "I will guide you through the basic functions of osmconvert.\n"
    "\n"
    "At first, please ensure to have the \"osmconvert\" file\n"
    "(resp. \"osmconvert.exe\" file if Windows) located in the\n"
    "same directory in which all your OSM data is stored.\n"
    "\n"
    "You may exit this program whenever you like. Just hold\n"
    "the <Ctrl> key and press the key C.\n"
    "\n"
    ,
    "Hallo, ich bin osmconBert - nennen Sie mich einfach Bert.\n"
    "Ich werde Sie durch die Standardfunktionen von osmconvert leiten.\n"
    "\n"
    "Bitte stellen Sie zuerst sicher, dass sich die Programmdatei\n"
    "\"osmconvert\" (bzw. \"osmconvert.exe\" im Fall von Windows) im\n"
    "gleichen Verzeichnis befindet wie Ihre OSM-Dateien.\n"
    "\n"
    "Sie koennen das Programm jederzeit beenden. Halten Sie dazu die\n"
    "<Strg>-Taste gedrueckt und druecken die Taste C.\n"
    "\n"
    };
  static const char* talk_input_file[langM]= {
    "Please please tell me the name of the file you want to process:\n"
    ,
    "Bitte nennen Sie mir den Namen der Datei, die verarbeitet werden soll:\n"
    };
  static const char* talk_not_found[langM]= {
    "Sorry, I cannot find a file with this name in the current directory.\n"
    "\n"
    ,
    "Sorry, ich kann diese Datei im aktuellen Verzeichnis nicht finden.\n"
    "\n"
    };
  static const char* talk_input_file_suffix[langM]= {
    "Sorry, the file must have \".osm\", \".o5m\" or \".pbf\" as suffix.\n"
    "\n"
    ,
    "Sorry, die Datei muss \".osm\", \".o5m\" oder \".pbf\" als Endung haben.\n"
    "\n"
    };
  static const char* talk_thanks[langM]= {
    "Thanks!\n"
    ,
    "Danke!\n"
    };
  static const char* talk_function[langM]= {
    "What may I do with this file?\n"
    "\n"
    "  1  convert it to a different file format\n"
    "  2  use an OSM Changefile to update this file\n"
    "  3  use a border box to limit the geographical region\n"
    "  4  use a border polygon file to limit the geographical region\n"
    "  5  minimize file size by deleting author information\n"
    "  6  display statistics of the file\n"
    "To options 3 or 4 you may also choose:\n"
    "  a  keep ways complete, even if they cross the border\n"
    "  b  keep ways and areas complete, even if they cross the border\n"
    "\n"
    "Please enter the number of one or more functions you choose:\n"
    ,
    "Was soll ich mit dieser Datei tun?\n"
    "\n"
    "  1  in ein anderes Format umwandeln\n"
    "  2  sie per OSM-Change-Datei aktualisieren\n"
    "  3  per Laengen- und Breitengrad einen Bereich ausschneiden\n"
    "  4  mit einer Polygon-Datei einen Bereich ausschneiden\n"
    "  5  Autorinformationen loeschen,damit Dateigroesse minimieren\n"
    "  6  statistische Daten zu dieser Datei anzeigen\n"
    "Zu den Optionen 3 oder 4 koennen zusaetzlich gewaehlt werden:\n"
    "  a  grenzueberschreitende Wege als Ganzes behalten\n"
    "  b  grenzueberschreitende Wege und Flaechen als Ganzes behalten\n"
    "\n"
    "Bitte waehlen Sie die Nummer(n) von einer oder mehreren Funktionen:\n"
    };
  static const char* talk_all_right[langM]= {
    "All right.\n"
    ,
    "Geht in Ordnung.\n"
    };
  static const char* talk_cannot_understand[langM]= {
    "Sorry, I could not understand.\n"
    "\n"
    ,
    "Sorry, das habe ich nicht verstanden.\n"
    "\n"
    };
  static const char* talk_two_borders[langM]= {
    "Please do not choose both, border box and border polygon.\n"
    "\n"
    ,
    "Bitte nicht beide Arten des Ausschneidens gleichzeitig waehlen.\n"
    "\n"
    };
  static const char* talk_changefile[langM]= {
    "Please tell me the name of the OSM Changefile:\n"
    ,
    "Bitte nennen Sie mir den Namen der OSM-Change-Datei:\n"
    };
  static const char* talk_changefile_suffix[langM]= {
    "Sorry, the Changefile must have \".osc\" or \".o5c\" as suffix.\n"
    "\n"
    ,
    "Sorry, die Change-Datei muss \".osc\" oder \".o5c\" als Endung haben.\n"
    "\n"
    };
  static const char* talk_polygon_file[langM]= {
    "Please tell me the name of the polygon file:\n"
    ,
    "Bitte nennen Sie mir den Namen der Polygon-Datei:\n"
    };
  static const char* talk_polygon_file_suffix[langM]= {
    "Sorry, the polygon file must have \".poly\" as suffix.\n"
    "\n"
    ,
    "Sorry, die Polygon-Datei muss \".poly\" als Endung haben.\n"
    "\n"
    };
  static const char* talk_coordinates[langM]= {
    "We need the coordinates of the border box.\n"
    "The unit is degree, just enter each number, e.g.: -35.75\n"
    ,
    "Wir brauchen die Bereichs-Koordinaten in Grad,\n"
    "aber jeweils ohne Einheitenbezeichnung, also z.B.: 7,75\n"
    };
  static const char* talk_minlon[langM]= {
    "Please tell me the minimum longitude:\n"
    ,
    "Bitte nennen Sie mir den Minimum-Laengengrad:\n"
    };
  static const char* talk_maxlon[langM]= {
    "Please tell me the maximum longitude:\n"
    ,
    "Bitte nennen Sie mir den Maximum-Laengengrad:\n"
    };
  static const char* talk_minlat[langM]= {
    "Please tell me the minimum latitude:\n"
    ,
    "Bitte nennen Sie mir den Minimum-Breitengrad:\n"
    };
  static const char* talk_maxlat[langM]= {
    "Please tell me the maximum latitude:\n"
    ,
    "Bitte nennen Sie mir den Maximum-Breitengrad:\n"
    };
  static const char* talk_output_format[langM]= {
    "Please choose the output file format:\n"
    "\n"
    "1 .osm (standard XML format - results in very large files)\n"
    "2 .o5m (binary format - allows fast)\n"
    "3 .pbf (standard binary format - results in small files)\n"
    "\n"
    "Enter 1, 2 or 3:\n"
    ,
    "Bitte waehlen Sie das Format der Ausgabe-Datei:\n"
    "\n"
    "1 .osm (Standard-XML-Format - ergibt sehr grosse Dateien)\n"
    "2 .o5m (binaeres Format - recht schnell)\n"
    "3 .pbf (binaeres Standard-Format - ergibt kleine Dateien)\n"
    "\n"
    "1, 2 oder 3 eingeben:\n"
    };
  static const char* talk_working[langM]= {
    "Now, please hang on - I am working for you.\n"
    "If the input file is very large, this will take several minutes.\n"
    "\n"
    "If you want to get acquainted with the much more powerful\n"
    "command line, this would have been your command:\n"
    "\n"
    ,
    "Einen Moment bitte - ich arbeite fuer Sie.\n"
    "Falls die Eingabe-Datei sehr gross ist, dauert das einige Minuten.\n"
    "\n"
    "Fall Sie sich mit der viel leistungsfaehigeren Kommandozeilen-\n"
    "eingabe vertraut machen wollen, das waere Ihr Kommando gewesen:\n"
    "\n"
    };
  static const char* talk_finished[langM]= {
    "Finished! Calculation time: "
    ,
    "Fertig! Berechnungsdauer: "
    };
  static const char* talk_finished_file[langM]= {
    "I just completed your new file with this name:\n"
    ,
    "Soeben habe ich Ihre neue Datei mit diesem Namen fertiggestellt:\n"
    };
  static const char* talk_error[langM]= {
    "I am sorry, an error has occurred (see above).\n"
    ,
    "Es tut mir Leid, es ist ein Fehler aufgetreten (siehe oben).\n"
    };
  static const char* talk_bye[langM]= {
    "\n"
    "Thanks for visiting me. Bye!\n"
    "Yours, Bert\n"
    "(To close this window, please press <Return>.)\n"
    ,
    "\n"
    "Danke fuer Ihren Besuch. Tschues!\n"
    "Schoene Gruesse - Bert\n"
    "(Zum Schließen dieses Fensters bitte die Eingabetaste druecken.)\n"
    };
  #define DD(s) fprintf(stderr,"%s",(s[lang]));  // display text
  #define DI(s) s[0]= 0; UR(fgets(s,sizeof(s),stdin)) \
    if(strchr(s,'\r')!=NULL) *strchr(s,'\r')= 0; \
    if(strchr(s,'\n')!=NULL) *strchr(s,'\n')= 0;  // get user's response
  bool
    function_convert= false,
    function_update= false,
    function_border_box= false,
    function_border_polygon= false,
    function_drop_author= false,
    function_statistics= false;
  int function_cut_mode= 0;
    // 0: normal; 1: complete ways; 2: complex ways;
  static bool function_only_statistics= false;
  static time_t start_time;
  bool verbose;
  char s[500];  // temporary string for several purposes
  char* sp;
  static char input_file[500];
  bool file_type_osm,file_type_osc,file_type_o5m,file_type_o5c,
    file_type_pbf;
  static char changefile[500];
  char polygon_file[500];
  char minlon[30],maxlon[30],minlat[30],maxlat[30];
  static char output_file[550]= "";  // the first three characters
    // are reserved for the commandline option "-o="
  int i;

  // display 'bye message' - if requested
  if(argcp==NULL) {
    static bool no_error= false;

    if(argvp==NULL)
      no_error= true;
    else {
      if(output_file[0]!=0) {
        DD(talk_section)
        if(no_error) {
          DD(talk_finished)
          fprintf(stderr,"%"PRIi64"s.\n",
            (int64_t)(time(NULL)-start_time));
          DD(talk_finished_file)
          fprintf(stderr,"  %s",output_file+3);
          }
        else
          DD(talk_error)
        DD(talk_bye)
        DI(s)
        }
      else if(function_only_statistics) {
        DD(talk_section)
        if(no_error) {
          DD(talk_finished)
          fprintf(stderr,"%"PRIi64"s.\n",
            (int64_t)(time(NULL)-start_time));
          }
        else
          DD(talk_error)
        DD(talk_bye)
        DI(s)
        }
      }
return false;
    }

  // initialization
  atexit(assistant_end);
  for(i= 1; i<langM; i++) {
    talk_section[i]= talk_section[0];
    // (this dialog text is the same for all languages)
    }
  verbose= false;

  /* get system language */ {
    const char* syslang;

    syslang= setlocale(LC_ALL,"");
    lang= langM;
    while(--lang>0)
      if(syslang!=NULL &&
          (strzcmp(syslang,talk_lang1[lang])==0 ||
          strzcmp(syslang,talk_lang2[lang])==0)) break;
    setlocale(LC_ALL,"C");  // switch back to C standard
    }

  // introduction
  DD(talk_intro)
  DI(s)
  sp= s;
  while(*sp==' ') sp++;  // dispose of leading spaces
  if((*sp!='a' && *sp!='A') || sp[1]!=0)
return true;
  verbose= isupper(*(unsigned char*)sp);

  // choose input file
  DD(talk_section)
  DD(talk_hello)
  for(;;) {
    DD(talk_input_file)
    DI(input_file)
    file_type_osm= strycmp(input_file,".osm")==0;
    file_type_osc= strycmp(input_file,".osc")==0;
    file_type_o5m= strycmp(input_file,".o5m")==0;
    file_type_o5c= strycmp(input_file,".o5c")==0;
    file_type_pbf= strycmp(input_file,".pbf")==0;
    if(!file_type_osm && !file_type_osc && !file_type_o5m &&
        !file_type_o5c && !file_type_pbf) {
      DD(talk_input_file_suffix)
  continue;
      }
    if(input_file[strcspn(input_file,"\"\', :;|&\\")]!=0 ||
        !file_exists(input_file)) {
      DD(talk_not_found)
  continue;
      }
    break;
    }
  DD(talk_thanks)

  // choose function
  DD(talk_section)
  for(;;) {
    function_convert= function_update= function_border_polygon=
      function_border_box= function_statistics= false;
    DD(talk_function)
    DI(s)
    i= 0;  // here: number of selected functions
    sp= s;
    while(*sp!=0) {
      if(*sp=='1')
        function_convert= true;
      else if(*sp=='2')
        function_update= true;
      else if(*sp=='3')
        function_border_box= true;
      else if(*sp=='4')
        function_border_polygon= true;
      else if(*sp=='5')
        function_drop_author= true;
      else if(*sp=='6')
        function_statistics= true;
      else if(*sp=='a' || *sp=='A') {
        if(function_cut_mode==0)
          function_cut_mode= 1;
        }
      else if(*sp=='b' || *sp=='B')
        function_cut_mode= 2;
      else if(*sp==' ' || *sp==',' || *sp==';') {
        sp++;
    continue;
        }
      else {  // syntax error
        i= 0;  // ignore previous input
    break;
        }
      i++; sp++;
      }
    if(function_border_box && function_border_polygon) {
      DD(talk_two_borders)
  continue;
      }
    if(i==0) {  // no function has been chosen OR syntax error
      DD(talk_cannot_understand)
  continue;
      }
    if(function_cut_mode!=0 &&
        !function_border_box && !function_border_polygon)
      function_border_box= true;
    break;
    }
  function_only_statistics= function_statistics &&
    !function_convert && !function_update &&
    !function_border_polygon && !function_border_box;
  DD(talk_all_right)

  // choose OSM Changefile
  if(function_update) {
    DD(talk_section)
    for(;;) {
      DD(talk_changefile)
      DI(changefile)
      if(strycmp(changefile,".osc")!=0 &&
          strycmp(changefile,".o5c")!=0) {
        DD(talk_changefile_suffix)
    continue;
        }
      if(changefile[strcspn(changefile,"\"\' ,:;|&\\")]!=0 ||
          !file_exists(changefile)) {
        DD(talk_not_found)
    continue;
        }
      break;
      }
    DD(talk_thanks)
    }

  // choose polygon file
  if(function_border_polygon) {
    DD(talk_section)
    for(;;) {
      DD(talk_polygon_file)
      DI(polygon_file)
      if(strycmp(polygon_file,".poly")!=0) {
        DD(talk_polygon_file_suffix)
    continue;
        }
      if(polygon_file[strcspn(polygon_file,"\"\' ,:;|&\\")]!=0 ||
          !file_exists(polygon_file)) {
        DD(talk_not_found)
    continue;
        }
      break;
      }
    DD(talk_thanks)
    }

  // choose coordinates
  if(function_border_box) {
    DD(talk_section)
    for(;;) {
      #define D(s) DI(s) \
        while(strchr(s,',')!=NULL) *strchr(s,',')= '.'; \
        if(s[0]==0 || s[strspn(s,"0123456789.-")]!=0) { \
          DD(talk_cannot_understand) continue; }
      DD(talk_coordinates)
      DD(talk_minlon)
      D(minlon)
      DD(talk_minlat)
      D(minlat)
      DD(talk_maxlon)
      D(maxlon)
      DD(talk_maxlat)
      D(maxlat)
      #undef D
      break;
      }
    DD(talk_thanks)
    }

  // choose file type
  if(function_convert) {
    file_type_osm= file_type_osc= file_type_o5m=
    file_type_o5c= file_type_pbf= false;
    DD(talk_section)
    for(;;) {
      DD(talk_output_format)
      DI(s)
      sp= s; while(*sp==' ') sp++;  // ignore spaces
      if(*sp=='1')
        file_type_osm= true;
      else if(*sp=='2')
        file_type_o5m= true;
      else if(*sp=='3')
        file_type_pbf= true;
      else {
        DD(talk_cannot_understand)
    continue;
        }
      break;
      }
    DD(talk_thanks)
    }

  // assemble output file name
  DD(talk_section)
  if(!function_only_statistics) {
    if(file_type_osm) strcpy(s,".osm");
    if(file_type_osc) strcpy(s,".osc");
    if(file_type_o5m) strcpy(s,".o5m");
    if(file_type_o5c) strcpy(s,".o5c");
    if(file_type_pbf) strcpy(s,".pbf");
    sp= stpcpy0(output_file,"-o=");
    strcpy(sp,input_file);
    sp= strrchr(sp,'.');
    if(sp==NULL) sp= strchr(output_file,0);
    i= 1;
    do
      sprintf(sp,"_%02i%s",i,s);
      while(++i<9999 && file_exists(output_file+3));
    }

  /* create new commandline arguments */ {
    int argc;
    static char* argv[10];
    static char border[550];

    argc= 0;
    argv[argc++]= (*argvp)[0];  // save program name
    if(verbose)
      argv[argc++]= "-v";  // activate verbose mode
    argv[argc++]= input_file;
    if(function_update)
      argv[argc++]= changefile;
    if(function_border_polygon) {
      sp= stpcpy0(border,"-B=");
      strcpy(sp,polygon_file);
      argv[argc++]= border;
      }
    else if(function_border_box) {
      sprintf(border,"-b=%s,%s,%s,%s",minlon,minlat,maxlon,maxlat);
      argv[argc++]= border;
      }
    if(function_drop_author)
      argv[argc++]= "--drop-author";
    if(function_cut_mode==1)
      argv[argc++]= "--complete-ways";
    if(function_cut_mode==2)
      argv[argc++]= "--complex-ways";
    if(function_only_statistics)
      argv[argc++]= "--out-statistics";
    else if(function_statistics)
        argv[argc++]= "--statistics";
    if(output_file[0]!=0) {
      if(file_type_osm) argv[argc++]= "--out-osm";
      else if(file_type_osc) argv[argc++]= "--out-osc";
      else if(file_type_o5m) argv[argc++]= "--out-o5m";
      else if(file_type_o5c) argv[argc++]= "--out-o5c";
      else if(file_type_pbf) argv[argc++]= "--out-pbf";
      argv[argc++]= output_file;
      }
    // return commandline variables
    *argcp= argc;
    *argvp= argv;

    // display the virtual command line
    DD(talk_working)
    fprintf(stderr,"osmconvert");
    i= 0;
    while(++i<argc)
      fprintf(stderr," %s",argv[i]);
    fprintf(stderr,"\n");
    DD(talk_section)
    }

  start_time= time(NULL);
  #undef langM
  #undef DP
  #undef DI
  return false;
  }  // assistant()

static void assistant_end() {
  // will be called via atexit()
  assistant(NULL,(char***)assistant_end);
  }  // assistant_end()



#if !__WIN32__
void sigcatcher(int sig) {
  fprintf(stderr,"osmconvert: Output has been terminated.\n");
  exit(1);
  }  // end   sigcatcher()
#endif

int main(int argc,char** argv) {
  // main program;
  // for the meaning of the calling line parameters please look at the
  // contents of helptext[];
  bool usesstdin;
  static char outputfilename[400]= "";  // standard output file name
    // =="": standard output 'stdout'
  int h_n,h_w,h_r;  // user-suggested hash size in MiB, for
    // hash tables of nodes, ways, and relations;
  int r,l;
  const char* a;  // command line argument
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
  usesstdin= false;
  h_n= h_w= h_r= 0;
  #if __WIN32__
    setmode(fileno(stdout),O_BINARY);
    setmode(fileno(stdin),O_BINARY);
  #endif

  // read command line parameters
  if(argc<=1) {  // no command line parameters given
    if(assistant(&argc,&argv))  // call interactive program guide
return 0;
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
        perror("osmconvert");
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
      fprintf(stderr,"osmconvert Parameter: %.2000s\n",a);
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
    if(strzcmp(a,"--diff-c")==0) {
        // user wants a diff file to be calculated
      global_diffcontents= true;
      global_diff= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--diff")==0) {
        // user wants a diff file to be calculated
      global_diff= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--subtract")==0) {
        // user wants to subtract any following input file
      global_subtract= true;
  continue;  // take next parameter
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
    if(strzcmp(a,"--drop-ver")==0) {
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
    if(strzcmp(a,"--drop-bro")==0) {
        // user does not want broken references in standard output
      global_dropbrokenrefs= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--drop-nod")==0) {
        // user does not want nodes section in standard output
      global_dropnodes= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--drop-way")==0) {
        // user does not want ways section in standard output
      global_dropways= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--drop-rel")==0) {
        // user does not want relations section in standard output
      global_droprelations= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--merge-ver")==0) {
        // user wants duplicate versions in input files to be merged
      global_mergeversions= true;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--csv="))>0 && a[l]!=0) {
        // user-defined columns for csv format
      csv_ini(a+l);
      global_outcsv= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--csv-headline")==0) {
        // write headline to csv output
      global_csvheadline= true;
      global_outcsv= true;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--csv-separator="))>0 && a[l]!=0) {
        // user-defined separator for csv format
      strMcpy(global_csvseparator,a+l);
      global_outcsv= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--in-josm")==0) {
      // deprecated;
      // this option is still accepted for compatibility reasons;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-o5m")==0 ||
        strcmp(a,"-5")==0) {
        // user wants output in o5m format
      global_outo5m= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-o5c")==0 ||
        strcmp(a,"-5c")==0) {
        // user wants output in o5m format
      global_outo5m= global_outo5c= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-osm")==0) {
        // user wants output in osm format
      global_outosm= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-osc")==0) {
        // user wants output in osc format
      global_outosc= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-osh")==0) {
        // user wants output in osc format
      global_outosh= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-none")==0) {
        // user does not want any standard output
      global_outnone= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-pbf")==0) {
        // user wants output in PBF format
      global_outpbf= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-csv")==0) {
        // user wants output in CSV format
      global_outcsv= true;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--pbf-granularity="))>0 && a[l]!=0) {
        // specify lon/lat granularity for .pbf input files
      global_pbfgranularity= oo__strtouint32(a+l);
      global_pbfgranularity100= global_pbfgranularity/100;
      global_pbfgranularity= global_pbfgranularity100*100;
      if(global_pbfgranularity==1) global_pbfgranularity= 0;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--emulate-pbf2")==0) {
        // emulate pbf2osm compatible output
      global_emulatepbf2osm= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--emulate-osmo")==0) {
        // emulate Osmosis compatible output
      global_emulateosmosis= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"--emulate-osmi")==0) {
        // emulate Osmium compatible output
      global_emulateosmium= true;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--timestamp="))>0 && a[l]!=0) {
        // user-defined file timestamp
      global_timestamp= oo__strtimetosint64(a+l);
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-timestamp")==0) {
        // user wants output in osc format
      global_outtimestamp= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--statistics")==0) {
        // print statistics (usually to stderr)
      global_statistics= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-statistics")==0) {  // print statistics to stdout
      global_outstatistics= true;
      global_statistics= true;
      global_outnone= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--complete-ways")==0) {
        // do not clip ways when applying borders
      global_completeways= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--complex-ways")==0) {
        // do not clip multipolygons when applying borders
      global_complexways= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--all-to-nodes")==0) {
        // convert ways and relations to nodes
      if(global_calccoords==0) global_calccoords= 1;
      global_alltonodes= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--all-to-nodes-bbox")==0) {
        // convert ways and relations to nodes,
        // and compute a bounding box
      PINFO("Option --all-to-nodes-bbox is deprecated. "
        "Using --all-to-nodes and --add-bbox-tags.");
      global_calccoords= -1;
      global_alltonodes= true;
      global_addbbox= true;
      global_add= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--add-bbox-tags")==0) {
        // compute a bounding box and add it as tag
      global_calccoords= -1;
      global_addbbox= true;
      global_add= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--add-bboxarea-tags")==0) {
        // compute a bounding box and add its area as tag
      global_calccoords= -1;
      global_addbboxarea= true;
      global_add= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--add-bboxweight-tags")==0) {
        // compute a bounding box and add its weight as tag
      global_calccoords= -1;
      global_addbboxweight= true;
      global_add= true;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--max-objects="))>0 && a[l]!=0) {
        // define maximum number of objects for --all-to-nodes
      global_maxobjects= oo__strtosint64(a+l);
      if(global_maxobjects<4) global_maxobjects= 4;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--max-refs="))>0 && a[l]!=0) {
        // define maximum number of references
      global_maxrefs= oo__strtosint64(a+l);
      if(global_maxrefs<1) global_maxrefs= 1;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--object-type-offset="))>0 && a[l]!=0) {
        // define id offset for ways and relations for --all-to-nodes
      global_otypeoffset10= oo__strtosint64(a+l);
      if(global_otypeoffset10<10) global_otypeoffset10= 10;
      if(strstr(a+l,"+1")!=NULL)
        global_otypeoffsetstep= true;
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
          fprintf(stderr,"osmconvert: Verbose mode.\n");
        else
          fprintf(stderr,"osmconvert: Verbose mode %i.\n",loglevel);
        }
  continue;  // take next parameter
      }
    if(strcmp(a,"-t")==0) {
        // test mode
      write_testmode= true;
      fprintf(stderr,"osmconvert: Entering test mode.\n");
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
    if(strzcmp(a,"-b=")==0) {
        // border consideration by a bounding box
      if(!border_box(a+3)) {
        fprintf(stderr,"osmconvert Error: use border format: "
          " -b=\"x1,y1,x2,y2\"\n");
return 3;
        }  // end   border consideration by a bounding box
      continue;  // take next parameter
      }
    if(strzcmp(a,"-B=")==0) {
        // border consideration by polygon file
      if(!border_file(a+3)) {
        fprintf(stderr,
          "osmconvert Error: no polygon file or too large: %s\n",a);
return 4;
        }  // end   border consideration by polygon file
  continue;  // take next parameter
      }
    if(strcmp(a,"-")==0) {  // use standard input
      usesstdin= true;
      if(oo_open(NULL))  // file cannot be read
return 2;
  continue;  // take next parameter
      }
    if(a[0]=='-') {
      PERRv("unrecognized option: %.80s",a)
return 1;
      }
    // here: parameter must be a file name
    if(strcmp(a,"/dev/stdin")==0)
      usesstdin= true;
    if(oo_open(a))  // file cannot be read
return 2;
    }  // end   for every parameter in command line

  // process parameters
  global_subtract= false;
  if(usesstdin && global_completeways) {
    PERR("cannot apply --complete-ways when reading standard input.")
return 2;
    }
  if(usesstdin && global_complexways) {
    PERR("cannot apply --complex-ways when reading standard input.")
return 2;
    }
  if(global_completeways || global_complexways) {
    uint32_t zlibflags;
    zlibflags= zlibCompileFlags();
    if(loglevel>=2) {
      PINFOv("zlib "ZLIB_VERSION" flags: %08"PRIx32"",zlibflags)
      }
    //if((zlibflags&0xc0) <= 0x40)
      //WARN("you are using the 32 bit zlib. Hence file size max. 2 GB.")
    }
  if(oo_ifn==0) {  // no input files given
    PERR("use \"-\" to read from standard input or try:  osmconvert -h")
return 0;  // end the program, because without having input files
      // we do not know what to do;
    }
  if(outputfilename[0]!=0 && !global_outo5m &&
      !global_outo5c && !global_outosm && !global_outosc &&
      !global_outosh && !global_outpbf && !global_outcsv &&
      !global_outnone && !global_outstatistics) {
      // have output file name AND  output format not defined
    // try to determine the output format by evaluating
    // the file name extension
    if(strycmp(outputfilename,".o5m")==0) global_outo5m= true;
    else if(strycmp(outputfilename,".o5c")==0)
      global_outo5m= global_outo5c= true;
    else if(strycmp(outputfilename,".osm")==0) global_outosm= true;
    else if(strycmp(outputfilename,".osc")==0) global_outosc= true;
    else if(strycmp(outputfilename,".osh")==0) global_outosh= true;
    else if(strycmp(outputfilename,".pbf")==0) global_outpbf= true;
    else if(strycmp(outputfilename,".csv")==0) global_outcsv= true;
    }
  if(write_open(outputfilename[0]!=0? outputfilename: NULL)!=0)
return 3;
  if(border_active || global_dropbrokenrefs) {  // user wants borders
    int r;

    if(global_diff) {
      PERR(
        "-b=, -B=, --drop-brokenrefs must not be combined with --diff");
return 6;
      }
    if(h_n==0) h_n= 1000;  // use standard value if not set otherwise
    if(h_w==0 && h_r==0) {
        // user chose simple form for hash memory value
      // take the one given value as reference and determine the 
      // three values using these factors: 90%, 9%, 1%
      h_w= h_n/10; h_r= h_n/100;
      h_n-= h_w; h_w-= h_r; }
    r= hash_ini(h_n,h_w,h_r);  // initialize hash table
    if(r==1)
      fprintf(stderr,"osmconvert: Hash size had to be reduced.\n");
    else if(r==2)
      fprintf(stderr,"osmconvert: Not enough memory for hash.\n");
    }  // end   user wants borders
  if(global_outo5m || border_active || global_dropbrokenrefs ||
      global_calccoords!=0) {
      // .o5m format is needed as output
    if(o5_ini()!=0) {
      fprintf(stderr,"osmconvert: Not enough memory for .o5m buffer.\n");
return 5;
      }
    }  // end   user wants borders
  if(global_diff) {
    if(oo_ifn!=2) {
      PERR("Option --diff requires exactly two input files.");
return 7;
      }
    if(!global_outosc && !global_outosh && !global_outo5c)
      global_outosc= true;
    }  // end   diff
  sprintf(strchr(global_tempfilename,0),".%"PRIi64,(int64_t)getpid());
  if(loglevel>=2)
    fprintf(stderr,"Tempfiles: %s.*\n",global_tempfilename);
  if(global_calccoords!=0)
    posi_ini();
  if(global_outcsv)
    csv_ini(NULL);

  // do the work
  r= oo_main();
  if(loglevel>=2) {  // verbose
    if(read_bufp!=NULL && read_bufp<read_bufe)
      fprintf(stderr,"osmconvert: Next bytes to parse:\n"
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
      fprintf(stderr,"osmconvert: Last processed: %s %"PRIu64".\n",
        ONAME(oo_sequencetype),oo_sequenceid);
    if(r!=0)
      fprintf(stderr,"osmconvert Exit: %i\n",r);
    }  // verbose mode
  assistant(NULL,NULL);
  return r;
  }  // end   main()


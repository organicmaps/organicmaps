# Curl library
TARGET = curl
TEMPLATE = lib
CONFIG += staticlib

DEFINES -= UNICODE

DEFINES += \
  CURL_STATICLIB \
  CURL_NO_OLDIES \
  HAVE_CONFIG_H \
  HTTP_ONLY \
  CURL_DISABLE_COOKIES \
  CURL_DISABLE_CRYPTO_AUTH \
  CURL_DISABLE_IMAP \
  CURL_DISABLE_LDAPS \
  CURL_DISABLE_POP3 \
  CURL_DISABLE_SMTP \
  CURL_DISABLE_VERBOSE_STRINGS \
  USE_BLOCKING_SOCKETS

ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

INCLUDEPATH += include

SOURCES += \
  lib/easy.c \
  lib/curl_rand.c \
  lib/hostip.c \
  lib/url.c \
  lib/hash.c \
  lib/transfer.c \
  lib/getinfo.c \
  lib/sendf.c \
  lib/connect.c \
  lib/inet_ntop.c \
  lib/mprintf.c \
  lib/share.c \
  lib/hostip4.c \
  lib/hostsyn.c \
  lib/curl_addrinfo.c \
  lib/multi.c \
  lib/llist.c \
  lib/sslgen.c \
  lib/select.c \
  lib/rawstr.c \
  lib/strequal.c \
  lib/timeval.c \
  lib/socks.c \
  lib/progress.c \
  lib/getenv.c \
  lib/escape.c \
  lib/warnless.c \
  lib/netrc.c \
  lib/speedcheck.c \
  lib/http.c \
  lib/http_chunks.c \
  lib/strerror.c \
  lib/wildcard.c \
  lib/if2ip.c \
  lib/inet_pton.c \
  lib/nonblock.c \
  lib/base64.c \
  lib/formdata.c \
  lib/parsedate.c \
  lib/splay.c \
  lib/strtok.c \
  lib/fileinfo.c \

HEADERS += \
  include/curl/curl.h \
  include/curl/curlver.h \
  include/curl/curlbuild.h \
  include/curl/curlrules.h \
  lib/config-iphone.h \
  lib/config-iphonesim.h \
  lib/config-mac64.h \
  lib/config-win32.h \
  lib/connect.h \
  lib/curl_config.h \
  lib/curl_memory.h \
  lib/curl_rand.h \
  lib/easyif.h \
  lib/getinfo.h \
  lib/hostip.h \
  lib/http_ntlm.h \
  lib/progress.h \
  lib/select.h \
  lib/sendf.h \
  lib/setup.h \
  lib/setup_once.h \
  lib/share.h \
  lib/slist.h \
  lib/sslgen.h \
  lib/strdup.h \
  lib/strequal.h \
  lib/transfer.h \
  lib/url.h \
  lib/urldata.h \

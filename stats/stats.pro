# Project that includes all stats projects.

TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = client

!iphone*:!bada*:!android* {
  SUBDIRS +=
}

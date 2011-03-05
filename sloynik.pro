TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS = \
    3party 3party_sloynik \
    base base/base_tests \
    coding coding/coding_tests \
    coding_sloynik coding_sloynik/coding_sloynik_tests \
    utils utils/utils_tests \
    words words/words_tests \
    publisher publisher/publisher_tests\
    console_sloynik


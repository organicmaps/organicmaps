#!/usr/bin/env python
import csv
import drules_struct_pb2
import os
import re
import sqlite3
import sys
import xml.etree.ElementTree as etree


def parse_mapcss_row(row):
    if len(row) < 2:
        # Allow for empty lines and comments that do not contain ';' symbol
        return None
    if len(row) == 3:
        # Short format: type name, type id, x / replacement type name
        tag = row[0].replace('|', '=')
        row = (row[0], '[{0}]'.format(tag), 'x' if len(row[2].strip()) > 0 else '')
    if len(row[2]) > 0:
        # Skipping obsolete types
        return None

    # Lifted off from Kothic
    cl = row[0].replace("|", "-")
    pairs = [i.strip(']').split("=") for i in row[1].split(',')[0].split('[')]
    kv = []
    for i in pairs:
        if len(i) == 1:
            if i[0]:
                if i[0][0] == "!":
                    kv.append((i[0][1:].strip('?'), "no"))
                else:
                    kv.append((i[0].strip('?'), "*"))
        elif len(i) == 2:
            kv.append(tuple(i))
        elif len(i) == 3:
            kv.append((i[0], i[1]))
            kv.append((i[1], i[2]))
    return cl, kv


def find_in_taginfo(cur, kv, seen):
    """Finds a set of tags in taginfo; updates a list of seen tags."""
    result = 0
    if len(kv) == 1:
        if kv[0][1] == '*':
            cur.execute('select count_all from keys where key = ?', (kv[0][0],))
        else:
            seen.add(kv[0])
            cur.execute('select count_all from tags where key = ? and value = ?', kv[0])
        row = cur.fetchone()
        if row is not None:
            result = row[0]
    else:
        # We disregard the 3rd pair of tags.
        if kv[0][1] == '*':
            kv[0], kv[1] = kv[1], kv[0]
        # Try to find a combination
        params = {'k1': kv[0][0], 'v1': kv[0][1], 'k2': kv[1][0], 'v2': '' if kv[1][1] == '*' else kv[1][1]}
        cur.execute("""select sum(count_all) from tag_combinations
                    where (key1=:k1 and value1=:v1 and key2=:k2 and value2=:v2)
                    or (key2=:k1 and value2=:v1 and key1=:k2 and value1=:v2)""", params)
        row = cur.fetchone()
        if row is not None and row[0] is not None:
            seen.add(kv[0])
            if kv[1][1] != '*':
                seen.add(kv[1])
            result = row[0]
        elif len(kv) == 2 and kv[1][0] not in ('city', 'colour', 'banner', 'bridge', 'tunnel', 'location', 'access', 'sport'):
            # If not found, just try to find the second tag
            if kv[1][1] == '*':
                cur.execute('select sum(count_all) from tags where key = ? and value != ?', (kv[1][0], 'no'))
            else:
                cur.execute('select count_all from tags where key = ? and value = ?', kv[1])
                seen.add(kv[1])
            row = cur.fetchone()
            sys.stderr.write('Failed query for {}, trying simple: {}\n'.format(kv, row))
            if row is not None:
                result = row[0]
    return result


def find_popular_taginfo(cur, seen):
    """Finds popular keys that have not been seen."""
    RE_VALID = re.compile(r'^[a-z_]+$')
    keys = ('amenity', 'shop', 'craft', 'emergency', 'office', 'highway', 'railway', 'tourism', 'historic', 'leisure', 'man_made')
    cur.execute("select key, value, count_all from tags where key in ({}) and count_all > 1000 order by count_all desc".
                format(','.join(['?' for x in keys])), keys)
    for row in cur:
        if (row[0], row[1]) not in seen and row[1] not in ('yes', 'no') and RE_VALID.match(row[1]):
            yield row


class DruleStat:
    def __init__(self):
        self.is_drawn = False
        self.has_icon = False
        self.has_name = False
        self.is_area = False


class EditStat:
    def __init__(self, editable='no', can_add='no'):
        self.editable = editable is None or editable == 'yes'
        self.can_add = can_add is None or can_add == 'yes'


def b2t(b, text='yes'):
    return text if b else ''


if __name__ == '__main__':
    DRULES_FILE_NAME = 'drules_proto_clear.bin'

    if len(sys.argv) < 2:
        print('Calculates tag usage and categories stats, suggesting new types.')
        print('Usage: {} <path_to_taginfo.db> [<path_to_omim_data>]'.format(sys.argv[0]))
        sys.exit(1)

    data_path = sys.argv[2] if len(sys.argv) >= 3 else os.path.join(os.path.dirname(sys.argv[0]), '..', '..', '..', 'data')

    # Read drules_proto.bin
    drules = drules_struct_pb2.ContainerProto()
    drules.ParseFromString(open(os.path.join(data_path, DRULES_FILE_NAME), 'rb').read())
    drawn = {}
    for elem in drules.cont:
        st = DruleStat()
        for el in elem.element:
            st.is_drawn = True
            if el.symbol.name:
                st.has_icon = True
            if el.caption.primary.height or el.path_text.primary.height:
                st.has_name = True
            if el.area.color:
                st.is_area = True
        if st.is_drawn:
            drawn[str(elem.name)] = st

    # Read editor.config
    ed_root = etree.parse(os.path.join(data_path, 'editor.config')).getroot()
    editor = {}
    for field in ed_root[0].find('types').findall('type'):
        editor[field.get('id')] = EditStat(field.get('editable'), field.get('can_add'))

    # Read mapcss-mapping.csv
    classificator = {}
    with open(os.path.join(data_path, 'mapcss-mapping.csv'), 'r') as f:
        for row in csv.reader(f, delimiter=';'):
            r = parse_mapcss_row(row)
            if r is not None:
                classificator[r[0]] = r[1]

    taginfo = sqlite3.connect(sys.argv[1])
    cursor = taginfo.cursor()

    # Iterate over know classificator types
    w = csv.writer(sys.stdout)
    w.writerow(('type', 'is editable', 'can add', 'drawn', 'icon', 'area', 'name drawn', 'usages in osm'))
    no_editor = EditStat()
    no_drawn = DruleStat()
    seen = set()
    for cl in sorted(classificator.keys()):
        # Find if the class is drawn and if there is an icon
        # Find the class in taginfo db
        row = [cl]
        ed = editor.get(cl, no_editor)
        row.extend([b2t(ed.editable, 'ed'), b2t(ed.can_add, 'add')])
        dr = drawn.get(cl, no_drawn)
        row.extend([b2t(dr.is_drawn, 'v'), b2t(dr.has_icon, 'i'), b2t(dr.is_area, 'a'), b2t(dr.has_name, 'n')])
        row.append(find_in_taginfo(cursor, classificator[cl], seen))
        row.append(' + '.join('{}={}'.format(k[0], k[1]) for k in classificator[cl]))
        w.writerow(row)

    try:
        tagwiki = sqlite3.connect(os.path.join(os.path.dirname(sys.argv[1]), 'taginfo-wiki.db'))
        wcur = tagwiki.cursor()
    except:
        wcur = None
    w.writerow([])
    w.writerow(['tag'] + ['']*6 + ['usages', 'description'])
    for row in find_popular_taginfo(cursor, seen):
        r = ['{}={}'.format(row[0], row[1])] + ['']*6 + [row[2]]
        if wcur is not None:
            wcur.execute("select description from wikipages where key=? and value=? and lang in ('en', 'ru') order by lang desc", (row[0], row[1]))
            wrow = wcur.fetchone()
            r.append('' if wrow is None or wrow[0] is None else wrow[0].encode('utf-8'))
        w.writerow(r)

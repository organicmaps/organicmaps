#!/usr/bin/env python
# Copyright: Veniamin Gvozdikov and contributors
# License: GNU AGPL, version 3 or later; http://www.gnu.org/licenses/agpl.html

#
# To generate configuration of CDN just execute command like below
#
# $ python generate_nginx_cdn.py
#

VERSIONS_LIST = set([
  210529,
  210703,
  210729,
  210825,
  211002,
  211022,
  211122,
  220103,
  220204,
  220314,
  220415,
  220515,
  220613,
  220718,
  220816,
  220912,
  221029,
  221119,
  221216,
  230121,
  230210,
  230227,
  230329,
  230503,
  230602,
  230710,
  230814,
  230920,
  231113,
  231213,
  240105,
  240202,
  240228,
  240326,
  240429,
  240528,
  240613,
  240702,
  240723,
  240810,
  240904,
  241001,
  241017,
  241107,
  241122,
  250121,
  250213,
  250227,
  250329,
  250418,
])

CDN_LIST = [
    "cdn-us1",
    "cdn-uk1",
    "cdn-nl1",
    "cdn",
    "cdn-beta",
    "cdn-fi1",
    "cdn-eu2",
    "cdn-de2",
    "cdn-de3",
    "cdn-us-east1",
    "cdn-us-west1",
    "cdn-vi1",
]

REGION_AMERICA_NORTH = [
    "cdn-us-east1",
    "cdn-us-west1",
    "cdn-uk1",
    "cdn-nl1",
    "cdn",
]

REGION_AMERICA_SOUTH = REGION_AMERICA_NORTH

REGION_ASIA = [
    "cdn-vi1",
    "cdn-uk1",
    "cdn-nl1",
    "cdn",
]

REGION_OCEANIA = [
    "cdn-vi1",
    "cdn-us-east1",
    "cdn-us-west1",
    "cdn",
]

REGION_DEFAULT = [
    "cdn-uk1",
    "cdn-nl1",
    "cdn-fi1",
    "cdn-eu2",
    "cdn-de2",
    "cdn-de3",
    "cdn",
]

REGION_LEGACY = [
    "cdn-us1",
    "cdn",
]

COUNTRIES_AMERICA_NORTH = set([
    "AI", "AG", "AW", "BB", "BZ", "BM", "BQ", "VG", "CA", "KY", "CR", "CU",
    "CW", "DM", "DO", "SV", "GL", "GD", "GP", "GT", "GT", "HN", "JM", "MQ",
    "MX", "MS", "AN", "NI", "PA", "PR", "BL", "KN", "LC", "MF", "PM", "VC",
    "SX", "BS", "TT", "TC", "US", "VI",
])

COUNTRIES_AMERICA_SOUTH = set([
    "AR", "BO", "BR", "CL", "CO", "EC", "FK", "GF", "GY", "PY", "PE", "SR",
    "UY", "VE",
])

COUNTRIES_ASIA = set([
    "AF", "AM", "AZ", "BH", "BD", "BT", "IO", "BN", "KH", "CN", "CC", "GE",
    "HK", "IN", "ID", "IR", "IQ", "IL", "JP", "JO", "KZ", "KW", "KG", "LA",
    "LB", "MO", "MY", "MV", "MN", "MM", "NP", "KP", "OM", "PK", "PS", "PH",
    "QA", "SA", "SG", "KR", "LK", "SY", "TW", "TJ", "TH", "TR", "TM", "AE",
    "UZ", "VN", "YE",
])

COUNTRIES_OCEANIA = set([
    "AS", "AU", "CX", "CK", "FJ", "PF", "GU", "KI", "MH", "FM", "NR", "NC",
    "NZ", "NU", "NF", "MP", "PW", "PG", "PN", "WS", "SB", "TL", "TK", "TO",
    "TV", "VU", "WF",
])

NGINX_TEMPLATE_HTTP = """
# Copyright: Veniamin Gvozdikov and contributors
# License: GNU AGPL, version 3 or later; http://www.gnu.org/licenses/agpl.html

worker_processes  1;

events {{
    worker_connections   1024; 
}}

http {{
    include              mime.types;
    default_type         application/octet-stream;
    sendfile             on;
    keepalive_timeout    65;

    map_hash_bucket_size 256;

    geoip_country /usr/share/GeoIP/GeoIP.dat;

    {maps}
    {regions}

    server {{
        listen       {port};
        server_name  {domain};
        access_log  /var/log/nginx/access.log;

        location ~/(servers|maps) {{
            default_type application/json;
            return 200 $servers_list_default;
        }}
    }}
}}
"""

NGINX_TEMPLATE_MAP_CDN = """
map "$http_x_om_dataversion:$cdn_region" $servers_list_{region} {{
    default '{{"error":"Unsupported DataVersion"}}';
    {versions}
}}
"""

NGINX_TEMPLATE_MAP_REGIONS = """
map $geoip_country_code $cdn_region {{
    {regions}
}}
"""


import json
from enum import Enum, unique

@unique
class GeoIPRegion(Enum):
    afrika = "AF"
    antarctica = "AN"
    asia = "AS"
    europe = "EU"
    north_america = "NA"
    oceania = "OC"
    south_america = "SA"

class CDNSetup:
    def __init__(self, domain, cdn):
        self.__domain = domain
        self.__cdn = cdn
        self.__proto = "http"
        self.__port = 80
        self._versions_available = 2

    def __make_cdn_url(self, cdn):
        return f"{self.__proto}://{cdn}.{self.__cdn}/"

    def __add_indent(self, string, indent):
        return " " * indent + string + "\n"

    def __gen_region(self, cdns):
        conf = ""
        buf = 'default "default";\n'

        for continent, _, _, countries in cdns:
            for country in countries:
                buf += f'"{country}" "{continent}";\n'

        for _str in buf.split("\n"):
            conf += self.__add_indent(_str, 4) 

        return NGINX_TEMPLATE_MAP_REGIONS.format(regions=conf.strip())


    def __hack_old_versions(self, region):
        data = {
            "servers": [self.__make_cdn_url(cdn) for cdn in REGION_LEGACY]
        }
        return f"'~^[0-9]+:{region}' '{json.dumps(data)}';\n"

    def __gen_versions(self, region, cdns):
        buf = ""
        conf = "\n"
        data = {
            "servers": [self.__make_cdn_url(cdn) for cdn in cdns]
        }
        payload = json.dumps(data)

        for version in list(VERSIONS_LIST)[-self._versions_available:]:
            buf += "'~{version}:{region}' '{payload}';\n".format(
                region=region,
                version=version,
                payload=payload,
            )

        buf += self.__hack_old_versions(region)

        for _str in buf.split("\n"):
            conf += self.__add_indent(_str, 4) 

        return conf

    def __gen_mapping(self, cdns):
        versions = self.__gen_versions("default", REGION_DEFAULT)

        for name, cdns, _, _ in cdns:
            versions += self.__gen_versions(name, cdns)

        return NGINX_TEMPLATE_MAP_CDN.format(region="default", versions=versions)

    def maps(self, regions):
        buf_regions = ""
        buf_map = ""

        gen_regions = ""
        gen_map = ""

        map_cdns_nginx = [
            ("asia", REGION_ASIA, (GeoIPRegion.asia,), COUNTRIES_ASIA),
            ("americas", REGION_AMERICA_NORTH, (GeoIPRegion.north_america, GeoIPRegion.south_america), COUNTRIES_AMERICA_NORTH.union(COUNTRIES_AMERICA_SOUTH)),
            ("oceania", REGION_OCEANIA, (GeoIPRegion.oceania,), COUNTRIES_OCEANIA),
        ]

        buf_map += self.__gen_mapping(map_cdns_nginx)
        buf_regions += self.__gen_region(map_cdns_nginx)

        for _str in buf_regions.split("\n"):
            gen_regions += self.__add_indent(_str, 4) 

        for _str in buf_map.split("\n"):
            gen_map += self.__add_indent(_str, 4) 

        nginx_conf = NGINX_TEMPLATE_HTTP.format(
            maps=gen_map,
            regions=gen_regions.strip(),
            domain=self.__domain,
            port=self.__port,
        )
        return nginx_conf


if __name__ == "__main__": 
    configration = CDNSetup("meta.omaps.app", "organicmaps.app")
    print(configration.maps(GeoIPRegion))


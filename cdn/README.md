Generation CDN configuration
----------------------------

It is simple implementation of CDN to work with generated maps. To generate
configuration just run command and save to your file:

```
$ python generate_nginx_cdn.py
```

Output should look like below.

```
# Copyright: Veniamin Gvozdikov and contributors
# License: GNU AGPL, version 3 or later; http://www.gnu.org/licenses/agpl.html

worker_processes  1;

events {
    worker_connections   1024;
}

http {
    include              mime.types;
    default_type         application/octet-stream;
    sendfile             on;
    keepalive_timeout    65;

    map_hash_bucket_size 256;

    geoip_country /usr/share/GeoIP/GeoIP.dat;


    map "$http_x_om_dataversion:$cdn_region" $servers_list_default {
        default '{"error":"Unsupported DataVersion"}';

        '~241017:default' '{"servers": ["http://cdn-uk1.organicmaps.app/", "http://cdn-nl1.organicmaps.app/", "http://cdn-fi1.organicmaps.app/", "http://cdn-eu2.organicmaps.app/", "http://cdn-de2.organicmaps.app/", "http://cdn-de3.organicmaps.app/", "http://cdn.organicmaps.app/"]}';
        '~220415:default' '{"servers": ["http://cdn-uk1.organicmaps.app/", "http://cdn-nl1.organicmaps.app/", "http://cdn-fi1.organicmaps.app/", "http://cdn-eu2.organicmaps.app/", "http://cdn-de2.organicmaps.app/", "http://cdn-de3.organicmaps.app/", "http://cdn.organicmaps.app/"]}';
        '~^[0-9]+:default' '{"servers": ["http://cdn-us1.organicmaps.app/", "http://cdn.organicmaps.app/"]}';


        '~241017:asia' '{"servers": ["http://cdn-vi1.organicmaps.app/", "http://cdn-uk1.organicmaps.app/", "http://cdn-nl1.organicmaps.app/", "http://cdn.organicmaps.app/"]}';
        '~220415:asia' '{"servers": ["http://cdn-vi1.organicmaps.app/", "http://cdn-uk1.organicmaps.app/", "http://cdn-nl1.organicmaps.app/", "http://cdn.organicmaps.app/"]}';
        '~^[0-9]+:asia' '{"servers": ["http://cdn-us1.organicmaps.app/", "http://cdn.organicmaps.app/"]}';


        '~241017:americas' '{"servers": ["http://cdn-us-east1.organicmaps.app/", "http://cdn-us-west1.organicmaps.app/", "http://cdn-uk1.organicmaps.app/", "http://cdn-nl1.organicmaps.app/", "http://cdn.organicmaps.app/"]}';
        '~220415:americas' '{"servers": ["http://cdn-us-east1.organicmaps.app/", "http://cdn-us-west1.organicmaps.app/", "http://cdn-uk1.organicmaps.app/", "http://cdn-nl1.organicmaps.app/", "http://cdn.organicmaps.app/"]}';
        '~^[0-9]+:americas' '{"servers": ["http://cdn-us1.organicmaps.app/", "http://cdn.organicmaps.app/"]}';


        '~241017:oceania' '{"servers": ["http://cdn-vi1.organicmaps.app/", "http://cdn-us-east1.organicmaps.app/", "http://cdn-us-west1.organicmaps.app/", "http://cdn.organicmaps.app/"]}';
        '~220415:oceania' '{"servers": ["http://cdn-vi1.organicmaps.app/", "http://cdn-us-east1.organicmaps.app/", "http://cdn-us-west1.organicmaps.app/", "http://cdn.organicmaps.app/"]}';
        '~^[0-9]+:oceania' '{"servers": ["http://cdn-us1.organicmaps.app/", "http://cdn.organicmaps.app/"]}';


    }


    map $geoip_country_code $cdn_region {
        default "default";
        "MM" "asia";
        "KH" "asia";
        "KP" "asia";
        "TJ" "asia";
        "IO" "asia";
        "JP" "asia";
        "PH" "asia";
        "CC" "asia";
        "IQ" "asia";
        "AF" "asia";
        "BN" "asia";
        "QA" "asia";
        "MV" "asia";
        "VN" "asia";
        "UZ" "asia";
        "KR" "asia";
        "LK" "asia";
        "AM" "asia";
        "GE" "asia";
        "SG" "asia";
        "KZ" "asia";
        "KW" "asia";
        "TR" "asia";
        "AZ" "asia";
        "SA" "asia";
        "LB" "asia";
        "TM" "asia";
        "KG" "asia";
        "AE" "asia";
        "MY" "asia";
        "MN" "asia";
        "PK" "asia";
        "JO" "asia";
        "LA" "asia";
        "MO" "asia";
        "BH" "asia";
        "PS" "asia";
        "IL" "asia";
        "IR" "asia";
        "HK" "asia";
        "TH" "asia";
        "CN" "asia";
        "NP" "asia";
        "ID" "asia";
        "BD" "asia";
        "OM" "asia";
        "SY" "asia";
        "TW" "asia";
        "IN" "asia";
        "BT" "asia";
        "YE" "asia";
        "MS" "americas";
        "CO" "americas";
        "BO" "americas";
        "KN" "americas";
        "BZ" "americas";
        "BM" "americas";
        "AR" "americas";
        "MF" "americas";
        "AI" "americas";
        "SR" "americas";
        "BB" "americas";
        "CR" "americas";
        "HN" "americas";
        "JM" "americas";
        "VC" "americas";
        "NI" "americas";
        "GP" "americas";
        "PE" "americas";
        "EC" "americas";
        "MX" "americas";
        "BQ" "americas";
        "TC" "americas";
        "TT" "americas";
        "UY" "americas";
        "CL" "americas";
        "LC" "americas";
        "AG" "americas";
        "CA" "americas";
        "MQ" "americas";
        "VI" "americas";
        "GF" "americas";
        "AW" "americas";
        "PR" "americas";
        "DO" "americas";
        "BR" "americas";
        "AN" "americas";
        "CU" "americas";
        "PM" "americas";
        "US" "americas";
        "PY" "americas";
        "VE" "americas";
        "PA" "americas";
        "BS" "americas";
        "FK" "americas";
        "BL" "americas";
        "SV" "americas";
        "CW" "americas";
        "GT" "americas";
        "SX" "americas";
        "GY" "americas";
        "DM" "americas";
        "KY" "americas";
        "VG" "americas";
        "GD" "americas";
        "GL" "americas";
        "CK" "oceania";
        "FJ" "oceania";
        "GU" "oceania";
        "CX" "oceania";
        "AU" "oceania";
        "TK" "oceania";
        "PG" "oceania";
        "FM" "oceania";
        "PN" "oceania";
        "VU" "oceania";
        "NC" "oceania";
        "NU" "oceania";
        "MP" "oceania";
        "NF" "oceania";
        "TO" "oceania";
        "KI" "oceania";
        "TV" "oceania";
        "TL" "oceania";
        "AS" "oceania";
        "WF" "oceania";
        "NR" "oceania";
        "MH" "oceania";
        "SB" "oceania";
        "PF" "oceania";
        "WS" "oceania";
        "NZ" "oceania";
        "PW" "oceania";
    }

    server {
        listen       80;
        server_name  meta.omaps.app;
        access_log  /var/log/nginx/access.log;

        location ~/(servers|maps) {
            default_type application/json;
            return 200 $servers_list_default;
        }
    }
}
```

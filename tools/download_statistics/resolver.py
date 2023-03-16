#!/usr/bin/env python
#coding: utf-8

import geoip2.database
import sys
from collections import defaultdict

reader = geoip2.database.Reader('./GeoLite2-Country.mmdb')

try:
    with sys.stdin as file:
	for rec in file:
	    try:
		parts = rec.strip().split('|')
		ip = parts[0]
		from_country = None
		try:
		    from_country = reader.country(ip).country.name
		except geoip2.errors.AddressNotFoundError:
		    from_country = 'Unknown'
		
		print '{}|{}'.format(from_country,'|'.join(parts))
#		print '{} | {} {} {} | {} | {} | {}'.format(from_country, date[0], date[1], date[2][:4], ip, parts[1][1:13], parts[1][parts[1].find(':')+1:-1])
	    except:
		pass # ignore all errors for one string 
except KeyboardInterrupt:
    exit(0)
except:
    raise



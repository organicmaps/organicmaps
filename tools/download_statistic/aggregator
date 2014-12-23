#!/usr/bin/env python
#coding: utf-8

from collections import defaultdict
import sys
import datetime

result = defaultdict(lambda : defaultdict(lambda :defaultdict(set)))



def print_result():
    for date_key in result.iterkeys():
	year, month, req_type = date_key.split('_')
	for from_country in result[date_key].iterkeys():
	    for req_country in result[date_key][from_country].iterkeys():
		print '{};{:02d};{};{};{};{}'.format(year,int(month),from_country,req_country,req_type,len(result[date_key][from_country][req_country]))

try:
    with sys.stdin as file:
	for rec in file:
	    try:
		parts = rec.strip().split('|')
		req_type = 'R' if len(parts) == 6 and parts[5]=='.routing' else 'M'
		from_country = parts[0]
		date = datetime.datetime.strptime(parts[2], '%d/%b/%Y:%H:%M:%S')
		user_id = parts[3]
		req_country = parts[4].split('_')[0]
		date_key = '{}_{}_{}'.format(date.year,date.month,req_type)
		user_key = '{}_{}'.format(user_id,req_country)
		result[date_key][from_country][req_country].add(user_key)
	    except:
		pass # ignore all errors for one string 
except KeyboardInterrupt:
    print_result()
    exit(0)
except:
    print_result()
    raise
    
print_result()



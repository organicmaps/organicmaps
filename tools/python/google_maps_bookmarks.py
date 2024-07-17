#!/usr/bin/env python3

import csv
import json
import os.path
import traceback
import urllib.error
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET
from os import R_OK


class GoogleMapsConverter:

    def __init__(self):
        print("Follow these steps to export your saved places from Google Maps and convert them to a KML-File")
        print()
        print("1. Create an API key for Google Places API following this guide")
        print()
        print("https://developers.google.com/maps/documentation/places/web-service/get-api-key")
        print()
        print("2. Go to https://takeout.google.com/ and sign in with your Google account")
        print("3. Select 'Saved' and 'Maps (My Places)' and create an export")
        print("4. Unzip the export and look for csv files in the folder Takeout/Saved/")
        print()
        while True:
            self.csv_file = input("Insert path to csv file: ")

            if not self.csv_file:
                print("Please provide a csv file" + os.linesep)
                continue
            elif not os.path.isfile(self.csv_file):
                print(f"Couldn't find {self.csv_file}" + os.linesep)
                continue
            elif not os.access(self.csv_file, R_OK):
                print(f"Couldn't read {self.csv_file}" + os.linesep)
                continue
            else:
                break

        while True:
            self.api_key = input("API key: ")
            if not self.api_key:
                print("Please provide an API key" + os.linesep)
                continue
            else:
                break

        while True:
            bookmark_list_name = input("Bookmark list name: ")
            if not bookmark_list_name:
                print("Please provide a name" + os.linesep)
                continue
            else:
                self.kml_file = bookmark_list_name + ".kml"
                break
        print()
        self.places = []

    def parse_csv(self):
        with open(self.csv_file, newline='') as file:
            row_count = sum(1 for _ in file)
            file.seek(0)
            csvreader = csv.reader(file, delimiter=',')
            next(csvreader)  # skip header
            for idx, row in enumerate(csvreader):
                name = row[0]
                description = row[1]
                url = row[2]
                print(f"\rProgress: {idx + 1}/{row_count - 1} Parsing {name}...", end='')
                try:
                    if url.startswith("https://www.google.com/maps/search/"):
                        coordinates = url.split('/')[-1].split(',')
                        coordinates.reverse()
                        coordinates = ','.join(coordinates)
                    elif url.startswith('https://www.google.com/maps/place/'):
                        ftid = url.split('!1s')[-1]
                        params = {'key': self.api_key, 'fields': 'geometry', 'ftid': ftid}
                        places_url = "https://maps.googleapis.com/maps/api/place/details/json?" \
                                     + urllib.parse.urlencode(params)
                        try:
                            data = get_json(places_url)
                            location = data['result']['geometry']['location']
                            coordinates = ','.join([str(location['lng']), str(location['lat'])])
                        except (urllib.error.URLError, KeyError):
                            print(f"Couldn't extract coordinates from Googe Maps. Skipping {name}")
                            continue
                    else:
                        print(f"Couldn't parse url. Skipping {name}")
                        continue

                    self.places.append({'name': name, 'description': description, 'coordinates': coordinates})
                except Exception:
                    print(f"Couldn't parse {name}: {traceback.format_exc()}")

    def write_kml(self):
        root = ET.Element("kml")
        doc = ET.SubElement(root, "Document")
        for place in self.places:
            placemark = ET.SubElement(doc, "Placemark")
            ET.SubElement(placemark, "name").text = place['name']
            ET.SubElement(placemark, "description").text = place['description']
            point = ET.SubElement(placemark, "Point")
            ET.SubElement(point, "coordinates").text = place['coordinates']
        tree = ET.ElementTree(root)
        tree.write(self.kml_file)
        print()
        print()
        print("Exported Google Saved Places to " + os.path.abspath(self.kml_file))


def get_json(url):
    max_attempts = 3
    for retry in range(max_attempts):
        try:
            response = urllib.request.urlopen(url)
            return json.load(response)
        except urllib.error.URLError:
            print(f"Couldn't connect to Google Maps. Retrying... ({retry + 1}/{max_attempts})")
            if retry < max_attempts - 1:
                continue
            else:
                raise


converter = GoogleMapsConverter()
converter.parse_csv()
converter.write_kml()

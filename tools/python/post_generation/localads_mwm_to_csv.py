import csv
import ctypes
import logging
import os
import sys
from multiprocessing import Pool
from multiprocessing import Process
from multiprocessing import Queue
from zlib import adler32

from mwm import MetadataField
from mwm import Mwm
from mwm.ft2osm import read_osm2ft

HEADERS = {
    "mapping": "osmid fid mwm_id mwm_version source_type".split(),
    "sponsored": "sid fid mwm_id mwm_version source_type".split(),
    "mwm": "mwm_id name mwm_version".split(),
}
QUEUES = {name: Queue() for name in HEADERS}
GOOD_TYPES = (
    "amenity",
    "shop",
    "tourism",
    "leisure",
    "sport",
    "craft",
    "man_made",
    "office",
    "historic",
    "aeroway",
    "natural-beach",
    "natural-peak",
    "natural-volcano",
    "natural-spring",
    "natural-cave_entrance",
    "waterway-waterfall",
    "place-island",
    "railway-station",
    "railway-halt",
    "aerialway-station",
    "building-train_station",
)
SOURCE_TYPES = {"osm": 0, "booking": 1}


def generate_id_from_name_and_version(name, version):
    return ctypes.c_long((adler32(bytes(name, "utf-8")) << 32) | version).value


def parse_mwm(mwm_name, osm2ft_name, override_version):
    region_name = os.path.splitext(os.path.basename(mwm_name))[0]
    logging.info(region_name)
    with open(osm2ft_name, "rb") as f:
        ft2osm = read_osm2ft(f, ft2osm=True, tuples=False)

    mwm_file = Mwm(mwm_name)
    version = override_version or mwm_file.version().version
    mwm_id = generate_id_from_name_and_version(region_name, version)
    QUEUES["mwm"].put((mwm_id, region_name, version))
    for feature in mwm_file:
        osm_id = ft2osm.get(feature.index(), None)
        readable_types = feature.readable_types()
        if osm_id is None:
            metadata = feature.metadata()
            if metadata is not None and MetadataField.sponsored_id in metadata:
                for t in readable_types:
                    if t.startswith("sponsored-"):
                        QUEUES["sponsored"].put(
                            (
                                metadata[MetadataField.sponsored_id],
                                feature.index(),
                                mwm_id,
                                version,
                                SOURCE_TYPES[t[t.find("-") + 1 :]],
                            )
                        )
                        break
        else:
            for t in readable_types:
                if t.startswith(GOOD_TYPES):
                    QUEUES["mapping"].put(
                        (
                            ctypes.c_long(osm_id).value,
                            feature.index(),
                            mwm_id,
                            version,
                            SOURCE_TYPES["osm"],
                        )
                    )
                    break


def write_csv(output_dir, qtype):
    with open(os.path.join(output_dir, qtype + ".csv"), "w") as f:
        mapping = QUEUES[qtype].get()
        w = csv.writer(f)
        w.writerow(HEADERS[qtype])
        while mapping is not None:
            w.writerow(mapping)
            mapping = QUEUES[qtype].get()


def create_csv(output, mwm_path, osm2ft_path, version, threads):
    if not os.path.isdir(output):
        os.mkdir(output)

    # Create CSV writer processes for each queue and a pool of MWM readers.
    writers = [Process(target=write_csv, args=(output, qtype)) for qtype in QUEUES]
    for w in writers:
        w.start()

    pool = Pool(processes=threads)
    for mwm_name in os.listdir(mwm_path):
        if (
            "World" in mwm_name
            or "minsk_pass" in mwm_name
            or not mwm_name.endswith(".mwm")
        ):
            continue
        osm2ft_name = os.path.join(osm2ft_path, os.path.basename(mwm_name) + ".osm2ft")
        if not os.path.exists(osm2ft_name):
            logging.error("Cannot find %s", osm2ft_name)
            sys.exit(2)
        parse_mwm_args = (os.path.join(mwm_path, mwm_name), osm2ft_name, int(version))
        pool.apply_async(parse_mwm, parse_mwm_args)
    pool.close()
    pool.join()
    for queue in QUEUES.values():
        queue.put(None)
    for w in writers:
        w.join()

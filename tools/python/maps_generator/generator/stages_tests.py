import os
from datetime import datetime
import json

from maps_generator.generator import settings
from maps_generator.generator.env import Env
from maps_generator.utils.file import download_file


def make_test_booking_data(max_days):
    def test_booking_data(env: Env, logger, *args, **kwargs):
        base_url, _ = settings.HOTELS_URL.rsplit("/", maxsplit=1)
        url = f"{base_url}/meta.json"
        meta_path = os.path.join(env.paths.tmp_dir(), "hotels-meta.json")

        download_file(url, meta_path)

        with open(meta_path) as f:
            meta = json.load(f)
            raw_date = meta["latest"].strip()
            logger.info(f"Booking date is from {raw_date}.")
            dt = datetime.strptime(raw_date, "%Y_%m_%d-%H_%M_%S")
            return (env.dt - dt).days < max_days

    return test_booking_data

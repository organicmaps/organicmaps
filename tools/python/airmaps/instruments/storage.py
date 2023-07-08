import logging

import webdav.client as wc

from airmaps.instruments import settings

logger = logging.getLogger("airmaps")

WD_OPTIONS = {
    "webdav_hostname": settings.WD_HOST,
    "webdav_login": settings.WD_LOGIN,
    "webdav_password": settings.WD_PASSWORD,
}


def wd_fetch(src, dst):
    logger.info(f"Fetch form {src} to {dst} with options {WD_OPTIONS}.")
    client = wc.Client(WD_OPTIONS)
    client.download_sync(src, dst)


def wd_publish(src, dst):
    logger.info(f"Publish form {src} to {dst} with options {WD_OPTIONS}.")
    client = wc.Client(WD_OPTIONS)
    tmp = f"{dst[:-1]}__/" if dst[-1] == "/" else f"{dst}__"
    client.upload_sync(local_path=src, remote_path=tmp)
    client.move(remote_path_from=tmp, remote_path_to=dst)

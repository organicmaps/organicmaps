import logging
import os
import tarfile

from six import BytesIO

from data_files import find_data_files

try:
    import lzma
except ImportError:
    from backports import lzma

logger = logging.getLogger(__name__)


def init(borders_path=None):
    data_path = find_data_files("omim-data")

    if data_path is None:
        logger.error("omim-data was not found.")
        return False

    if borders_path is None:
        borders_path = os.path.join(data_path, "borders")

    if not os.path.exists(borders_path):
        tar_lzma_path = os.path.join(data_path, "borders.tar.xz")
        lzma_stream = BytesIO()
        with open(tar_lzma_path, mode="rb") as f:
            decompressed = lzma.decompress(f.read())
            lzma_stream.write(decompressed)

        lzma_stream.seek(0)
        try:
            with tarfile.open(fileobj=lzma_stream, mode="r") as tar:
                tar.extractall(borders_path)
        except PermissionError as e:
            logger.error(str(e))
            return False

        logger.info("{} was created.".format(borders_path))

    return True

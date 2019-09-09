import logging


class DummyObject:
    def __getattr__(self, name):
        return lambda *args: None


def create_file_logger(file, level=logging.DEBUG,
                       format="[%(asctime)s] %(levelname)s %(module)s %(message)s"):
    logger = logging.getLogger(file)
    logger.setLevel(level)
    formatter = logging.Formatter(format)
    handler = logging.FileHandler(file)
    handler.setLevel(level)
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    return logger, handler

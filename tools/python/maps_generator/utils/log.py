import logging

logger = logging.getLogger("maps_generator")


class DummyObject:
    def __getattr__(self, name):
        return lambda *args: None


def create_file_handler(
        file,
        level=logging.DEBUG,
        formatter=None
):
    if formatter is None and logger.hasHandlers():
        formatter = logger.handlers[0].formatter

    handler = logging.FileHandler(file)
    handler.setLevel(level)
    handler.setFormatter(formatter)
    return handler


def create_file_logger(
    file,
    level=logging.DEBUG,
    formatter=None
):
    _logger = logging.getLogger(file)
    _logger.setLevel(level)
    handler = create_file_handler(file, level, formatter)
    _logger.addHandler(handler)
    return _logger, handler

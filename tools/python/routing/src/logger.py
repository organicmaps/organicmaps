import logging

formatter = logging.Formatter('%(asctime)s |  %(levelname)s: %(message)s')

LOG = logging.getLogger('main_logger')

stream_handler = logging.StreamHandler()
stream_handler.setFormatter(formatter)
stream_handler.setLevel(logging.INFO)

LOG.addHandler(stream_handler)
LOG.setLevel('INFO')

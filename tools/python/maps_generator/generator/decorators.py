import datetime
import logging
import os
import time
from functools import wraps

from .env import Env
from .status import Status
from ..utils.log import create_file_logger, DummyObject

logger = logging.getLogger("maps_generator")


def stage(func):
    @wraps(func)
    def wrap(env: Env, *args, **kwargs):
        func_name = func.__name__
        stage_formatted = " ".join(func_name.split("_")).capitalize()
        logfile = os.path.join(env.log_path, f"{func_name}.log")
        log_handler = logging.FileHandler(logfile)
        logger.addHandler(log_handler)
        if not env.is_accepted_stage(func):
            logger.info(f"{stage_formatted} was not accepted.")
            logger.removeHandler(log_handler)
            return
        main_status = env.main_status
        main_status.init(env.main_status_path, func_name)
        if main_status.need_skip():
            logger.warning(f"{stage_formatted} was skipped.")
            logger.removeHandler(log_handler)
            return
        main_status.update_status()
        logger.info(f"{stage_formatted}: start ...")
        t = time.time()
        env.set_subprocess_out(log_handler.stream)
        func(env, *args, **kwargs)
        d = time.time() - t
        logger.info(f"{stage_formatted}: finished in "
                    f"{str(datetime.timedelta(seconds=d))}")
        logger.removeHandler(log_handler)

    return wrap


def country_stage_status(func):
    @wraps(func)
    def wrap(env: Env, country: str, *args, **kwargs):
        func_name = func.__name__
        _logger = DummyObject()
        countries_meta = env.countries_meta
        if "logger" in countries_meta[country]:
            _logger, _ = countries_meta[country]["logger"]
        stage_formatted = " ".join(func_name.split("_")).capitalize()
        if not env.is_accepted_stage(func):
            _logger.info(f"{stage_formatted} was not accepted.")
            return
        if "status" not in countries_meta[country]:
            countries_meta[country]["status"] = Status()
        status = countries_meta[country]["status"]
        status_file = os.path.join(env.status_path, f"{country}.status")
        status.init(status_file, func_name)
        if status.need_skip():
            _logger.warning(f"{stage_formatted} was skipped.")
            return
        status.update_status()
        func(env, country, *args, **kwargs)

    return wrap


def country_stage_log(func):
    @wraps(func)
    def wrap(env: Env, country: str, *args, **kwargs):
        func_name = func.__name__
        log_file = os.path.join(env.log_path, f"{country}.log")
        countries_meta = env.countries_meta
        if "logger" not in countries_meta[country]:
            countries_meta[country]["logger"] = create_file_logger(log_file)
        _logger, log_handler = countries_meta[country]["logger"]
        stage_formatted = " ".join(func_name.split("_")).capitalize()
        _logger.info(f"{stage_formatted}: start ...")
        t = time.time()
        env.set_subprocess_out(log_handler.stream, country)
        func(env, country, *args, logger=_logger, **kwargs)
        d = time.time() - t
        _logger.info(f"{stage_formatted}: finished in "
                     f"{str(datetime.timedelta(seconds=d))}")

    return wrap


def country_stage(func):
    return country_stage_log(country_stage_status(func))

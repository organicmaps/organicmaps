""""
This file contains some decorators that define stages.
There are two main types of stages:
    1. outer_stage - a high level stage
    2. country_stage - a stage that applies to countries files(*.mwm).

country_stage might be inside stage. There are country stages inside mwm_stage.
mwm_stage is only one stage that contains country_stages.
"""
import datetime
import logging
import os
import time
from abc import ABC
from abc import abstractmethod
from collections import defaultdict
from typing import AnyStr
from typing import Callable
from typing import Dict
from typing import List
from typing import Optional
from typing import Type
from typing import Union

import filelock

from maps_generator.generator import status
from maps_generator.generator.exceptions import FailedTest
from maps_generator.utils.file import download_files
from maps_generator.utils.file import normalize_url_to_path_dict
from maps_generator.utils.log import DummyObject
from maps_generator.utils.log import create_file_handler
from maps_generator.utils.log import create_file_logger

logger = logging.getLogger("maps_generator")


class InternalDependency:
    def __init__(self, url, path_method, mode=""):
        self.url = url
        self.path_method = path_method
        self.mode = mode


class Test:
    def __init__(self, test, need_run=None, is_pretest=False):
        self._test = test
        self._need_run = need_run
        self.is_pretest = is_pretest

    @property
    def name(self):
        return self._test.__name__

    def need_run(self, env, _logger):
        if self._need_run is None:
            return True

        if callable(self._need_run):
            return self._need_run(env, _logger)

        return self._need_run

    def test(self, env, _logger, *args, **kwargs):
        try:
            res = self._test(env, _logger, *args, **kwargs)
        except Exception as e:
            raise FailedTest(f"Test {self.name} is failed.") from e

        if not res:
            raise FailedTest(f"Test {self.name} is failed.")

        _logger.info(f"Test {self.name} is successfully completed.")


class Stage(ABC):
    need_planet_lock = False
    need_build_lock = False
    is_helper = False
    is_mwm_stage = False
    is_production_only = False

    def __init__(self, **args):
        self.args = args

    def __call__(self, env: "Env"):
        return self.apply(env, **self.args)

    @abstractmethod
    def apply(self, *args, **kwargs):
        pass


def get_stage_name(stage: Union[Type[Stage], Stage]) -> AnyStr:
    n = stage.__class__.__name__ if isinstance(stage, Stage) else stage.__name__
    return n.replace("Stage", "")


def get_stage_type(stage: Union[Type[Stage], AnyStr]):
    from . import stages_declaration as sd

    if isinstance(stage, str):
        if not stage.startswith("Stage"):
            stage = f"Stage{stage}"
        return getattr(sd, stage)

    return stage


class Stages:
    """Stages class is used for storing all stages."""

    def __init__(self):
        self.mwm_stage: Optional[Type[Stage]] = None
        self.countries_stages: List[Type[Stage]] = []
        self.stages: List[Type[Stage]] = []
        self.helper_stages: List[Type[Stage]] = []
        self.dependencies = defaultdict(set)

    def init(self):
        # We normalize self.dependencies to Dict[Type[Stage], Set[Type[Stage]]].
        dependencies = defaultdict(set)
        for k, v in self.dependencies.items():
            dependencies[get_stage_type(k)] = set(get_stage_type(x) for x in v)
        self.dependencies = dependencies

    def set_mwm_stage(self, stage: Type[Stage]):
        assert self.mwm_stage is None
        self.mwm_stage = stage

    def add_helper_stage(self, stage: Type[Stage]):
        self.helper_stages.append(stage)

    def add_country_stage(self, stage: Type[Stage]):
        self.countries_stages.append(stage)

    def add_stage(self, stage: Type[Stage]):
        self.stages.append(stage)

    def add_dependency_for(self, stage: Type[Stage], *deps):
        for dep in deps:
            self.dependencies[stage].add(dep)

    def get_invisible_stages_names(self) -> List[AnyStr]:
        return [get_stage_name(st) for st in self.helper_stages]

    def get_visible_stages_names(self) -> List[AnyStr]:
        """Returns all stages names except helper stages names."""
        stages = []
        for s in self.stages:
            stages.append(get_stage_name(s))
            if s == self.mwm_stage:
                stages += [get_stage_name(st) for st in self.countries_stages]
        return stages

    def is_valid_stage_name(self, stage_name) -> bool:
        return get_stage_name(self.mwm_stage) == stage_name or any(
            any(stage_name == get_stage_name(x) for x in c)
            for c in [self.countries_stages, self.stages, self.helper_stages]
        )


# A global variable stage contains all possible stages.
stages = Stages()


def outer_stage(stage: Type[Stage]) -> Type[Stage]:
    """It's decorator that defines high level stage."""
    if stage.is_helper:
        stages.add_helper_stage(stage)
    else:
        stages.add_stage(stage)
        if stage.is_mwm_stage:
            stages.set_mwm_stage(stage)

    def new_apply(method):
        def apply(obj: Stage, env: "Env", *args, **kwargs):
            name = get_stage_name(obj)
            logfile = os.path.join(env.paths.log_path, f"{name}.log")
            log_handler = create_file_handler(logfile)
            logger.addHandler(log_handler)
            # This message is used as an anchor for parsing logs.
            # See maps_generator/checks/logs/logs_reader.py STAGE_START_MSG_PATTERN
            logger.info(f"Stage {name}: start ...")
            t = time.time()
            try:
                if not env.is_accepted_stage(stage):
                    logger.info(f"Stage {name} was not accepted.")
                    return

                main_status = env.main_status
                main_status.init(env.paths.main_status_path, name)
                if main_status.need_skip():
                    logger.warning(f"Stage {name} was skipped.")
                    return

                main_status.update_status()
                env.set_subprocess_out(log_handler.stream)
                method(obj, env, *args, **kwargs)
            finally:
                d = time.time() - t
                # This message is used as an anchor for parsing logs.
                # See maps_generator/checks/logs/logs_reader.py STAGE_FINISH_MSG_PATTERN
                logger.info(
                    f"Stage {name}: finished in {str(datetime.timedelta(seconds=d))}"
                )
                logger.removeHandler(log_handler)

        return apply

    stage.apply = new_apply(stage.apply)
    return stage


def country_stage_status(stage: Type[Stage]) -> Type[Stage]:
    """It's helper decorator that works with status file."""

    def new_apply(method):
        def apply(obj: Stage, env: "Env", country: AnyStr, *args, **kwargs):
            name = get_stage_name(obj)
            _logger = DummyObject()
            countries_meta = env.countries_meta
            if "logger" in countries_meta[country]:
                _logger, _ = countries_meta[country]["logger"]

            if not env.is_accepted_stage(stage):
                _logger.info(f"Stage {name} was not accepted.")
                return

            if "status" not in countries_meta[country]:
                countries_meta[country]["status"] = status.Status()

            country_status = countries_meta[country]["status"]
            status_file = os.path.join(
                env.paths.status_path, status.with_stat_ext(country)
            )
            country_status.init(status_file, name)
            if country_status.need_skip():
                _logger.warning(f"Stage {name} was skipped.")
                return

            country_status.update_status()
            method(obj, env, country, *args, **kwargs)

        return apply

    stage.apply = new_apply(stage.apply)
    return stage


def country_stage_log(stage: Type[Stage]) -> Type[Stage]:
    """It's helper decorator that works with log file."""

    def new_apply(method):
        def apply(obj: Stage, env: "Env", country: AnyStr, *args, **kwargs):
            name = get_stage_name(obj)
            log_file = os.path.join(env.paths.log_path, f"{country}.log")
            countries_meta = env.countries_meta
            if "logger" not in countries_meta[country]:
                countries_meta[country]["logger"] = create_file_logger(log_file)

            _logger, log_handler = countries_meta[country]["logger"]
            # This message is used as an anchor for parsing logs.
            # See maps_generator/checks/logs/logs_reader.py STAGE_START_MSG_PATTERN
            _logger.info(f"Stage {name}: start ...")
            t = time.time()
            env.set_subprocess_out(log_handler.stream, country)
            method(obj, env, country, *args, logger=_logger, **kwargs)
            d = time.time() - t
            # This message is used as an anchor for parsing logs.
            # See maps_generator/checks/logs/logs_reader.py STAGE_FINISH_MSG_PATTERN
            _logger.info(
                f"Stage {name}: finished in {str(datetime.timedelta(seconds=d))}"
            )

        return apply

    stage.apply = new_apply(stage.apply)
    return stage


def test_stage(*tests: Test) -> Callable[[Type[Stage],], Type[Stage]]:
    def new_apply(method):
        def apply(obj: Stage, env: "Env", *args, **kwargs):
            _logger = kwargs["logger"] if "logger" in kwargs else logger

            def run_tests(tests):
                for test in tests:
                    if test.need_run(env, _logger):
                        test.test(env, _logger, *args, **kwargs)
                    else:
                        _logger.info(f"Test {test.name} was skipped.")

            run_tests(filter(lambda t: t.is_pretest, tests))
            method(obj, env, *args, **kwargs)
            run_tests(filter(lambda t: not t.is_pretest, tests))

        return apply

    def wrapper(stage: Type[Stage]) -> Type[Stage]:
        stage.apply = new_apply(stage.apply)
        return stage

    return wrapper


def country_stage(stage: Type[Stage]) -> Type[Stage]:
    """It's decorator that defines country stage."""
    if stage.is_helper:
        stages.add_helper_stage(stage)
    else:
        stages.add_country_stage(stage)

    return country_stage_log(country_stage_status(stage))


def mwm_stage(stage: Type[Stage]) -> Type[Stage]:
    stage.is_mwm_stage = True
    return stage


def production_only(stage: Type[Stage]) -> Type[Stage]:
    stage.is_production_only = True
    return stage


def helper_stage_for(*deps) -> Callable[[Type[Stage],], Type[Stage]]:
    def wrapper(stage: Type[Stage]) -> Type[Stage]:
        stages.add_dependency_for(stage, *deps)
        stage.is_helper = True
        return stage

    return wrapper


def depends_from_internal(*deps) -> Callable[[Type[Stage],], Type[Stage]]:
    def get_urls(
        env: "Env", internal_dependencies: List[InternalDependency]
    ) -> Dict[AnyStr, AnyStr]:
        deps = {}
        for d in internal_dependencies:
            if "p" in d.mode and not env.production or not d.url:
                continue

            path = None
            if type(d.path_method) is property:
                path = d.path_method.__get__(env.paths)

            assert path is not None, type(d.path_method)
            deps[d.url] = path

        return deps

    def download_under_lock(env: "Env", urls: Dict[AnyStr, AnyStr], stage_name: AnyStr):
        lock_name = f"{os.path.join(env.paths.status_path, stage_name)}.lock"
        status_name = f"{os.path.join(env.paths.status_path, stage_name)}.download"
        with filelock.FileLock(lock_name):
            s = status.Status(status_name)
            if not s.is_finished():
                urls = normalize_url_to_path_dict(urls)
                download_files(urls, env.force_download_files)
                s.finish()

    def new_apply(method):
        def apply(obj: Stage, env: "Env", *args, **kwargs):
            if hasattr(obj, "internal_dependencies") and obj.internal_dependencies:
                urls = get_urls(env, obj.internal_dependencies)
                if urls:
                    download_under_lock(env, urls, get_stage_name(obj))

            method(obj, env, *args, **kwargs)

        return apply

    def wrapper(stage: Type[Stage]) -> Type[Stage]:
        stage.internal_dependencies = deps
        stage.apply = new_apply(stage.apply)
        return stage

    return wrapper

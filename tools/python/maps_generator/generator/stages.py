""""
This file contains some decorators that define stages.
There are two main types of stages:
    1. stage - a high level stage
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
from multiprocessing import Lock
from typing import AnyStr
from typing import Callable
from typing import List
from typing import Optional
from typing import Type
from typing import Union

from maps_generator.generator.status import Status
from maps_generator.utils.file import download_files
from maps_generator.utils.log import DummyObject
from maps_generator.utils.log import create_file_logger

logger = logging.getLogger("maps_generator")


class InternalDependency:
    def __init__(self, url, path_method, mode=""):
        self.url = url
        self.path_method = path_method
        self.mode = mode


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
            log_handler = logging.FileHandler(logfile)
            logger.addHandler(log_handler)
            if not env.is_accepted_stage(stage):
                logger.info(f"{name} was not accepted.")
                logger.removeHandler(log_handler)
                return

            main_status = env.main_status
            main_status.init(env.paths.main_status_path, name)
            if main_status.need_skip():
                logger.warning(f"{name} was skipped.")
                logger.removeHandler(log_handler)
                return

            main_status.update_status()
            logger.info(f"{name}: start ...")
            t = time.time()
            env.set_subprocess_out(log_handler.stream)
            method(obj, env, *args, **kwargs)
            d = time.time() - t
            logger.info(f"{name}: finished in " f"{str(datetime.timedelta(seconds=d))}")
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
                _logger.info(f"{name} was not accepted.")
                return

            if "status" not in countries_meta[country]:
                countries_meta[country]["status"] = Status()

            status = countries_meta[country]["status"]
            status_file = os.path.join(env.paths.status_path, f"{country}.status")
            status.init(status_file, name)
            if status.need_skip():
                _logger.warning(f"{name} was skipped.")
                return

            status.update_status()
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
            stage_formatted = " ".join(name.split("_")).capitalize()
            _logger.info(f"{stage_formatted}: start ...")
            t = time.time()
            env.set_subprocess_out(log_handler.stream, country)
            method(obj, env, country, *args, logger=_logger, **kwargs)
            d = time.time() - t
            _logger.info(
                f"{stage_formatted}: finished in "
                f"{str(datetime.timedelta(seconds=d))}"
            )

        return apply

    stage.apply = new_apply(stage.apply)
    return stage


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
    def new_apply(method):
        def apply(obj: Stage, env: "Env", *args, **kwargs):
            if hasattr(obj, "internal_dependencies") and obj.internal_dependencies:
                with obj.depends_from_internal_lock:
                    if not obj.depends_from_internal_downloaded:
                        deps = {}
                        for d in obj.internal_dependencies:
                            if "p" in d.mode and not env.production:
                                continue

                            path = None
                            if type(d.path_method) is property:
                                path = d.path_method.__get__(env.paths)

                            assert path is not None, type(d.path_method)
                            deps[d.url] = path

                        if deps:
                            download_files(deps, env.force_download_files)

                        obj.depends_from_internal_downloaded = True

            method(obj, env, *args, **kwargs)

        return apply

    def wrapper(stage: Type[Stage]) -> Type[Stage]:
        stage.internal_dependencies = deps
        stage.depends_from_internal_lock = Lock()
        stage.depends_from_internal_downloaded = False
        stage.apply = new_apply(stage.apply)
        return stage

    return wrapper

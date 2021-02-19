""""
This file contains possible stages that maps_generator can run.
Some algorithms suppose a maps genration processes looks like:
    stage1, ..., stage_mwm[country_stage_1, ..., country_stage_M], ..., stageN
Only stage_mwm can contain country_
"""
import datetime
import json
import logging
import multiprocessing
import os
import shutil
import tarfile
from collections import defaultdict
from multiprocessing.pool import ThreadPool
from typing import AnyStr
from typing import Type

import maps_generator.generator.diffs as diffs
import maps_generator.generator.stages_tests as st
from descriptions.descriptions_downloader import check_and_get_checker
from descriptions.descriptions_downloader import download_from_wikidata_tags
from descriptions.descriptions_downloader import download_from_wikipedia_tags
from maps_generator.generator import coastline
from maps_generator.generator import settings
from maps_generator.generator import steps
from maps_generator.generator.env import Env
from maps_generator.generator.env import PathProvider
from maps_generator.generator.env import WORLD_COASTS_NAME
from maps_generator.generator.env import WORLD_NAME
from maps_generator.generator.exceptions import BadExitStatusError
from maps_generator.generator.gen_tool import run_gen_tool
from maps_generator.generator.stages import InternalDependency as D
from maps_generator.generator.stages import Stage
from maps_generator.generator.stages import Test
from maps_generator.generator.stages import country_stage
from maps_generator.generator.stages import depends_from_internal
from maps_generator.generator.stages import helper_stage_for
from maps_generator.generator.stages import mwm_stage
from maps_generator.generator.stages import outer_stage
from maps_generator.generator.stages import production_only
from maps_generator.generator.stages import stages
from maps_generator.generator.stages import test_stage
from maps_generator.generator.statistics import get_stages_info
from maps_generator.utils.file import download_files
from maps_generator.utils.file import is_verified
from post_generation.hierarchy_to_countries import hierarchy_to_countries
from post_generation.inject_promo_ids import inject_promo_ids


@outer_stage
class StageStatistics(Stage):
    def apply(self, env: Env):
        steps_info = get_stages_info(env.paths.log_path, {"statistics"})
        stats = defaultdict(lambda: defaultdict(dict))
        stats["steps"] = steps_info["steps"]
        for country in env.get_tmp_mwm_names():
            with open(os.path.join(env.paths.stats_path, f"{country}.json")) as f:
                stats["countries"][country] = {
                    "types": json.load(f),
                    "steps": steps_info["countries"][country],
                }

        def default(o):
            if isinstance(o, datetime.timedelta):
                return str(o)

        with open(os.path.join(env.paths.stats_path, "stats.json"), "w") as f:
            json.dump(
                stats, f, ensure_ascii=False, sort_keys=True, indent=2, default=default
            )


@outer_stage
class StageCleanup(Stage):
    def apply(self, env: Env):
        logger.info(
            f"osm2ft files will be moved from {env.paths.mwm_path} "
            f"to {env.paths.osm2ft_path}."
        )
        for x in os.listdir(env.paths.mwm_path):
            p = os.path.join(env.paths.mwm_path, x)
            if os.path.isfile(p) and x.endswith(".mwm.osm2ft"):
                shutil.move(p, os.path.join(env.paths.osm2ft_path, x))

        logger.info(f"{env.paths.draft_path} will be removed.")
        shutil.rmtree(env.paths.draft_path)


stages.init()

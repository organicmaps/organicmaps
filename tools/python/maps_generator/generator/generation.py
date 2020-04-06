import os
from typing import AnyStr
from typing import List
from typing import Optional
from typing import Type
from typing import Union

import filelock

from maps_generator.generator.env import Env
from maps_generator.generator.exceptions import ContinueError
from maps_generator.generator.stages import Stage
from maps_generator.generator.stages import get_stage_name
from maps_generator.generator.stages import stages
from maps_generator.generator.status import Status


class Generation:
    """
    Generation describes process of a map generation. It contains stages.

    For example:
      generation = Generation(env)
      generation.add_stage(s1)
      generation.add_stage(s2)
      generation.run()
    """

    def __init__(self, env: Env):
        self.env: Env = env
        self.stages: List[Stage] = []
        self.runnable_stages: Optional[List[Stage]] = None

        for country_stage in stages.countries_stages:
            if self.is_skipped_stage(country_stage):
                self.env.add_skipped_stage(country_stage)

    def is_skipped_stage(self, stage: Union[Type[Stage], Stage]) -> bool:
        return stage.is_production_only and not self.env.production

    def add_stage(self, stage: Stage):
        self.stages.append(stage)
        if self.is_skipped_stage(stage):
            self.env.add_skipped_stage(stage)

    def pre_run(self):
        skipped = set()

        def traverse(current: Type[Stage]):
            deps = stages.dependencies.get(current, [])
            for d in deps:
                skipped.add(d)
                traverse(d)

        for skipped_stage in self.env.skipped_stages:
            traverse(skipped_stage)

        for s in skipped:
            self.env.add_skipped_stage(s)

        self.runnable_stages = [s for s in self.stages if self.env.is_accepted_stage(s)]

    def run(self, from_stage: Optional[AnyStr] = None):
        self.pre_run()
        if from_stage is not None:
            self.reset_to_stage(from_stage)

        lock_filename = f"{os.path.join(self.env.paths.build_path, 'lock')}.lock"
        with filelock.FileLock(lock_filename):
            for stage in self.runnable_stages:
                stage(self.env)

    def reset_to_stage(self, stage_name: AnyStr):
        """
        Resets generation state to stage_name.
        Status files are overwritten new statuses according stage_name.
        It supposes that stages have next representation:
          stage1, ..., stage_mwm[country_stage_1, ..., country_stage_M], ..., stageN
        """
        countries_statuses_paths = [
            os.path.join(self.env.paths.status_path, f)
            for f in os.listdir(self.env.paths.status_path)
            if os.path.isfile(os.path.join(self.env.paths.status_path, f))
            and os.path.join(self.env.paths.status_path, f)
            != self.env.paths.main_status_path
        ]

        def set_countries_stage(st):
            for path in countries_statuses_paths:
                Status(path, st).update_status()

        def finish_countries_stage():
            for path in countries_statuses_paths:
                Status(path).finish()

        high_level_stages = [get_stage_name(s) for s in self.runnable_stages]
        if not (
            stage_name in high_level_stages
            or any(stage_name == get_stage_name(s) for s in stages.countries_stages)
        ):
            raise ContinueError(f"{stage_name} not in {', '.join(high_level_stages)}.")

        if not os.path.exists(self.env.paths.main_status_path):
            raise ContinueError(
                f"Status file {self.env.paths.main_status_path} not found."
            )

        if not os.path.exists(self.env.paths.status_path):
            raise ContinueError(f"Status path {self.env.paths.status_path} not found.")

        mwm_stage_name = get_stage_name(stages.mwm_stage)
        stage_mwm_index = high_level_stages.index(mwm_stage_name)

        main_status = None
        if stage_name in high_level_stages[: stage_mwm_index + 1]:
            main_status = stage_name
            set_countries_stage("")
        elif stage_name in high_level_stages[stage_mwm_index + 1 :]:
            main_status = stage_name
            finish_countries_stage()
        else:
            main_status = get_stage_name(stages.mwm_stage)
            set_countries_stage(stage_name)

        Status(self.env.paths.main_status_path, main_status).update_status()

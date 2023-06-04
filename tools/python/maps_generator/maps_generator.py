import logging
from typing import AnyStr
from typing import Iterable
from typing import Optional

from maps_generator.generator import stages_declaration as sd
from maps_generator.generator.env import Env
from maps_generator.generator.generation import Generation
from .generator.stages import Stage

logger = logging.getLogger("maps_generator")


def run_generation(
    env: Env,
    stages: Iterable[Stage],
    from_stage: Optional[AnyStr] = None,
    build_lock: bool = True,
):
    generation = Generation(env, build_lock)
    for s in stages:
        generation.add_stage(s)

    generation.run(from_stage)


def generate_maps(env: Env, from_stage: Optional[AnyStr] = None):
    """"Runs maps generation."""
    stages = (
        sd.StageDownloadAndConvertPlanet(),
        sd.StageUpdatePlanet(),
        sd.StageCoastline(),
        sd.StagePreprocess(),
        sd.StageFeatures(),
        sd.StageDownloadDescriptions(),
        sd.StageMwm(),
        sd.StageCountriesTxt(),
        sd.StageLocalAds(),
        sd.StageStatistics(),
        sd.StageCleanup(),
    )

    run_generation(env, stages, from_stage)


def generate_coasts(env: Env, from_stage: Optional[AnyStr] = None):
    """Runs coasts generation."""
    stages = (
        sd.StageDownloadAndConvertPlanet(),
        sd.StageUpdatePlanet(),
        sd.StageCoastline(use_old_if_fail=False),
        sd.StageCleanup(),
    )

    run_generation(env, stages, from_stage)

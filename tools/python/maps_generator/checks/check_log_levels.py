import logging
from functools import lru_cache

from maps_generator.checks import check
from maps_generator.checks.logs import logs_reader
from maps_generator.generator.stages_declaration import stages


@lru_cache(maxsize=None)
def _get_log_stages(path):
    log = logs_reader.Log(path)
    return logs_reader.normalize_logs(logs_reader.split_into_stages(log))


def get_log_levels_check_set(old_path: str, new_path: str) -> check.CompareCheckSet:
    """
    Returns a log levels check set, that checks a difference in a number of
    message levels from warning and higher for each stage between old mwms
    and new mwms.
    """
    cs = check.CompareCheckSet("Log levels check")

    def make_do(level, stage_name, cache={}):
        def do(path):
            for s in _get_log_stages(path):
                if s.name == stage_name:
                    k = f"{path}:{stage_name}"
                    if k not in cache:
                        cache[k] = logs_reader.count_levels(s)

                    return cache[k][level]
            return None

        return do

    for stage_name in (
        stages.get_visible_stages_names() + stages.get_invisible_stages_names()
    ):
        for level in (logging.CRITICAL, logging.ERROR, logging.WARNING):
            cs.add_check(
                check.build_check_set_for_files(
                    f"Stage {stage_name} - {logging.getLevelName(level)} check",
                    old_path,
                    new_path,
                    ext=".log",
                    do=make_do(level, stage_name),
                )
            )
    return cs

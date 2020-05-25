import os

from maps_generator.checks import check


def get_size_check_set(old_path: str, new_path: str) -> check.CompareCheckSet:
    return check.build_check_set_for_files(
        "Size check",
        old_path,
        new_path,
        ext=".mwm",
        do=lambda path: os.path.getsize(path),
    )

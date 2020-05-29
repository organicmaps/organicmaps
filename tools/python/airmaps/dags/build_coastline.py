import logging
import os
import shutil
from datetime import timedelta

from airflow import DAG
from airflow.operators.python_operator import PythonOperator
from airflow.utils.dates import days_ago

from airmaps.instruments import settings
from airmaps.instruments import storage
from airmaps.instruments.utils import get_latest_filename
from airmaps.instruments.utils import make_rm_build_task
from airmaps.instruments.utils import put_current_date_in_filename
from airmaps.instruments.utils import rm_build
from maps_generator.generator import stages_declaration as sd
from maps_generator.generator.env import Env
from maps_generator.generator.env import WORLD_COASTS_NAME
from maps_generator.maps_generator import run_generation

logger = logging.getLogger("airmaps")


DAG = DAG(
    "Build_coastline",
    schedule_interval=timedelta(days=1),
    default_args={
        "owner": "MAPS.ME",
        "depends_on_past": True,
        "start_date": days_ago(0),
        "email": settings.EMAILS,
        "email_on_failure": True,
        "email_on_retry": False,
        "retries": 0,
        "retry_delay": timedelta(minutes=5),
        "priority_weight": 1,
    },
)

COASTLINE_STORAGE_PATH = f"{settings.STORAGE_PREFIX}/coasts"


def publish_coastline(**kwargs):
    build_name = kwargs["ti"].xcom_pull(key="build_name")
    env = Env(build_name=build_name)
    for name in (f"{WORLD_COASTS_NAME}.geom", f"{WORLD_COASTS_NAME}.rawgeom"):
        coastline = put_current_date_in_filename(name)
        latest = get_latest_filename(name)
        coastline_full = os.path.join(env.paths.coastline_path, coastline)
        latest_full = os.path.join(env.paths.coastline_path, latest)
        shutil.move(os.path.join(env.paths.coastline_path, name), coastline_full)
        os.symlink(coastline, latest_full)

        storage.wd_publish(coastline_full, f"{COASTLINE_STORAGE_PATH}/{coastline}")
        storage.wd_publish(latest_full, f"{COASTLINE_STORAGE_PATH}/{latest}")


def build_coastline(**kwargs):
    env = Env()
    kwargs["ti"].xcom_push(key="build_name", value=env.build_name)

    run_generation(
        env,
        (
            sd.StageDownloadAndConvertPlanet(),
            sd.StageCoastline(use_old_if_fail=False),
            sd.StageCleanup(),
        ),
    )
    env.finish()


BUILD_COASTLINE_TASK = PythonOperator(
    task_id="Build_coastline_task",
    provide_context=True,
    python_callable=build_coastline,
    on_failure_callback=lambda ctx: rm_build(**ctx),
    dag=DAG,
)


PUBLISH_COASTLINE_TASK = PythonOperator(
    task_id="Publish_coastline_task",
    provide_context=True,
    python_callable=publish_coastline,
    dag=DAG,
)


RM_BUILD_TASK = make_rm_build_task(DAG)


BUILD_COASTLINE_TASK >> PUBLISH_COASTLINE_TASK >> RM_BUILD_TASK

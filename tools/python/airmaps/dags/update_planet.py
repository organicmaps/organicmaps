import logging
from datetime import timedelta

from airflow import DAG
from airflow.operators.python_operator import PythonOperator
from airflow.utils.dates import days_ago

from airmaps.instruments import settings
from airmaps.instruments import storage
from airmaps.instruments.utils import make_rm_build_task
from maps_generator.generator import stages_declaration as sd
from maps_generator.generator.env import Env
from maps_generator.maps_generator import run_generation
from maps_generator.utils.md5 import md5_ext

logger = logging.getLogger("airmaps")


DAG = DAG(
    "Update_planet",
    schedule_interval=timedelta(days=1),
    default_args={
        "owner": "OMaps",
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


PLANET_STORAGE_PATH = f"{settings.STORAGE_PREFIX}/planet_regular/planet-latest.o5m"


def update_planet(**kwargs):
    env = Env()
    kwargs["ti"].xcom_push(key="build_name", value=env.build_name)

    if settings.DEBUG:
        env.add_skipped_stage(sd.StageUpdatePlanet)

    run_generation(
        env,
        (
            sd.StageDownloadAndConvertPlanet(),
            sd.StageUpdatePlanet(),
            sd.StageCleanup(),
        ),
    )
    env.finish()


def publish_planet(**kwargs):
    build_name = kwargs["ti"].xcom_pull(key="build_name")
    env = Env(build_name=build_name)
    storage.wd_publish(env.paths.planet_o5m, PLANET_STORAGE_PATH)
    storage.wd_publish(md5_ext(env.paths.planet_o5m), md5_ext(PLANET_STORAGE_PATH))


UPDATE_PLANET_TASK = PythonOperator(
    task_id="Update_planet_task",
    provide_context=True,
    python_callable=update_planet,
    dag=DAG,
)


PUBLISH_PLANET_TASK = PythonOperator(
    task_id="Publish_planet_task",
    provide_context=True,
    python_callable=publish_planet,
    dag=DAG,
)


RM_BUILD_TASK = make_rm_build_task(DAG)


UPDATE_PLANET_TASK >> PUBLISH_PLANET_TASK >> RM_BUILD_TASK

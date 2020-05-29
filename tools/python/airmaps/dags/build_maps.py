import logging
from datetime import timedelta

from airflow import DAG
from airflow.operators.python_operator import PythonOperator
from airflow.utils.dates import days_ago

from airmaps.instruments import settings
from airmaps.instruments import storage
from airmaps.instruments.utils import make_rm_build_task
from airmaps.instruments.utils import run_generation_from_first_stage
from maps_generator.generator import stages_declaration as sd
from maps_generator.generator.env import Env
from maps_generator.generator.env import PathProvider
from maps_generator.generator.env import get_all_countries_list
from maps_generator.maps_generator import run_generation

logger = logging.getLogger("airmaps")


MAPS_STORAGE_PATH = f"{settings.STORAGE_PREFIX}/maps"


class MapsGenerationDAG(DAG):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        build_prolog_task = PythonOperator(
            task_id="Build_prolog_task",
            provide_context=True,
            python_callable=MapsGenerationDAG.build_prolog,
            dag=self,
        )

        build_epilog_task = PythonOperator(
            task_id="Build_epilog_task",
            provide_context=True,
            python_callable=MapsGenerationDAG.build_epilog,
            dag=self,
        )

        publish_maps_task = PythonOperator(
            task_id="Publish_maps_task",
            provide_context=True,
            python_callable=MapsGenerationDAG.publish_maps,
            dag=self,
        )

        rm_build_task = make_rm_build_task(self)

        build_epilog_task >> publish_maps_task >> rm_build_task
        for country in get_all_countries_list(PathProvider.borders_path()):
            build_prolog_task >> self.make_mwm_operator(country) >> build_epilog_task

    @staticmethod
    def get_params(namespace="env", **kwargs):
        return kwargs.get("params", {}).get(namespace, {})

    @staticmethod
    def build_prolog(**kwargs):
        params = MapsGenerationDAG.get_params(**kwargs)
        env = Env(**params)
        kwargs["ti"].xcom_push(key="build_name", value=env.build_name)
        run_generation(
            env,
            (
                sd.StageDownloadAndConvertPlanet(),
                sd.StageCoastline(),
                sd.StagePreprocess(),
                sd.StageFeatures(),
                sd.StageDownloadDescriptions(),
            ),
        )

    @staticmethod
    def make_build_mwm_func(country):
        def build_mwm(**kwargs):
            build_name = kwargs["ti"].xcom_pull(key="build_name")
            params = MapsGenerationDAG.get_params(**kwargs)
            params.update({"build_name": build_name, "countries": [country,]})
            env = Env(**params)
            # We need to check existing of mwm.tmp. It is needed if we want to
            # build mwms from part of planet.
            tmp_mwm_name = env.get_tmp_mwm_names()
            assert len(tmp_mwm_name) <= 1
            if not tmp_mwm_name:
                logger.warning(f"mwm.tmp does not exist for {country}.")
                return

            run_generation_from_first_stage(env, (sd.StageMwm(),), build_lock=False)

        return build_mwm

    @staticmethod
    def build_epilog(**kwargs):
        build_name = kwargs["ti"].xcom_pull(key="build_name")
        params = MapsGenerationDAG.get_params(**kwargs)
        params.update({"build_name": build_name})
        env = Env(**params)
        run_generation_from_first_stage(
            env,
            (
                sd.StageCountriesTxt(),
                sd.StageExternalResources(),
                sd.StageLocalAds(),
                sd.StageStatistics(),
                sd.StageCleanup(),
            ),
        )
        env.finish()

    @staticmethod
    def publish_maps(**kwargs):
        build_name = kwargs["ti"].xcom_pull(key="build_name")
        params = MapsGenerationDAG.get_params(**kwargs)
        params.update({"build_name": build_name})
        env = Env(**params)
        subdir = MapsGenerationDAG.get_params(namespace="storage", **kwargs)["subdir"]
        storage_path = f"{MAPS_STORAGE_PATH}/{subdir}"
        storage.wd_publish(env.paths.mwm_path, f"{storage_path}/{env.mwm_version}/")

    def make_mwm_operator(self, country):
        normalized_name = "__".join(country.lower().split())
        return PythonOperator(
            task_id=f"Build_country_{normalized_name}_task",
            provide_context=True,
            python_callable=MapsGenerationDAG.make_build_mwm_func(country),
            dag=self,
        )


PARAMS = {"storage": {"subdir": "open_source"}}
if settings.DEBUG:
    PARAMS["env"] = {
        # The planet file in debug mode does not contain Russia_Moscow territory.
        # It is needed for testing.
        "countries": ["Cuba", "Haiti", "Jamaica", "Cayman Islands", "Russia_Moscow"]
    }

OPEN_SOURCE_MAPS_GENERATION_DAG = MapsGenerationDAG(
    "Generate_open_source_maps",
    schedule_interval=timedelta(days=7),
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
        "params": PARAMS,
    },
)

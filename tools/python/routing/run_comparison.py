import sys
import argparse
import os

from src.utils import Omim
from src.utils import ConfigINI
from src import utils
from src.logger import LOG


def run_routes_builder_tool(*, omim, config_ini, versions):
    is_benchmark = config_ini.read_value_by_path(path=['TOOL', 'Benchmark'])
    args = {
        'resources_path': omim.data_path,
        'routes_file': config_ini.read_value_by_path(path=['PATHS', 'RoutesFile']),
        'threads': 1 if is_benchmark else utils.cpu_count(),
        'timeout': config_ini.read_value_by_path(path=['TOOL', 'Timeout']),
        'launches_number': config_ini.read_value_by_path(path=['TOOL', 'LaunchesNumber']) if is_benchmark else 1,
        'vehicle_type': utils.get_vehicle_type(config_ini=config_ini)
    }

    data_from_routes_builder_tool = dict()

    for version in versions:
        name = version['name']
        branch = version['branch']
        hash = version['hash']

        branch_hash = utils.get_branch_hash_name(branch=branch, hash=hash)
        LOG.info(f'Get: {name} {branch_hash}')

        omim.checkout(branch=branch, hash=hash)
        omim.build(aim='routes_builder_tool', binary_cache_suffix='cpu' if is_benchmark else '')

        args['dump_path'] = omim.get_or_create_unique_dir_path(
            prefix_path=config_ini.read_value_by_path(path=['PATHS', 'ResultsSaveDir']))

        args['data_path'] = version['mwm_path']

        data_from_routes_builder_tool[branch_hash] = args.copy()

        utils.log_with_stars('CPP Logs')
        omim.run(binary='routes_builder_tool', binary_cache_suffix='cpu' if is_benchmark else '', args=args)

        LOG.info(os.linesep, os.linesep)

    return data_from_routes_builder_tool


def run_routing_quality_tool(*, omim, config_ini, old_versions, new_versions, data_from_routes_builder_tool):
    args = {}
    is_benchmark = config_ini.read_value_by_path(path=['TOOL', 'Benchmark'])
    if is_benchmark:
        args['benchmark_stat'] = ''

    results_base_dir = config_ini.read_value_by_path(path=['PATHS', 'ResultsSaveDir'])

    for old_version in old_versions:
        for new_version in new_versions:
            utils.log_with_stars(f'Compare {old_version["name"]} VS {new_version["name"]}')
            old_branch_hash = utils.get_branch_hash_name(branch=old_version['branch'], hash=old_version['hash'])
            old_version_task = data_from_routes_builder_tool[old_branch_hash]

            new_branch_hash = utils.get_branch_hash_name(branch=new_version['branch'], hash=new_version['hash'])
            new_version_task = data_from_routes_builder_tool[new_branch_hash]

            args['mapsme_results'] = new_version_task['dump_path']
            args['mapsme_old_results'] = old_version_task['dump_path']
            args['save_results'] = os.path.join(results_base_dir, old_branch_hash + '__vs__' + new_branch_hash)

            omim.run(binary='routing_quality_tool', args=args)
            LOG.info(os.linesep, os.linesep)


def main():
    config_ini = ConfigINI('etc/comparison.ini')

    old_versions = utils.load_run_config_ini(config_ini=config_ini, path=["OLD_VERSION", "Params"])
    new_versions = utils.load_run_config_ini(config_ini=config_ini, path=["NEW_VERSION", "Params"])

    all_versions = []
    all_versions.extend(old_versions)
    all_versions.extend(new_versions)

    omim = Omim()

    try:
        # Run sequentially all versions of routing and dumps results to some directory
        # based on ResultsSaveDir from config_ini file.
        data_from_routes_builder_tool = run_routes_builder_tool(omim=omim, config_ini=config_ini, versions=all_versions)
    except Exception as e:
        LOG.info(f'Error in run_routes_builder_tool(): {e}')
        omim.checkout_to_init_state()
        sys.exit()

    LOG.info("run_routes_builder_tool() Done.")

    omim.checkout(branch='master')
    omim.build(aim='routing_quality_tool')

    try:
        # Run routing_quality_tool, which compares results of routes_builder_tool.
        run_routing_quality_tool(omim=omim, config_ini=config_ini, old_versions=old_versions, new_versions=new_versions,
                                 data_from_routes_builder_tool=data_from_routes_builder_tool)
    except Exception as e:
        LOG.error(f'Error in run_routing_quality_tool(): {e}')
        omim.checkout_to_init_state()
        sys.exit()

    omim.checkout_to_init_state()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument('--run',
                        help='Make sure that you have been created `etc/comparison.ini` and `etc/paths.ini` before '
                             'running. Look to `etc/comparison.ini.example` and `etc/paths.ini.example` for example. '
                             'If you are sure, run this script with --run option.'
                             'Look to: https://confluence.mail.ru/display/MAPSME/Python+Tools for more details.',
                        action='store_true', default=False)

    args = parser.parse_args()
    if args.run:
        main()
    else:
        parser.print_help()


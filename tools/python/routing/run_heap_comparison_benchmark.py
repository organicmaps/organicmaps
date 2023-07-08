import os
import sys
import re
from multiprocessing import Pool
import argparse

from src.utils import Omim
from src.utils import ConfigINI
from src.logger import LOG
from src import graph_scripts, utils


def get_version_dump_path(*, config_ini, version):
    results_save_dir = config_ini.read_value_by_path(path=['PATHS', 'ResultsSaveDir'])
    branch_hash = utils.get_branch_hash_name(branch=version['branch'], hash=version['hash'])
    return os.path.join(results_save_dir, branch_hash)


def get_version_heapprof_dump_path(*, config_ini, version):
    version_dump_path = get_version_dump_path(config_ini=config_ini, version=version)
    return os.path.join(version_dump_path, 'heap_prof')


def create_file_with_line(*, file, line):
    with open(file, 'w') as f:
        f.write(line)


def get_max_mb_usage(*, log_file):
    max_mb = 0
    with open(log_file) as log_file_fh:
        for line in log_file_fh:
            matched = re.match(r'.*[(\s](\d+) MB currently in use.*', line)
            if matched is None:
                continue
            mb = int(matched.group(1))
            max_mb = max(max_mb, mb)

    return max_mb


def run_routes_builder_tool_one_route(omim, version, config_ini, id, route_line):
    dump_path = get_version_dump_path(config_ini=config_ini, version=version)
    routes_file = os.path.join(dump_path, str(id) + '.route')
    output_file = os.path.join(dump_path, str(id) + '.log')
    create_file_with_line(file=routes_file, line=route_line)
    heap_prof_path = os.path.join(get_version_heapprof_dump_path(config_ini=config_ini, version=version),
                                  str(id) + '.hprof')

    args = {
        'resources_path': omim.data_path,
        'routes_file': routes_file,
        'threads': 1,
        'timeout': config_ini.read_value_by_path(path=['TOOL', 'Timeout']),
        'verbose': '',
        'data_path': version['mwm_path'],
        'dump_path': dump_path,
        'vehicle_type': utils.get_vehicle_type(config_ini=config_ini)
    }

    env = {
        'HEAP_PROFILE_INUSE_INTERVAL': 1024 * 1024,  # Kb
        'HEAPPROFILE': heap_prof_path
    }

    omim.run(binary='routes_builder_tool', binary_cache_suffix='heap', args=args, env=env, output_file=output_file,
             log_error_code=False)
    return {'id': id,
            'max_mb_usage': get_max_mb_usage(log_file=output_file)}


def run_heap_comparison(*, omim, config_ini, versions):
    routes_file = config_ini.read_value_by_path(path=['PATHS', 'RoutesFile'])
    data_from_heap_comparison = dict()

    for version in versions:
        name = version['name']
        branch = version['branch']
        hash = version['hash']

        branch_hash = utils.get_branch_hash_name(branch=branch, hash=hash)

        version_dump_path = get_version_dump_path(config_ini=config_ini, version=version)
        heapprof_dump_path = get_version_heapprof_dump_path(config_ini=config_ini, version=version)
        if not os.path.exists(version_dump_path):
            os.mkdir(version_dump_path)

        if not os.path.exists(heapprof_dump_path):
            os.mkdir(heapprof_dump_path)

        LOG.info(f'Get: {name} {branch_hash}')

        omim.checkout(branch=branch, hash=hash)
        omim.build(aim='routes_builder_tool', binary_cache_suffix='heap', cmake_options="-DUSE_HEAPPROF=ON")

        LOG.info(f'Start build routes from file: {routes_file}')
        pool_args = []
        with open(routes_file) as routes_file_fh:
            for route_id, line in enumerate(routes_file_fh):
                args_tuple = (omim, version, config_ini, route_id, line)
                pool_args.append(args_tuple)

        with Pool(omim.cpu_count) as p:
            version_result = p.starmap(run_routes_builder_tool_one_route, pool_args)

            results = dict()
            for result in version_result:
                results[result['id']] = result['max_mb_usage']

            data_from_heap_comparison[branch_hash] = {
                'version': version,
                'results': results
            }

        LOG.info(os.linesep, os.linesep)

    return data_from_heap_comparison


def get_by_func_and_key_in_diff(*, diff, key, func):
    value = 0
    for item in diff:
        value = func(value, item[key])
    return value


def get_median_by_key_in_diff(*, diff, key):
    values = [item[key] for item in diff]
    values.sort()
    return values[len(values) // 2]


def create_diff_mb_percents_plots(diff):
    plots = []

    percents_in_conventional_units = 5
    mbs_in_conventional_units = 10

    diff_percent = list(map(lambda item: item['percent'] / percents_in_conventional_units, diff))
    diff_mb = list(map(lambda item: item['mb'] / mbs_in_conventional_units, diff))

    x_list = list(range(0, len(diff)))

    plots.append({
        'legend': f'Diff megabytes in conventional units = {percents_in_conventional_units}',
        'points_x': x_list,
        'points_y': diff_mb
    })

    plots.append({
        'legend': f'Diff percents in conventional units = {mbs_in_conventional_units}',
        'points_x': x_list,
        'points_y': diff_percent
    })

    return plots


# Calculate some stat comparison about maximum memory usage in two versions, creates some plots to look at this stat
# by eyes.
def compare_two_versions(*, config_ini, old_version_data, new_version_data):
    old_version = old_version_data['version']
    new_version = new_version_data['version']

    old_version_name = old_version['name']
    new_version_name = new_version['name']

    results_save_dir = config_ini.read_value_by_path(path=['PATHS', 'ResultsSaveDir'])
    results_path_prefix = os.path.join(results_save_dir, f'{old_version_name}__{new_version_name}')

    utils.log_with_stars(f'Compare {old_version_name} VS {new_version_name}')

    diff = []

    old_version_results = old_version_data['results']
    new_version_results = new_version_data['results']
    for route_id, old_max_mb in old_version_results.items():
        if route_id not in new_version_results:
            LOG.info(f'Cannot find: {route_id} route in {new_version_name} results.')
            continue

        new_max_mb = new_version_results[route_id]

        diff_mb = new_max_mb - old_max_mb
        diff_percent = round((new_max_mb - old_max_mb) / old_max_mb * 100.0, 2)
        diff.append({'mb': diff_mb, 'percent': diff_percent})

    diff.sort(key=lambda item: item['mb'])

    min_mb = get_by_func_and_key_in_diff(diff=diff, key='mb', func=min)
    median_mb = get_median_by_key_in_diff(diff=diff, key='mb')
    max_mb = get_by_func_and_key_in_diff(diff=diff, key='mb', func=max)

    min_percent = get_by_func_and_key_in_diff(diff=diff, key='percent', func=min)
    median_percent = get_median_by_key_in_diff(diff=diff, key='percent')
    max_percent = get_by_func_and_key_in_diff(diff=diff, key='percent', func=max)

    LOG.info(f'Next semantic is used: {old_version_name} - {new_version_name}')
    LOG.info(f'min({min_mb}Mb), median({median_mb}Mb) max({max_mb}Mb)')
    LOG.info(f'min({min_percent}%), median({median_percent}%) max({max_percent}%)')

    diff_mb_script = f'{results_path_prefix}__diff_mb.py'
    diff_mb = list(map(lambda item: item['mb'], diff))
    graph_scripts.create_distribution_script(values=diff_mb, title='Difference MB', save_path=diff_mb_script)

    plots_script = f'{results_path_prefix}__mb_percents.py'
    plots = create_diff_mb_percents_plots(diff)
    graph_scripts.create_plots(plots=plots, xlabel='Route number', ylabel='conventional units', save_path=plots_script)

    LOG.info(os.linesep, os.linesep)

def run_results_comparison(*, config_ini, old_versions, new_versions, data_from_heap_comparison):
    for old_version in old_versions:
        for new_version in new_versions:
            old_branch_hash = utils.get_branch_hash_name(branch=old_version['branch'], hash=old_version['hash'])
            new_branch_hash = utils.get_branch_hash_name(branch=new_version['branch'], hash=new_version['hash'])

            compare_two_versions(config_ini=config_ini,
                                 old_version_data=data_from_heap_comparison[old_branch_hash],
                                 new_version_data=data_from_heap_comparison[new_branch_hash])


def main():
    config_ini = ConfigINI('etc/heap_comparison_benchmark.ini')

    old_versions = utils.load_run_config_ini(config_ini=config_ini, path=["OLD_VERSION", "Params"])
    new_versions = utils.load_run_config_ini(config_ini=config_ini, path=["NEW_VERSION", "Params"])

    all_versions = []
    all_versions.extend(old_versions)
    all_versions.extend(new_versions)

    omim = Omim()

    try:
        # Run routes_builder_tool each route separately, save maximum memory usage for all versions.
        data_from_heap_comparison = run_heap_comparison(omim=omim, config_ini=config_ini, versions=all_versions)
        LOG.info("run_heap_comparison() Done.")

        utils.log_with_stars("Run run_results_comparison()")
        # Compare each version from |old_versions| to each version from |new_versions|, dump some useful info to log.
        # Like: maximum memory usage (mb), median, average, min etc.
        run_results_comparison(config_ini=config_ini,
                               old_versions=old_versions, new_versions=new_versions,
                               data_from_heap_comparison=data_from_heap_comparison)
    except Exception as e:
        LOG.error(f'Error in run_heap_comparison(): {e}')
        omim.checkout_to_init_state()
        sys.exit()

    omim.checkout_to_init_state()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument('--run',
                        help='Make sure that you created `etc/heap_comparison_benchmark.ini` and `etc/paths.ini` before '
                             'running. Look to `etc/heap_comparison_benchmark.ini.example` and `etc/paths.ini.example` for example. '
                             'If you are sure, run this script with --run option.'
                             'Look to: https://confluence.mail.ru/display/MAPSME/Python+Tools for more details.',
                        action='store_true', default=False)

    args = parser.parse_args()
    if args.run:
        main()
    else:
        parser.print_help()

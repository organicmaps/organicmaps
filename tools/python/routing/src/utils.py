import configparser
import multiprocessing
import os
import json

from git import Repo

from src.logger import LOG
from src.common_config import CommonConfig


def load_run_config_ini(*, config_ini, path):
    value = config_ini.read_value_by_path(path=path)
    data = json.loads(value)
    for param in data:
        if 'hash' not in param:
            param['hash'] = None
    return data


def get_cmake_cmd():
    for cmd in CommonConfig.CMAKE_BINS:
        if not os.system(f'{cmd} --version > /dev/null 2>&1'):
            return cmd
    raise Exception(f'Cannot find cmake cmd, try: {CommonConfig.CMAKE_BINS}')


def log_with_stars(string):
    LOG.info('')
    LOG.info('')
    stars_number = 96
    LOG.info('*' * 96)
    first_stars = '*' * int((stars_number - len(string)) / 2)

    result = first_stars + ' ' + string + ' ' + '*' * (stars_number - len(first_stars) - len(string) - 2)

    LOG.info(result)
    LOG.info('*' * 96)


def get_branch_hash_name(*, branch, hash):
    return branch + (f'_{hash}' if hash is not None else '')


def get_vehicle_type(*, config_ini):
    vehicle_type = config_ini.read_value_by_path(path=['TOOL', 'VehicleType'])
    if vehicle_type in CommonConfig.VEHICLE_TYPES:
        return vehicle_type
    raise Exception(f'Bad VehicleType: {vehicle_type}, only: {CommonConfig.VEHICLE_TYPES} supported')


class ConfigINI():
    def __init__(self, config_ini_path):
        self.config_ini_path = config_ini_path
        self.config_ini = configparser.ConfigParser()
        try:
            self.config_ini.read(config_ini_path)
        except:
            raise Exception(f'Cannot read {config_ini_path} file.')

    def read_value_by_path(self, *, path):
        value = self._read_config_ini_value(config_ini=self.config_ini, path=path)
        if value.lower() == 'true':
            return True
        if value.lower() == 'false':
            return False
        else:
            return value

    def _read_config_ini_value(self, *, config_ini, path):
        item = path.pop(0)
        if item not in config_ini:
            raise Exception(f'Cannot find {item} in {self.config_ini_path}.')

        if len(path) == 0:
            return config_ini[item]

        return self._read_config_ini_value(config_ini=config_ini[item], path=path)

def cpu_count():
    return multiprocessing.cpu_count()


class Omim():
    def __init__(self):
        config_ini = ConfigINI('etc/paths.ini')

        self.omim_path = config_ini.read_value_by_path(path=['PATHS', 'OmimPath'])
        self.data_path = config_ini.read_value_by_path(path=['PATHS', 'DataPath'])
        self.build_dir = config_ini.read_value_by_path(path=['PATHS', 'BuildDir'])
        self.cpu_count = cpu_count()

        repo = Repo(self.omim_path)
        self.branch = repo.active_branch.name
        self.hash = repo.head.object.hexsha

        self.init_branch = self.branch
        self.init_hash = self.hash

        self.cur_time_string = self._pretty_time_string(dt=datetime.now())
        self.cmake_cmd = get_cmake_cmd()

    @staticmethod
    def _pretty_time_string(*, dt):
        return dt.strftime('%Y_%m_%d__%H_%M_%S')

    def _run_system_unsafe(self, *, cmd, env=None, output_file=None, log_cmd=False):
        env_params = ""
        if env is None:
            env = dict()
        else:
            env_params = "env "

        for key, value in env.items():
            env_params += f'{key}={value} '

        if output_file is None:
            output = ""
        else:
            output = f'> {output_file} 2>&1'

        full_cmd = env_params + cmd + output
        if log_cmd:
            LOG.info(f'Run: {full_cmd}')
        return {os.system(full_cmd), full_cmd}

    def _run_system(self, *, cmd, env=None, output_file=None, log_cmd=False):
        result, full_cmd = self._run_system_unsafe(cmd=cmd, env=env, output_file=output_file, log_cmd=log_cmd)
        if result:
            raise Exception(f'Error during executing {full_cmd}')

    def _get_cached_binary_name(self, *, binary, binary_cache_suffix=None):
        binary_path = os.path.join(self.build_dir, f'{binary}_{self.branch}')

        if self.hash is not None:
            binary_path += f'_{self.hash}'

        if binary_cache_suffix is not None:
            binary_path += f'_{binary_cache_suffix}'

        return binary_path

    def checkout_to_init_state(self):
        self.checkout(branch=self.init_branch, hash=self.init_hash)

    def checkout(self, *, branch, hash=None):
        branch_hash_name = get_branch_hash_name(branch=branch, hash=hash)
        LOG.info(f'Do checkout to: {branch_hash_name}')
        repo = Repo(self.omim_path)
        repo.remote('origin').fetch()
        repo.git.checkout(branch)
        if hash:
            repo.git.reset('--hard', hash)
        else:
            hash = repo.head.object.hexsha

        LOG.info(f'Checkout to: {branch} {hash} done')

        self.branch = branch
        self.hash = hash

    def build(self, *, aim, binary_cache_suffix=None, cmake_options=""):
        os.chdir(self.build_dir)
        binary_path = self._get_cached_binary_name(binary=aim, binary_cache_suffix=binary_cache_suffix)
        if os.path.exists(binary_path):
            LOG.info(f'Found cached binary: {binary_path}')
            return

        branch_hash = get_branch_hash_name(branch=self.branch, hash=self.hash)
        output_prefix = os.path.join(self.build_dir, branch_hash + '_')

        cmake_cmd = f'{self.cmake_cmd} {self.omim_path} {cmake_options}'
        self._run_system(cmd=cmake_cmd, output_file=output_prefix + 'cmake_run.log', log_cmd=True)
        make_cmd = f'make -j{self.cpu_count} {aim}'
        self._run_system(cmd=make_cmd, output_file=output_prefix + 'make_run.log', log_cmd=True)
        LOG.info(f'Build {aim} done')
        self._run_system(cmd=f'cp {aim} {binary_path}')

    def run(self, *, binary, binary_cache_suffix=None, args, env=None, output_file=None, log_error_code=True):
        binary_path = self._get_cached_binary_name(binary=binary, binary_cache_suffix=binary_cache_suffix)
        if not os.path.exists(binary_path):
            raise Exception(f'Cannot find {binary_path}, did you call build()?')

        args_string = ""
        for arg, value in args.items():
            if value:
                args_string += f' --{arg}={value}'
            else:
                args_string += f' --{arg}'

        cmd = binary_path + args_string
        code, _ = self._run_system_unsafe(cmd=cmd, env=env, output_file=output_file)
        if log_error_code:
            LOG.info(f'Finish with exit code: {code}')

    def get_or_create_unique_dir_path(self, *, prefix_path):
        branch_hash_name = get_branch_hash_name(branch=self.branch, hash=self.hash)
        result_dir = os.path.join(prefix_path, f'{branch_hash_name}_{self.cur_time_string}')
        if not os.path.exists(result_dir):
            os.mkdir(result_dir)

        return result_dir

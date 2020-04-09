#!/usr/bin/env python
# -*- coding: utf-8 -*-
from contextlib import contextmanager
import multiprocessing
import os
import re
import sys

from distutils import log
from distutils.command.bdist import bdist as distutils_bdist
from distutils.command.build import build as distutils_build
from distutils.core import Command
from distutils.dir_util import mkpath, remove_tree
from distutils.file_util import copy_file
from distutils.spawn import spawn, find_executable
from distutils.sysconfig import (
    get_config_var, get_python_inc, get_python_version,
)
from distutils.version import LooseVersion

from setuptools import dist, setup as setuptools_setup
from setuptools.command.build_ext import build_ext as setuptools_build_ext
from setuptools.command.build_py import build_py as setuptools_build_py
from setuptools.command.egg_info import (
    egg_info as setuptools_egg_info,
    manifest_maker as setuptools_manifest_maker,
)
from setuptools.command.install import install as setuptools_install
from setuptools.command.install_lib import (
    install_lib as setuptools_install_lib,
)
from setuptools.extension import Extension


"""Monkey-patching to disable checking package names"""
dist.check_packages = lambda dist, attr, value: None

pyhelpers_dir = os.path.abspath(os.path.dirname(__file__))
omim_root = os.path.dirname(pyhelpers_dir)
boost_root = os.path.join(omim_root, '3party', 'boost')
boost_librarydir = os.path.join(boost_root, 'stage', 'lib')


def python_static_libdir():
    return get_config_var('LIBPL')


def python_ld_library():
    LDLIBRARY = get_config_var('LDLIBRARY')
    PYTHONFRAMEWORKPREFIX = get_config_var('PYTHONFRAMEWORKPREFIX')
    LIBDIR = get_config_var('LIBDIR')
    LIBPL = get_config_var('LIBPL')
    candidates = [
        os.path.join(PYTHONFRAMEWORKPREFIX, LDLIBRARY),
        os.path.join(LIBDIR, LDLIBRARY),
        os.path.join(LIBPL, LDLIBRARY),
    ]
    for candidate in candidates:
        if os.path.exists(candidate):
            return candidate


@contextmanager
def chdir(target_dir):
    saved_cwd = os.getcwd()
    os.chdir(target_dir)
    try:
        yield
    finally:
        os.chdir(saved_cwd)


class build(distutils_build, object):
    user_options = setuptools_build_ext.user_options + [
        ('omim-builddir=', None, 'Path to omim build directory'),
    ]

    def initialize_options(self):
        super(build, self).initialize_options()
        self.omim_builddir = os.path.join(omim_root, 'build')

    def finalize_options(self):
        self.build_base = os.path.relpath(
            os.path.join(self.omim_builddir, 'pybindings-builddir')
        )
        self.omim_builddir = os.path.abspath(self.omim_builddir)
        super(build, self).finalize_options()


class build_py(setuptools_build_py, object):
    def _get_data_files(self):
        data_files = (
            super(build_py, self)._get_data_files()
        )
        if self.distribution.include_package_data:
            ei_cmd = self.get_finalized_command('egg_info')
            output_data_files = []
            for package, src_dir, build_dir, filenames in data_files:
                filenames = list(filter(
                    lambda f: not f.startswith(ei_cmd.egg_info),
                    filenames
                ))
                output_data_files.append(
                    (package, src_dir, build_dir, filenames)
                )
        else:
            output_data_files = data_files
        return output_data_files


class bdist(distutils_bdist, object):
    user_options = setuptools_build_ext.user_options + [
        ('omim-builddir=', None, 'Path to omim build directory'),
    ]

    def initialize_options(self):
        super(bdist, self).initialize_options()
        self.omim_builddir = None

    def finalize_options(self):
        super(bdist, self).finalize_options()
        self.set_undefined_options('build',
                                   ('omim_builddir', 'omim_builddir'),
                                   )
        self.dist_dir = os.path.join(self.omim_builddir, 'pybindings-dist')


class manifest_maker(setuptools_manifest_maker, object):
    def add_defaults(self):
        super(manifest_maker, self).add_defaults()
        # Our README.md is for entire omim project, no point including it
        # into python package, so remove it.
        self.filelist.exclude('README.*')


class egg_info(setuptools_egg_info, object):
    user_options = setuptools_build_ext.user_options + [
        ('omim-builddir=', None, 'Path to omim build directory'),
    ]

    def initialize_options(self):
        super(egg_info, self).initialize_options()
        self.omim_builddir = None

    def finalize_options(self):
        self.set_undefined_options('build',
                                   ('omim_builddir', 'omim_builddir'),
                                   )
        self.egg_base = os.path.relpath(
            os.path.join(self.omim_builddir, 'pybindings-egg-info')
        )
        mkpath(self.egg_base)
        super(egg_info, self).finalize_options()

    def find_sources(self):
        """
        Copied from setuptools.command.egg_info method to override
        internal manifest_maker with our subclass

        Generate SOURCES.txt manifest file
        """
        manifest_filename = os.path.join(self.egg_info, 'SOURCES.txt')
        mm = manifest_maker(self.distribution)
        mm.manifest = manifest_filename
        mm.run()
        self.filelist = mm.filelist


class install(setuptools_install, object):
    user_options = setuptools_install.user_options + [
        ('omim-builddir=', None, 'Path to omim build directory'),
    ]

    def initialize_options(self):
        super(install, self).initialize_options()
        self.omim_builddir = None

    def finalize_options(self):
        super(install, self).finalize_options()
        self.set_undefined_options('build',
                                   ('omim_builddir', 'omim_builddir'),
                                   )


class install_lib(setuptools_install_lib, object):
    def get_exclusions(self):
        """
        A kludge to allow building all pybindings in one run
        and pack it into wheels separately.
        """
        include_ext_names = set(
            ext.name for ext in self.distribution.ext_modules
        )
        build_ext = self.get_finalized_command('build_ext')
        install_root = self.get_finalized_command('install').root or ''
        excludes = set()
        for ext_name in set(PYBINDINGS.keys()) - include_ext_names:
            excludes.add(os.path.join(
                install_root, build_ext.get_ext_filename(ext_name)
            ))
            for data_path in PYBINDINGS[ext_name].get('package_data', []):
                excludes.add(os.path.join(install_root, data_path))

        own_files = {
            os.path.join(install_root, data_path)
            for ext_name in include_ext_names
            for data_path in PYBINDINGS[ext_name].get('package_data', [])
        }
        excludes -= own_files

        return super(install_lib, self).get_exclusions() | excludes


class build_boost_python(Command, object):
    user_options = [
        ('force', 'f',
         'forcibly build boost_python library (ignore existence)'),
        ('omim-builddir=', None,
         'Path to omim build directory'),
    ]
    boolean_options = ['force']

    def initialize_options(self):
        self.force = None
        self.omim_builddir = None

    def finalize_options(self):
        self.set_undefined_options('build',
                                   ('force', 'force'),
                                   ('omim_builddir', 'omim_builddir'),
                                   )

    def get_boost_python_libname(self):
        return (
            'boost_python{}{}'
            .format(sys.version_info.major, sys.version_info.minor)
        )

    def get_boost_config_path(self):
        return os.path.join(
            self.omim_builddir,
            'python{}-config.jam'.format(get_python_version())
        )

    def configure_omim(self):
        with chdir(omim_root):
            spawn(['./configure.sh'])

    def create_boost_config(self):
        mkpath(self.omim_builddir)
        with open(self.get_boost_config_path(), 'w') as f:
            f.write('using python : {} : {} : {} : {} ;\n'.format(
                get_python_version(), sys.executable,
                get_python_inc(), python_static_libdir()
            ))

    def get_boost_python_builddir(self):
        return os.path.join(
            self.omim_builddir,
            'boost-build-python{}'.format(get_python_version())
        )

    def clean(self):
        with chdir(boost_root):
            spawn([
                './b2',
                '--user-config={}'.format(self.get_boost_config_path()),
                '--with-python',
                'python={}'.format(get_python_version()),
                '--build-dir={}'.format(self.get_boost_python_builddir()),
                '--clean'
            ])

    def build(self):
        if os.path.exists(self.get_boost_python_builddir()):
            self.clean()
        with chdir(boost_root):
            spawn([
                './b2',
                '--user-config={}'.format(self.get_boost_config_path()),
                '--with-python',
                'python={}'.format(get_python_version()),
                '--build-dir={}'.format(self.get_boost_python_builddir()),
                'cxxflags="-fPIC"'
            ])

    def run(self):
        lib_path = os.path.join(
            boost_librarydir,
            'lib{}.a'.format(self.get_boost_python_libname())
        )
        if os.path.exists(lib_path) and not self.force:
            log.info(
                'Boost_python library `{}` for current '
                'python version already present, skipping build'
                .format(lib_path)
            )
            return

        self.configure_omim()
        self.create_boost_config()
        self.build()


class build_omim_binding(setuptools_build_ext, object):
    user_options = setuptools_build_ext.user_options + [
        ('omim-builddir=', None, 'Path to omim build directory'),
    ]

    def initialize_options(self):
        super(build_omim_binding, self).initialize_options()
        self.omim_builddir = None

    def finalize_options(self):
        super(build_omim_binding, self).finalize_options()
        self.set_undefined_options('build',
                                   ('omim_builddir', 'omim_builddir'),
                                   )

    def cmake_pybindings(self):
        # On some linux systems the cmake we need is called cmake3
        # So we must prefer it
        for cmake in ['cmake3', 'cmake']:
            if find_executable(cmake):
                break
        mkpath(self.omim_builddir)
        with chdir(self.omim_builddir):
            spawn([
                cmake,
                '-DSKIP_DESKTOP=1',
                '-DPYBINDINGS=ON',
                '-DPYTHON_VERSION={}'.format(get_python_version()),
                '-DPYTHON_EXECUTABLE={}'.format(sys.executable),
                '-DPYTHON_INCLUDE_DIR={}'.format(get_python_inc()),
                '-DPYTHON_LIBRARY={}'.format(python_ld_library()),
                omim_root
            ])

    def build_extension(self, ext):
        with chdir(self.omim_builddir):
            spawn([
                'make',
                '-j', str(multiprocessing.cpu_count()//2),
                ext.name
            ])

        mkpath(self.build_lib)
        copy_file(
            os.path.join(self.omim_builddir, '{}.so'.format(ext.name)),
            self.get_ext_fullpath(ext.name)
        )

    def run(self):
        self.run_command('build_boost_python')
        self.cmake_pybindings()
        super(build_omim_binding, self).run()


"""{Path_to_file: variable_in_file}"""
VERSIONS_LOCATIONS = {
    ('xcode', 'common.xcconfig',): 'CURRENT_PROJECT_VERSION',
    ('android', 'gradle.properties',): 'propVersionName',
}

PYBINDINGS = {
    'pygen': {
        'description': 'Binding for working with generation data',
        'package_data': (
            'data/classificator.txt',
            'data/types.txt',
        )
    },
    'pykmlib': {
        'description': 'Binding for working with maps.me KML files',
        'package_data': (
            'data/classificator.txt',
            'data/types.txt',
        )
    },
    'pylocal_ads': {
        'description': 'Binding for working with maps.me local ads data',
    },
    'pymwm_diff': {
        'description': 'Binding for working with mwm diffs',
    },
    'pysearch': {
        'description': 'Binding to access maps.me search engine',
        'package_data': (
            'data/categories_brands.txt',
            'data/categories_cuisines.txt',
            'data/categories.txt',
            'data/classificator.txt',
            'data/types.txt',
        )
    },
    'pytracking': {
        'description': 'Binding for working with user tracks',
    },
    'pytraffic': {
        'description': 'Binding for generationg traffic '
                       'data for maps.me application',
        'package_data': (
            'data/classificator.txt',
            'data/types.txt',
        )
    },
}


def get_version():
    versions = []
    for path, varname in VERSIONS_LOCATIONS.items():
        with open(os.path.join(omim_root, *path), 'r') as f:
            for line in f:
                match = re.search(
                    r'^\s*{}\s*=\s*(?P<version>.*)'.format(varname),
                    line.strip()
                )
                if match:
                    versions.append(LooseVersion(match.group('version')))
                    break
    return sorted(versions)[-1]


def setup_omim_pybinding(
    name=None,
    version=None,
    author='My.com B.V. (Mail.Ru Group)',
    author_email='dev@maps.me',
    url='https://github.com/mapsme/omim',
    license='Apache-2.0',
    supported_pythons=['2', '2.7', '3', '3.5', '3.6', '3.7'],
):
    if name is None:
        log("Name not specified, can't setup module")
        sys.exit()

    if version is None:
        version = get_version()

    package_data = PYBINDINGS[name].get('package_data', [])

    setuptools_setup(
        name='omim-{}'.format(name),
        version=str(version),
        description=PYBINDINGS[name]['description'],
        author=author,
        author_email=author_email,
        url=url,
        license=license,
        packages=[''],
        package_data={
            '': [
                os.path.join(omim_root, path)
                for path in package_data
            ]
        },
        include_package_data=bool(package_data),
        ext_modules=[Extension(name, [])],
        cmdclass={
            'bdist': bdist,
            'build': build,
            'build_boost_python': build_boost_python,
            'build_ext': build_omim_binding,
            'build_py': build_py,
            'egg_info': egg_info,
            'install': install,
            'install_lib': install_lib,
        },
        classifiers=[
            # Trove classifiers
            # Full list:
            # https://pypi.python.org/pypi?%3Aaction=list_classifiers
            'License :: OSI Approved :: Apache Software License',
            'Programming Language :: Python',
            'Programming Language :: Python :: Implementation :: CPython',
        ] + [
            'Programming Language :: Python :: {}'.format(supported_python)
            for supported_python in supported_pythons
        ],
    )


if __name__ == '__main__':
    for binding in PYBINDINGS.keys():
        setup_omim_pybinding(name=binding)

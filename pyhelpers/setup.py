#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import inspect
import linecache
import multiprocessing
import os
import re
import sys
from contextlib import contextmanager
from distutils import log
from distutils.command.bdist import bdist
from distutils.command.build import build
from distutils.core import Command
from distutils.dir_util import mkpath
from distutils.file_util import copy_file
from distutils.spawn import spawn, find_executable
from distutils.sysconfig import (
    get_config_var,
    get_python_inc,
    get_python_version,
)
from distutils.version import LooseVersion
from inspect import (
    getsourcefile,
    getfile,
    getmodule,
    ismodule,
    isclass,
    ismethod,
    isfunction,
    istraceback,
    isframe,
    iscode,
)

from setuptools import dist, setup
from setuptools.command.build_ext import build_ext
from setuptools.command.egg_info import egg_info, manifest_maker
from setuptools.command.install import install
from setuptools.extension import Extension

try:
    # for pip >= 10
    from pip._internal.req import parse_requirements
except ImportError:
    # for pip <= 9.0.3
    from pip.req import parse_requirements

# Monkey-patching to disable checking package names
dist.check_packages = lambda dist, attr, value: None


# Patch from https://github.com/ipython/ipython/issues/1456/
def findsource(object):
    """Return the entire source file and starting line number for an object.
    The argument may be a module, class, method, function, traceback, frame,
    or code object.  The source code is returned as a list of all the lines
    in the file and the line number indexes a line in that list.  An IOError
    is raised if the source code cannot be retrieved.
    FIXED version with which we monkeypatch the stdlib to work around a bug."""

    file = getsourcefile(object) or getfile(object)
    # If the object is a frame, then trying to get the globals dict from its
    # module won't work. Instead, the frame object itself has the globals
    # dictionary.
    globals_dict = None
    if inspect.isframe(object):
        # XXX: can this ever be false?
        globals_dict = object.f_globals
    else:
        module = getmodule(object, file)
        if module:
            globals_dict = module.__dict__
    lines = linecache.getlines(file, globals_dict)
    if not lines:
        raise IOError('could not get source code')

    if ismodule(object):
        return lines, 0

    if isclass(object):
        name = object.__name__
        pat = re.compile(r'^(\s*)class\s*' + name + r'\b')
        # make some effort to find the best matching class definition:
        # use the one with the least indentation, which is the one
        # that's most probably not inside a function definition.
        candidates = []
        for i in range(len(lines)):
            match = pat.match(lines[i])
            if match:
                # if it's at toplevel, it's already the best one
                if lines[i][0] == 'c':
                    return lines, i
                # else add whitespace to candidate list
                candidates.append((match.group(1), i))
        if candidates:
            # this will sort by whitespace, and by line number,
            # less whitespace first
            candidates.sort()
            return lines, candidates[0][1]
        else:
            raise IOError('could not find class definition')

    if ismethod(object):
        object = object.im_func
    if isfunction(object):
        object = object.func_code
    if istraceback(object):
        object = object.tb_frame
    if isframe(object):
        object = object.f_code
    if iscode(object):
        if not hasattr(object, 'co_firstlineno'):
            raise IOError('could not find function definition')
        pat = re.compile(r'^(\s*def\s)|(.*(?<!\w)lambda(:|\s))|^(\s*@)')
        pmatch = pat.match
        # fperez - fix: sometimes, co_firstlineno can give a number larger than
        # the length of lines, which causes an error.  Safeguard against that.
        lnum = min(object.co_firstlineno, len(lines)) - 1
        while lnum > 0:
            if pmatch(lines[lnum]):
                break
            lnum -= 1

        return lines, lnum
    raise IOError('could not find code object')


# Monkeypatch inspect to apply our bugfix.
# This code only works with Python >= 2.5
inspect.findsource = findsource


PYHELPERS_DIR = os.path.abspath(os.path.dirname(__file__))
OMIM_ROOT = os.path.dirname(PYHELPERS_DIR)
BOOST_ROOT = os.path.join(OMIM_ROOT, '3party', 'boost')
BOOST_LIBRARYDIR = os.path.join(BOOST_ROOT, 'stage', 'lib')
ORIGINAL_CWD = os.getcwd()


@contextmanager
def chdir(target_dir):
    saved_cwd = os.getcwd()
    os.chdir(target_dir)
    try:
        yield
    finally:
        os.chdir(saved_cwd)


class BuildCommand(build, object):
    user_options = build_ext.user_options + [
        ('omim-builddir=', None, 'Path to omim build directory'),
    ]

    def initialize_options(self):
        super(BuildCommand, self).initialize_options()
        self.omim_builddir = os.path.join(OMIM_ROOT, 'build')

    def finalize_options(self):
        if os.path.isabs(self.omim_builddir):
            self.omim_builddir = os.path.abspath(self.omim_builddir)
        else:
            self.omim_builddir = os.path.abspath(
                os.path.join(ORIGINAL_CWD, self.omim_builddir)
            )
        self.build_base = os.path.relpath(
            os.path.join(
                self.omim_builddir,
                '{}-builddir'.format(self.distribution.get_name()),
            )
        )
        super(BuildCommand, self).finalize_options()


class BdistCommand(bdist, object):
    user_options = build_ext.user_options + [
        ('omim-builddir=', None, 'Path to omim build directory'),
    ]

    def initialize_options(self):
        super(BdistCommand, self).initialize_options()
        self.omim_builddir = None

    def finalize_options(self):
        self.set_undefined_options(
            'build', ('omim_builddir', 'omim_builddir'),
        )
        self.dist_dir = os.path.join(
            self.omim_builddir, '{}-dist'.format(self.distribution.get_name())
        )
        super(BdistCommand, self).finalize_options()


class ManifestMaker(manifest_maker, object):
    def add_defaults(self):
        super(ManifestMaker, self).add_defaults()
        # Our README.md is for entire omim project, no point including it
        # into python package, so remove it.
        self.filelist.exclude('README.*')


class EggInfoCommand(egg_info, object):
    user_options = build_ext.user_options + [
        ('omim-builddir=', None, 'Path to omim build directory'),
    ]

    def initialize_options(self):
        super(EggInfoCommand, self).initialize_options()
        self.omim_builddir = None

    def finalize_options(self):
        self.set_undefined_options(
            'build', ('omim_builddir', 'omim_builddir'),
        )
        self.egg_base = os.path.relpath(
            os.path.join(
                self.omim_builddir,
                '{}-egg-info'.format(self.distribution.get_name()),
            )
        )
        mkpath(self.egg_base)
        super(EggInfoCommand, self).finalize_options()

    def find_sources(self):
        """
        Copied from setuptools.command.egg_info method to override
        internal manifest_maker with our subclass

        Generate SOURCES.txt manifest file
        """
        manifest_filename = os.path.join(self.egg_info, 'SOURCES.txt')
        mm = ManifestMaker(self.distribution)
        mm.manifest = manifest_filename
        mm.run()
        self.filelist = mm.filelist


class InstallCommand(install, object):
    user_options = install.user_options + [
        ('omim-builddir=', None, 'Path to omim build directory'),
    ]

    def initialize_options(self):
        super(InstallCommand, self).initialize_options()
        self.omim_builddir = None

    def finalize_options(self):
        super(InstallCommand, self).finalize_options()
        self.set_undefined_options(
            'build', ('omim_builddir', 'omim_builddir'),
        )


class BuildBoostPythonCommand(Command, object):
    user_options = [
        (
            'force',
            'f',
            'forcibly build boost_python library (ignore existence)',
        ),
        ('omim-builddir=', None, 'Path to omim build directory'),
    ]
    boolean_options = ['force']

    def initialize_options(self):
        self.force = None
        self.omim_builddir = None

    def finalize_options(self):
        self.set_undefined_options(
            'build', ('force', 'force'), ('omim_builddir', 'omim_builddir'),
        )

    def get_boost_python_libname(self):
        return 'boost_python{}{}'.format(
            sys.version_info.major, sys.version_info.minor
        )

    def get_boost_config_path(self):
        return os.path.join(
            self.omim_builddir,
            'python{}-config.jam'.format(get_python_version()),
        )

    def configure_omim(self):
        with chdir(OMIM_ROOT):
            spawn(['./configure.sh'])

    def create_boost_config(self):
        mkpath(self.omim_builddir)
        with open(self.get_boost_config_path(), 'w') as f:
            f.write(
                'using python : {} : {} : {} ;\n'.format(
                    get_python_version(),
                    sys.executable,
                    get_python_inc(),
                )
            )

    def get_boost_python_builddir(self):
        return os.path.join(
            self.omim_builddir,
            'boost-build-python{}'.format(get_python_version()),
        )

    def clean(self):
        with chdir(BOOST_ROOT):
            spawn(
                [
                    './b2',
                    '--user-config={}'.format(self.get_boost_config_path()),
                    '--with-python',
                    'python={}'.format(get_python_version()),
                    '--build-dir={}'.format(self.get_boost_python_builddir()),
                    '--clean',
                ]
            )

    def build(self):
        if os.path.exists(self.get_boost_python_builddir()):
            self.clean()

        with chdir(BOOST_ROOT):
            spawn(
                [
                    './b2',
                    '--user-config={}'.format(self.get_boost_config_path()),
                    '--with-python',
                    'python={}'.format(get_python_version()),
                    '--build-dir={}'.format(self.get_boost_python_builddir()),
                    'cxxflags="-fPIC"',
                ]
            )

    def run(self):
        lib_path = os.path.join(
            BOOST_LIBRARYDIR, 'lib{}.a'.format(self.get_boost_python_libname())
        )
        if os.path.exists(lib_path) and not self.force:
            log.info(
                'Boost_python library `{}` for current '
                'python version already present, skipping build'.format(
                    lib_path
                )
            )
            return

        self.configure_omim()
        self.create_boost_config()
        self.build()


class BuildOmimBindingCommand(build_ext, object):
    user_options = build_ext.user_options + [
        ('omim-builddir=', None, 'Path to omim build directory'),
    ]

    def initialize_options(self):
        super(BuildOmimBindingCommand, self).initialize_options()
        self.omim_builddir = None

    def finalize_options(self):
        super(BuildOmimBindingCommand, self).finalize_options()
        self.set_undefined_options(
            'build', ('omim_builddir', 'omim_builddir'),
        )

    def cmake_pybindings(self):
        # On some linux systems the cmake we need is called cmake3
        # So we must prefer it
        for cmake in ['cmake3', 'cmake']:
            if find_executable(cmake):
                break

        mkpath(self.omim_builddir)
        with chdir(self.omim_builddir):
            spawn(
                [
                    cmake,
                    '-DSKIP_QT_GUI=1',
                    '-DPYBINDINGS=ON',
                    '-DPYBINDINGS_VERSION={}'.format(get_version()),
                    '-DPYTHON_EXECUTABLE={}'.format(sys.executable),
                    '-DPYTHON_INCLUDE_DIR={}'.format(get_python_inc()),
                    OMIM_ROOT,
                ]
            )

    def build_extension(self, ext):
        with chdir(self.omim_builddir):
            spawn(
                [
                    'make',
                    '-j',
                    str(max(1, multiprocessing.cpu_count() // 2)),
                    ext.name,
                ]
            )

        mkpath(self.build_lib)
        copy_file(
            os.path.join(self.omim_builddir, '{}.so'.format(ext.name)),
            self.get_ext_fullpath(ext.name),
        )

    def run(self):
        self.run_command('build_boost_python')
        self.cmake_pybindings()
        super(BuildOmimBindingCommand, self).run()


VERSIONS_LOCATIONS = {
    'xcode/common.xcconfig': 'CURRENT_PROJECT_VERSION',
    'android/gradle.properties': 'propVersionName',
}

PYBINDINGS = {
    'pygen': {
        'path': 'generator/pygen',
        'py_modules': ['example',],
        'install_requires': ['omim-data-essential'],
        'description': 'Binding for working with generation data',
    },
    'pykmlib': {
        'path': 'kml/pykmlib',
        'install_requires': ['omim-data-essential'],
        'description': 'Binding for working with maps.me KML files',
    },
    'pymwm_diff': {
        'path': 'generator/mwm_diff/pymwm_diff',
        'description': 'Binding for working with mwm diffs',
    },
    'pysearch': {
        'path': 'search/pysearch',
        'description': 'Binding to access maps.me search engine',
        'install_requires': ['omim-data-essential'],
    },
    'pytracking': {
        'path': 'tracking/pytracking',
        'description': 'Binding for working with user tracks',
    },
    'pytraffic': {
        'path': 'traffic/pytraffic',
        'description': (
            'Binding for generation traffic data for maps.me application'
        ),
        'install_requires': ['omim-data-essential'],
    },
}


def get_version():
    versions = []
    for path, varname in VERSIONS_LOCATIONS.items():
        with open(os.path.join(OMIM_ROOT, os.path.normpath(path))) as f:
            for line in f:
                match = re.search(
                    r'^\s*{}\s*=\s*(?P<version>.*)'.format(varname),
                    line.strip(),
                )
                if match:
                    versions.append(LooseVersion(match.group('version')))
                    break
    code_version = max(versions)

    env_version_addendum = os.environ.get('OMIM_SCM_VERSION', '')

    return "{}{}".format(code_version, env_version_addendum)


def transform_omim_requirement(requirement, omim_package_version):
    if requirement.startswith("omim"):
        index = len(requirement) - 1
        for i, ch in enumerate(requirement):
            if not (ch.isalnum() or ch in "-_"):
                index = i
                break

        requirement_without_version = requirement[0: index + 1]
        requirement = "{}=={}".format(
            requirement_without_version, omim_package_version
        )
    return requirement


def get_requirements(path="", omim_package_version=get_version()):
    requirements = []
    fpath = os.path.join(path, "requirements.txt")
    for d in parse_requirements(fpath, session="s"):
        try:
            req_with_version = d.requirement
        except AttributeError:
            req_with_version = str(d.req)

        requirements.append(
            transform_omim_requirement(req_with_version, omim_package_version)
        )
    return requirements


def setup_omim_pybinding(
    name,
    version=None,
    author='CoMaps',
    author_email='info@comaps.app',
    url='https://codeberg.org/comaps/comaps',
    license='Apache-2.0',
    supported_pythons=('2', '2.7', '3', '3.5', '3.6', '3.7', '3.8', '3.9'),
):
    if version is None:
        version = str(get_version())

    install_requires = [
        transform_omim_requirement(req, version)
        for req in PYBINDINGS[name].get('install_requires', [])
    ]

    setup(
        name='omim-{}'.format(name),
        version=version,
        description=PYBINDINGS[name]['description'],
        author=author,
        author_email=author_email,
        url=url,
        license=license,
        packages=PYBINDINGS[name].get('packages', []),
        package_dir=PYBINDINGS[name].get('package_dir', {}),
        py_modules=PYBINDINGS[name].get('py_modules', []),
        package_data=PYBINDINGS[name].get('package_data', {}),
        install_requires=install_requires,
        ext_modules=[Extension(name, [])],
        cmdclass={
            'bdist': BdistCommand,
            'build': BuildCommand,
            'build_boost_python': BuildBoostPythonCommand,
            'build_ext': BuildOmimBindingCommand,
            'egg_info': EggInfoCommand,
            'install': InstallCommand,
        },
        classifiers=[
            # Trove classifiers
            # Full list:
            # https://pypi.python.org/pypi?%3Aaction=list_classifiers
            'License :: OSI Approved :: Apache Software License',
            'Programming Language :: Python',
            'Programming Language :: Python :: Implementation :: CPython',
        ]
        + [
            'Programming Language :: Python :: {}'.format(supported_python)
            for supported_python in supported_pythons
        ],
    )


if __name__ == '__main__':
    log.set_threshold(log.INFO)

    for binding in PYBINDINGS.keys():
        log.info('Run {}:'.format(binding))
        path = os.path.join(
            OMIM_ROOT, os.path.normpath(PYBINDINGS[binding]['path'])
        )

        with chdir(path):
            setup_omim_pybinding(binding)

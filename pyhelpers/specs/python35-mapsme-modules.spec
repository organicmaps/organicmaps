# TODO(mgsergio, yershov): Add other modules.

%global __arch_install_post     QA_SKIP_RPATHS=1 /usr/lib/rpm/check-rpaths   /usr/lib/rpm/check-buildroot
%define py_version 3.5.1
%define py_release 1.portal
%define py_prefix /usr/local/python35

%global __python %{py_prefix}/bin/python3.5
%define python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; import sys; sys.stdout.write(get_python_lib())")
%define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; import sys; sys.stdout.write(get_python_lib(1))")
%define pybindir %(%{__python} -c "import sys; print '%s/bin' % sys.exec_prefix")
#%{expand: %%define pyver %(%{__python} -c 'import sys;print(sys.version[0:5])')}

%define __os_install_post \
    /usr/lib/rpm/redhat/brp-compress \
    %{!?__debug_package:/usr/lib/rpm/redhat/brp-strip %{__strip}} \
    /usr/lib/rpm/redhat/brp-strip-static-archive %{__strip} \
    /usr/lib/rpm/redhat/brp-strip-comment-note %{__strip} %{__objdump} \
    /usr/lib/rpm/brp-python-bytecompile %{__python} \
    /usr/lib/rpm/redhat/brp-python-hardlink \
    %{!?__jar_repack:/usr/lib/rpm/redhat/brp-java-repack-jars} \
%{nil}

%define unmangled_version 0.1.7
%define version %{unmangled_version}
%define release 1
%define tag py-modules-%{unmangled_version}

Name:           python35-mapsme-modules
Version:        %{version}
Release:        %{release}.portal%{dist}
Summary:	Python maps.me modules
License:	Apache Public License 2.0
Vendor:         Mail.Ru Group

Group:          Development/Languages/Python
URL:		https://github.com/mapsme/omim
Source:		omim-py-modules-%{unmangled_version}.tar.gz

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Prefix:         %{_prefix}

BuildRequires:  python35 >= %{py_version}
# BuildRequires:  python35-setuptools
BuildRequires:  cmake3
BuildRequires:  boost_prefix-devel >= 1.54.0-3
Requires:       python35 >= %{py_version}
Requires:	python35-pylocal_ads = %{version}-%{release}

%description
Python maps.me modules metapackage. Installs all included modules

%package -n python35-pylocal_ads
Summary:  pylocal_ads python module from maps.me localads
Group:          Development/Languages/Python

%description -n python35-pylocal_ads
Separate pylocal_ads module for python35 from maps.me localads

%prep
%{__rm} -rf %{_builddir}/%{name}-%{version}
if [ -e %{S:0} ]; then
        %{__tar} xzf %{S:0}
        %{__chmod} -Rf a+rX,u+w,g-w,o-w %{_builddir}/%{name}-%{version}
else
        git clone --depth=1 https://github.com/mapsme/omim.git %{_builddir}/%{name}-%{version}/omim
        pushd %{_builddir}/%{name}-%{version}/omim
	git fetch origin tag %{tag} --depth=1
        git checkout %{tag}
	git submodule update --init --checkout
	# pack source to save it in src rpm
        popd
        %{__tar} czf %{S:0} %{name}-%{version}
fi
%setup -D -T

%build
. /opt/rh/devtoolset-3/enable
cd omim
echo git@github.com:mapsme/omim-private.git | ./configure.sh
cd ..
%{__mkdir_p} build && cd build
# TODO(mgergio, yershov): Why should we stills specify PYTHON_LIBRARY and
# PYTHON_INCLUDE_DIR manually?
%{__cmake3} -DPYTHON_LIBRARY=/usr/local/python35/lib/libpython3.so -DPYTHON_INCLUDE_DIR=/usr/local/python35/include/python3.5m/ -DPYTHON_VERSION=3.5 -DBOOST_INCLUDEDIR=/usr/local/boost_1.54.0/include/ -DPYBINDINGS=ON ../omim
%{__make} %{?_smp_mflags} pylocal_ads

%install
%{__install} -m 755 -D %{_builddir}/%{name}-%{version}/build/pylocal_ads.so %{buildroot}/%{python_sitelib}/pylocal_ads.so

%clean
rm -rf %{buildroot}

%files -n python35-pylocal_ads
%defattr(-,root,root)
%{python_sitelib}/pylocal_ads.so

%changelog
* Wed Apr 26 2017 Magidovich Sergey <s.magidovich@corp.mail.ru> - 0.1b-1
- Initiated build

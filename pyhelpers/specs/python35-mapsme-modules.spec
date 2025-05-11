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

%define project %(echo $PROJECT)
%define version %(echo $VERSION)
%define release %(echo $RELEASE)
%define tag py-modules-%{version}

Name:           python35-mapsme-modules
Version:        %{version}
Release:        %{release}.portal%{dist}
Summary:	Python maps.me modules
License:	Apache Public License 2.0
Vendor:         Mail.Ru Group

Group:          Development/Languages/Python
URL:		https://codeberg.org/comaps/comaps
Source:		omim-py-modules-%{version}.tar.gz

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Prefix:         %{_prefix}

BuildRequires:  python35 >= %{py_version}
# BuildRequires:  python35-setuptools
BuildRequires:  cmake3
BuildRequires:  boost_prefix-devel >= 1.54.0-3
BuildRequires:  redhat-rpm-config
Requires:       python35 >= %{py_version}
Requires:	python35-%{project} = %{version}-%{release}

%description
Python maps.me modules metapackage. Installs all included modules

%package -n python35-%{project}
Summary:  %{project} python module from maps.me localads
Group:          Development/Languages/Python

%description -n python35-%{project}
Separate %{project} module for python35 from maps.me localads

%prep

%{__rm} -rf %{_builddir}/%{name}-%{version}
if [ -e %{S:0} ]; then
        %{__tar} xzf %{S:0}
        %{__chmod} -Rf a+rX,u+w,g-w,o-w %{_builddir}/%{name}-%{version}
else
        git clone --depth=1 https://codeberg.org/comaps/comaps.git %{_builddir}/%{name}-%{version}/omim
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
%{__cmake3} -DPYTHON_LIBRARY=/usr/local/python35/lib/libpython3.so -DPYTHON_INCLUDE_DIR=/usr/local/python35/include/python3.5m/ -DBOOST_INCLUDEDIR=/usr/local/boost_1.54.0/include/ -DPYBINDINGS=ON -DSKIP_QT_GUI=ON ../omim
%{__make} %{?_smp_mflags} %{project}

%install
%{__install} -m 755 -D %{_builddir}/%{name}-%{version}/build/%{project}.so %{buildroot}/%{python_sitelib}/%{project}.so

%clean
rm -rf %{buildroot}

%files -n python35-%{project}
%defattr(-,root,root)
%{python_sitelib}/%{project}.so

%changelog
* Thu Aug 03 2017 Sergey Yershov <yershov@corp.mail.ru> 0.2a-1
- Adopted to build any of available bingings

* Wed Apr 26 2017 Magidovich Sergey <s.magidovich@corp.mail.ru> - 0.1b-1
- Initiated build

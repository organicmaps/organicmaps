%define version_string 20150602.1200
%define version_date %(date -d "`echo %{version_string} | sed -e 's#\\.# #'`" +"%Y-%m-%d %H:%M")
%define clone_url git@gitmm.corp.mail.ru:maps/omim.git

Name:		osrm-routed
Version:	%{version_string}
Release:	1%{?dist}
Summary:	OSRM - OpenStreetmap Routing Machine

Group:		System Environment/Daemons
License:	Proprietary
URL:		https://gitmm.corp.mail.ru/maps/omim
BuildRoot:	%{_tmppath}/%{name}-%{version}-%(id -u -n)
Source0:	omim-%{version}.tar.gz
Source1:	%{name}.init

BuildRequires: git cmake28 protobuf-devel expat-devel devtoolset-2-gcc-c++ devtoolset-2-gcc stxxl-devel osmpbf luabind-devel luajit-devel qt5-qtbase-devel tbb-devel expat-devel lua-devel devtoolset-2-binutils protobuf-devel protobuf-lite-devel bzip2-devel 
BuildRequires: boost_prefix-devel >= 1.54.0
BuildRequires: boost_prefix-devel < 1.55.0

Requires:	omim-data-polygons = %{version}-%{release}

%description
OSM Routing Machine with Maps.me additions
https://gitmm.corp.mail.ru/maps/omim.git

%package -n omim-data-polygons
Summary:	Data polygons - Maps.me data from OMIM (One Month In Minsk) repository
Group:		System Environment/Daemons
BuildArch:	noarch

%description -n omim-data-polygons
Maps.me polygons packed from omim repository

%prep
%{__rm} -rf %{_builddir}/%{name}-%{version}
%setup -c -T -D
if [ -e %{S:0} ]; then
        %{__tar} xzf %{S:0}
        %{__chmod} -Rf a+rX,u+w,g-w,o-w %{_builddir}/%{name}-%{version}/omim-%{version}
else
	%{__mkdir_p} %{_builddir}/%{name}-%{version}
        git clone %{clone_url} %{_builddir}/%{name}-%{version}/omim-%{version}
        cd %{_builddir}/%{name}-%{version}/omim-%{version}
        git checkout  `git rev-list -n 1 --before="%{version_date}" master`
        # pack source to save it in src rpm
        cd ..
        %{__tar} czf %{S:0} omim-%{version}
fi

%build
# aux build script
cat <<EOF > build_conf_co6_mailru.sh
#!/bin/bash
set -x -u -e

DIRNAME=\$1

if [ ! -d \$DIRNAME ]; then
  mkdir \$DIRNAME
  ln -s ../omim-%{version}/data \$DIRNAME/data
fi

CONF=\`sed -e 's#.*-\(\)#\1#' <<< \$1\`
INC="INCLUDEPATH *= /usr/include/ /opt/rh/devtoolset-2/root/usr/include/c++/4.8.2/ /opt/rh/devtoolset-2/root/usr/include/c++/4.8.2/x86_64-redhat-linux/"
LIB="LIBPATH *= /opt/rh/devtoolset-2/root/usr/lib/gcc/x86_64-redhat-linux/4.8.2/ /opt/rh/devtoolset-2/root/usr/lib"

export BOOST_INCLUDEDIR=/usr/local/boost_1.54.0/include

pushd \$DIRNAME
qmake-qt5 -r ../omim-%{version}/omim.pro CONFIG+=\$CONF "\$INC" "\$LIB" -spec linux-clang
make %{?_smp_mflags}
popd
EOF
chmod +x build_conf_co6_mailru.sh

# logging
cat << EOF > rsyslog-osrm-routed.conf
if $programname == 'osrm-routed' then /logs/mapsme/osrm-routed.log
& ~
EOF

./build_conf_co6_mailru.sh omim-build-release 
./build_conf_co6_mailru.sh omim-build-debug

%{__mkdir} omim-%{version}/3party/osrm/osrm-backend/build
cd omim-%{version}/3party/osrm/osrm-backend/build
. /opt/rh/devtoolset-2/enable
cmake28 . -DBOOST_ROOT="/usr/local/boost_1.54.0/" ..
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%{__mkdir_p} %{buildroot}%{_bindir} %{buildroot}%{_initrddir} %{buildroot}%{_localstatedir}/run/%{name} %{buildroot}%{_sysconfdir}/rsyslog.d %{buildroot}/logs/mapsme
cd omim-%{version}/3party/osrm/osrm-backend/build
%{__install} -m 755 %{name} %{buildroot}%{_bindir}/%{name}
%{__install} -m 555 %{S:1} %{buildroot}%{_initrddir}/%{name}
%{__install} -m 644 %{_builddir}/%{name}-%{version}/rsyslog-osrm-routed.conf %{buildroot}%{_sysconfdir}/rsyslog.d/osrm-routed.conf

#omim-data-polygons
cd %{_builddir}/%{name}-%{version}
%{__mkdir_p} %{buildroot}%{_datadir}/omim/data
%{__install} -m 644 omim-%{version}/data/packed_polygons.bin %{buildroot}%{_datadir}/omim/data/

%clean
rm -rf %{buildroot}

%pre
getent group osrm >/dev/null || groupadd -r osrm
getent passwd osrm >/dev/null || useradd -r -g osrm -d /var/lib/osrm/ -s /sbin/nologin -c "OSRM user" osrm

%post
/sbin/chkconfig --add %{name}

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_initrddir}/%{name}
%config(noreplace) %{_sysconfdir}/rsyslog.d/osrm-routed.conf
%dir %attr(-,osrm,osrm) /var/run/%{name}
%dir /logs/mapsme

%files -n omim-data-polygons
%defattr(-,root,root,-)
%{_datadir}/omim/data/*

%changelog
* Thu May 28 2015 Victor Popov <v.popov@corp.mail.ru> - 20150602.1200
- inital packaging

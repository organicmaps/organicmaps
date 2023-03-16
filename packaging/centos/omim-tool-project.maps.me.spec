%define project %(echo $PROJECT)
%define tool_name %(echo $SUBPROJECT)
%define version %(echo $VERSION)
%define project_repo %(echo $REPO_URL)/%{project}.git
%define release %(echo $RELEASE)

%define project_tool_name %{project}-%{tool_name}
%define project_root %{_builddir}/%{project_tool_name}-%{version}
%define source_dir_name %{project}-%{version}
%define project_src %{project_root}/%{source_dir_name}

%define _empty_manifest_terminate_build 0

Name:      %{project_tool_name}
Summary:   %{project_tool_name} - a utility from %{project} (maps.me project)
Version:   %{version}
Release:   %{release}
License:   Apache 2.0
Url:       http://github.com/mapsme/%{project}
Buildroot: %{_tmppath}/%{project_tool_name}-%{version}-%(%{__id_u} -n)
Source:    %{source_dir_name}.tar.gz
BuildRequires: cmake3
%if %{rhel} == 7
BuildRequires: devtoolset-7-gcc-c++
%endif
%if %{rhel} == 8
BuildRequires: gcc-c++ 
%endif
BuildRequires: git
BuildRequires: qt5-qtbase-devel
BuildRequires: sqlite-devel
BuildRequires: zlib-devel

%description
%{project_tool_name} is a maps.me tool.

%prep
rm -rf %{project_root} 2> /dev/null
mkdir -p %{project_src}
git clone -b %{version} --depth 1 --recurse-submodules %{project_repo} %{project_src}
rm -rf %{project_src}/.git
cd %{project_src}/..
%{__tar} czf %{S:0} %{source_dir_name}
%setup -T -D

%build
%if %{rhel} == 7
source /opt/rh/devtoolset-7/enable
%endif
echo | %{project_src}/configure.sh
mkdir -p %{project_root}/build
cd %{project_root}/build
cmake3 %{project_src} -DSKIP_DESKTOP=ON
make %{?_smp_mflags} %{tool_name}

%install
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}
mkdir -p %{buildroot}/%{_bindir}
mkdir -p %{buildroot}/%{_datadir}/%{project_tool_name}
cp -Rp %{project_root}/build/%{tool_name} %{buildroot}/%{_bindir}/%{project_tool_name}
cp -Rp %{project_src}/data %{buildroot}/%{_datadir}/%{project_tool_name}

%files
%defattr(-,root,root,-)
%{_bindir}/*
%{_datadir}/*

%changelog
* Thu Jul 22 2020 Maksim Andrianov <m.andrianov@corp.mail.ru>
- Initial build

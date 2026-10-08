# The Jolla Harbour package, harbour-organicmaps without voice instructions, is built with "--with harbour",
# e.g. sfdk build -- --with harbour.
%bcond_with harbour

Name:       %{?with_harbour:harbour-}organicmaps
Summary:    Offline maps and navigation based on OpenStreetMap
# The date based version of the other platforms, see tools/unix/version.sh.
Version:    %(bash tools/unix/version.sh ios_version)
Release:    %(bash tools/unix/version.sh count)
# The map data files are under DATA_LICENSE.txt.
License:    Apache-2.0
URL:        https://organicmaps.app
Source0:    %{name}-%{version}.tar.bz2

BuildRequires:  cmake
# For tools/unix/version.sh, which falls back to release 0 without git.
BuildRequires:  git
BuildRequires:  ninja
BuildRequires:  python3-base
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Multimedia)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5Positioning)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Sensors)
BuildRequires:  pkgconfig(sailfishapp)
BuildRequires:  pkgconfig(glesv2)
BuildRequires:  pkgconfig(mlite5)
# QML modules, the linked Qt libraries are found automatically. Harbour allows no Requires for the other
# imported modules, which are on every device.
Requires:       sailfishsilica-qt5
Requires:       libkeepalive
%if %{without harbour}
# They share the data and the D-Bus name.
Conflicts:      harbour-organicmaps
%endif

# Harbour allows neither providing nor requiring the private library.
%define __provides_exclude_from ^%{_datadir}/%{name}/lib/.*$
%define __requires_exclude ^liborganicmaps.*$

%description
Organic Maps is a privacy-focused offline maps and navigation app based on
OpenStreetMap data.

%if "%{?vendor}" == "chum"
PackageName: Organic Maps
Type: desktop-application
DeveloperName: Organic Maps
Categories:
 - Maps
 - Navigation
Custom:
  Repo: https://github.com/organicmaps/organicmaps
Icon: https://raw.githubusercontent.com/organicmaps/organicmaps/master/android/app/ic_launcher-playstore.png
Url:
  Homepage: https://organicmaps.app
  Help: https://organicmaps.app/faq/
  Bugtracker: https://github.com/organicmaps/organicmaps/issues
  Donation: https://organicmaps.app/donate/
%endif

%prep
%setup -q -n %{name}-%{version}

%build
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DSAILFISH=ON \
  -DSAILFISH_HARBOUR=%{?with_harbour:ON}%{!?with_harbour:OFF} \
  -DBUILD_TESTING=OFF
cmake --build build --target organicmaps_sailfish

%install
# Only the app; the bundled 3party libraries carry their own install rules.
DESTDIR=%{buildroot} cmake -P build/sailfish/cmake_install.cmake
# brp-strip skips shared objects, which would otherwise ship with ~300 MB of debug info.
%{__strip} --strip-unneeded %{buildroot}%{_datadir}/%{name}/lib/liborganicmaps.so %{buildroot}%{_bindir}/%{name}
# Harbour allows no files outside the app directories, so not %%license in /usr/share/licenses.
install -m 644 LICENSE DATA_LICENSE.txt %{buildroot}%{_datadir}/%{name}/

%if %{without harbour}
%post
# A minimized app keeps running as its cover, so the next launch would show the old version.
pkill -f '^%{_bindir}/%{name}$' || :
%endif

%files
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%if %{without harbour}
%{_sysconfdir}/sailjail/permissions/%{name}.profile
%endif

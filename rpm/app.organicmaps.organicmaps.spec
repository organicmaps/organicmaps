# Aurora OS package. Build with:
#   sfdk build            (from the repository root)
# or, inside an Aurora SDK shell:
#   mb2 -t <target> -s rpm/app.organicmaps.organicmaps.spec build
#
# Unlike the Sailfish package there is no Sailjail profile: Aurora isolates apps
# through the [X-Application] section of the desktop file and libauroraapp.

Name:       app.organicmaps.organicmaps
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
# The supported application library; libsailfishapp is deprecated on Aurora.
BuildRequires:  pkgconfig(auroraapp)
BuildRequires:  pkgconfig(appdir-cpp)
BuildRequires:  pkgconfig(glesv2)
BuildRequires:  pkgconfig(mlite5)
# QML modules; the linked Qt libraries are found automatically.
Requires:       sailfishsilica-qt5
Requires:       nemo-qml-plugin-notifications-qt5
Requires:       libkeepalive

# The private liborganicmaps.so (Provides + Requires) and the transitive libappmanifest-glib.so.1
# are not in the Aurora allowed-dependency list; strip them from RPM's automatic dependency scan.
%global __provides_exclude_from ^%{_datadir}/%{name}/lib/.*$
%global __requires_exclude ^(liborganicmaps|libappmanifest-glib).*$

%description
Organic Maps is a privacy-focused offline maps and navigation app based on
OpenStreetMap data.

%prep
%autosetup

%build
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DAURORA=ON \
  -DBUILD_TESTING=OFF
cmake --build build --target organicmaps_sailfish

%install
# Only the app; the bundled 3party libraries carry their own install rules.
DESTDIR=%{buildroot} cmake -P build/sailfish/cmake_install.cmake
# Strip the private shared object, which would otherwise ship with ~300 MB of debug info.
%{__strip} --strip-unneeded %{buildroot}%{_datadir}/%{name}/lib/liborganicmaps.so %{buildroot}%{_bindir}/%{name}

%files
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png

%changelog

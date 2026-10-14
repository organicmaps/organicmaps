# Configuration package with the offline voice data for app.organicmaps.organicmaps:
# Piper voice models (ru/en, medium) and the espeak-ng data used by libpiper.
# Built with sfdk/mb2 or, directly, rpmbuild from a source tree laid out as:
#   app.organicmaps.voices.desktop, qml/app.organicmaps.voices.qml,
#   icons/{86x86,108x108,128x128,172x172}/app.organicmaps.voices.png,
#   configuration/{*.onnx,*.onnx.json,espeak-ng-data/}
# The model and espeak-ng data files are downloaded separately and are not in the repository.

Name:       app.organicmaps.voices
Summary:    Piper voice models and espeak-ng data for Organic Maps
Version:    2026.10.08
Release:    1
License:    MIT and CC-BY-4.0 and GPLv3
URL:        https://gitflic.ru/project/ub3gad/maps
BuildArch:  noarch
Source0:    %{name}-%{version}.tar.zst

Requires: libauroraapp-launcher
BuildRequires: qtchooser

%description
Optional offline text-to-speech add-on for Organic Maps on Aurora OS: Piper voice models
(Russian and English, medium quality) and the espeak-ng phonemization data used by libpiper.
Without this package the app has no voice instructions.

%prep
%autosetup

%build

%install
install -d %{buildroot}%{_datadir}/%{name}/qml
install -m644 qml/%{name}.qml %{buildroot}%{_datadir}/%{name}/qml/
install -d %{buildroot}%{_datadir}/applications
install -m644 %{name}.desktop %{buildroot}%{_datadir}/applications/
for size in 86x86 108x108 128x128 172x172; do
  install -d %{buildroot}%{_datadir}/icons/hicolor/$size/apps
  install -m644 icons/$size/%{name}.png %{buildroot}%{_datadir}/icons/hicolor/$size/apps/
done
install -d %{buildroot}%{_datadir}/common/app.organicmaps/voices
cp -a configuration/. %{buildroot}%{_datadir}/common/app.organicmaps/voices/
# The Aurora validator rejects group/world writable files.
find %{buildroot}%{_datadir}/common/app.organicmaps/voices -type d -exec chmod 755 {} +
find %{buildroot}%{_datadir}/common/app.organicmaps/voices -type f -exec chmod 644 {} +

%files
%{_datadir}/%{name}/qml
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/common/app.organicmaps/voices

%changelog

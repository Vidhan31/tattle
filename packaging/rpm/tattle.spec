%global debug_package %{nil}

Name:           tattle
Version:        %{?version}%{!?version:1.1.0}
Release:        %{?release}%{!?release:1}%{?dist}
Summary:        Fast and modern speech-to-text desktop application

License:        GPL-3.0-or-later
URL:            https://github.com/Vidhan31/tattle
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  cmake >= 3.28
BuildRequires:  ninja-build
BuildRequires:  desktop-file-utils
BuildRequires:  libappstream-glib

BuildRequires:  qt6-rpm-macros
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  qt6-qtmultimedia-devel
BuildRequires:  qt6-qtsvg-devel
BuildRequires:  qtkeychain-qt6-devel

BuildRequires:  libevdev-devel
BuildRequires:  libcap-devel
BuildRequires:  wayland-devel
BuildRequires:  wayland-protocols-devel
BuildRequires:  vulkan-loader-devel
BuildRequires:  vulkan-headers
BuildRequires:  glslc
BuildRequires:  spirv-headers-devel

Requires:       qt6-qtdeclarative%{?_isa}
Requires:       qt6-qtmultimedia%{?_isa}
Requires:       qt6-qtwayland%{?_isa}
Requires:       qtkeychain-qt6%{?_isa}
Requires:       vulkan-loader%{?_isa}
Requires:       wl-clipboard

%description
Tattle is a fast, lightweight speech-to-text desktop application
built with Qt 6 and QML, offering global shortcuts, audio dictation,
and seamless desktop integration on Wayland environments (Plasma 6+, GNOME 49+).

%prep
%autosetup -p1

%build
%cmake -G Ninja \
    -DENABLE_QT_DEPLOYMENT=OFF \
    -DENABLE_VULKAN=ON \
    -DFETCHCONTENT_SOURCE_DIR_TRANSCRIBE=/opt/transcribe-src \
    -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/io.github.tattle.desktop
desktop-file-validate %{buildroot}%{_datadir}/applications/io.github.tattle.daemon.desktop
desktop-file-validate %{buildroot}%{_sysconfdir}/xdg/autostart/io.github.tattle.daemon.desktop
QT_QPA_PLATFORM=offscreen %ctest

%files
%license LICENSE
%doc README.md
%{_bindir}/tattle
%{_bindir}/tattled
%caps(cap_dac_override=p) %{_bindir}/keyinjectord
%{_datadir}/applications/io.github.tattle.desktop
%{_datadir}/applications/io.github.tattle.daemon.desktop
%{_sysconfdir}/xdg/autostart/io.github.tattle.daemon.desktop
%{_datadir}/icons/hicolor/*/apps/io.github.tattle.png
%{_datadir}/icons/hicolor/*/apps/tattle.png
%{_datadir}/icons/hicolor/*/apps/io.github.tattle.svg
%{_datadir}/icons/hicolor/*/apps/tattle.svg

%changelog
* Sun Nov 22 2026 Vidhan Raut <rautvidhan31@gmail.com> - 1.1.0-1
- Release v1.1.0

* Mon Nov 09 2026 Vidhan Raut <rautvidhan31@gmail.com> - 1.0.0-1
- Initial RPM release for Fedora 43

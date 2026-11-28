Name:           storybuilderstudio
Version:        0.1
Release:        1%{?dist}
Summary:        Offline-capable generative narrative and multimedia studio

License:        MIT
URL:            https://github.com/gufymike/StoryBuilderStudio
Source0:        %{url}/archive/v%{version}/StoryBuilderStudio-%{version}.tar.gz

BuildRequires:  cmake >= 3.20
BuildRequires:  ninja-build
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel >= 6.8
BuildRequires:  qt6-qtdeclarative-devel >= 6.8
BuildRequires:  qt6-qtmultimedia-devel >= 6.8
BuildRequires:  qt6-qtsvg-devel >= 6.8
BuildRequires:  desktop-file-utils

%bcond llama 0
%if %{with llama}
BuildRequires:  llama-cpp-devel
%endif

%description
Story Builder Studio is a cross-platform generative narrative studio
orchestrating local multi-modal AI plugins and remote REST APIs.

%prep
%autosetup -n StoryBuilderStudio-%{version}

%build
%cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install
rm -rf %{buildroot}%{_datadir}/StoryBuilderStudio
desktop-file-validate %{buildroot}%{_datadir}/applications/StoryBuilderStudio.desktop

%check
%ctest

%files
%license LICENSE THIRD-PARTY-NOTICES.txt
%doc README.md docs/index.html
%{_bindir}/appStoryBuilderStudio
%{_bindir}/storybuilderstudio.sh
%{_datadir}/applications/StoryBuilderStudio.desktop
%{_datadir}/icons/hicolor/256x256/apps/storybuilderstudio.png
%{_datadir}/doc/StoryBuilderStudio/*

%changelog
* Tue Nov 24 2026 Mike Ramirez <gufymike@gmail.com> - 0.1-1
- Initial RPM package for Story Builder Studio

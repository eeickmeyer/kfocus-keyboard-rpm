%global commit bd27410cee88089699195dac130a8e446817549a
%global module tuxedo-keyboard

Name:           kfocus-keyboard
Version:        3.1.4
Release:        9%{?dist}
Summary:        DKMS keyboard driver for Kubuntu Focus systems
License:        GPL-3.0-or-later
URL:            https://github.com/eeickmeyer/kfocus-keyboard
Source0:        https://github.com/eeickmeyer/kfocus-keyboard/archive/%{commit}.tar.gz
BuildArch:      noarch
Requires:       dkms >= 2.1.0

%description
Keyboard and keyboard backlight driver sources for TUXEDO and Kubuntu Focus
notebooks, installed and built for supported kernels through DKMS.

%prep
%autosetup -n kfocus-keyboard-%{commit}

%build

%install
install -d %{buildroot}%{_usrsrc}/%{module}-%{version}
install -pm 0644 dkms.conf Makefile \
    %{buildroot}%{_usrsrc}/%{module}-%{version}/
cp -a src %{buildroot}%{_usrsrc}/%{module}-%{version}/

install -d %{buildroot}%{_datadir}/kfocus-keyboard
install -d %{buildroot}%{_sysconfdir}/modprobe.d
install -pm 0755 src_pkg/dkms_postinst \
    %{buildroot}%{_datadir}/kfocus-keyboard/postinst
install -pm 0644 tuxedo_keyboard.conf \
    %{buildroot}%{_sysconfdir}/modprobe.d/tuxedo_keyboard.conf

%post
%{_datadir}/kfocus-keyboard/postinst %{module} %{version} \
    %{_datadir}/kfocus-keyboard

%preun
if [ "$1" -eq 0 ] && [ -d "/var/lib/dkms/%{module}/%{version}" ]; then
    dkms remove -m %{module} -v %{version} --all
fi

%files
%license LICENSE
%{_usrsrc}/%{module}-%{version}
%{_datadir}/kfocus-keyboard
%config(noreplace) %{_sysconfdir}/modprobe.d/tuxedo_keyboard.conf

%changelog
* Sat Nov 14 2026 Kubuntu Focus Team <support@kfocus.org> - 3.1.4-9
- Package the DKMS driver for Fedora

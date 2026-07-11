Name:           jobarranger-manager
Version:        6.1.9
Release:        2%{?dist}
Summary:        Job Arranger Manager.
Vendor:         Daiwa Institute of Research Ltd

Group:		Application/Internet
License:        Apache 2.0
URL:            https://www.jobarranger.info/redmine/
Source: 	%{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-root

Requires: php-common >= 7.2 php-common < 8.2
Requires: php-cli >= 7.2 php-cli < 8.2
Requires: php-xml >= 7.2 php-xml < 8.2
Requires: php-pdo >= 7.2 php-pdo < 8.2
Requires: php-fpm >= 7.2 php-fpm < 8.2
Requires: php-mbstring >= 7.2 php-mbstring < 8.2
Requires: php-json >= 7.2 php-json < 8.2
Requires: httpd

%description
Job Arranger Manager is a client terminal for editing job nets and checking job operation status.

%prep
%setup0 -q -n %{name}-%{version}

%install
mkdir -p $RPM_BUILD_ROOT%{_datadir}
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/jobarranger/web
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/httpd/conf.d
cp -a %{_builddir}/%{name}-%{version}/jobarranger %{buildroot}/%{_datadir}
install -Dm 0644 -p jam-cleanup.service $RPM_BUILD_ROOT%{_unitdir}/jam-cleanup.service
install -Dm 0644 -p jobarranger-ui.conf $RPM_BUILD_ROOT%{_sysconfdir}/httpd/conf.d/jobarranger-ui.conf
install -Dm 0644 -p jobarranger-api.conf $RPM_BUILD_ROOT%{_sysconfdir}/httpd/conf.d/jobarranger-api.conf


%clean
rm -rf ${buildroot}

%files
%attr(0755,apache,apache) %{_datadir}/jobarranger
%attr(0755,apache,apache) %{_sysconfdir}/jobarranger/web
%config(noreplace) %{_sysconfdir}/httpd/conf.d/jobarranger-ui.conf
%config(noreplace) %{_sysconfdir}/httpd/conf.d/jobarranger-api.conf
%{_unitdir}/jam-cleanup.service

%pre
if ! rpm -q 'php-mysqlnd' > /dev/null && \
   ! rpm -q 'php-pgsql' > /dev/null && \
   ! rpm -q 'php7.2-mysqlnd' > /dev/null && \
   ! rpm -q 'php7.3-mysqlnd' > /dev/null && \
   ! rpm -q 'php7.4-mysqlnd' > /dev/null && \
   ! rpm -q 'php8.0-mysqlnd' > /dev/null && \
   ! rpm -q 'php8.1-mysqlnd' > /dev/null && \
   ! rpm -q 'php7.2-pgsql' > /dev/null && \
   ! rpm -q 'php7.3-pgsql' > /dev/null && \
   ! rpm -q 'php7.4-pgsql' > /dev/null && \
   ! rpm -q 'php8.0-pgsql' > /dev/null && \
   ! rpm -q 'php8.1-pgsql' > /dev/null; then
    echo "You need to have either php-mysqlnd or php-pgsql installed (supported versions: 7.2 to 8.1)."
    exit 1
fi

%post
%systemd_post jam-cleanup.service
if [ -f %{_sysconfdir}/jobarranger/web/jam.config.php ]; then
 echo "Create new config file as jam.config.php.rpmnew"
 cp %{_datadir}/jobarranger/api/app/config/jam.config.php \
 %{_sysconfdir}/jobarranger/web/jam.config.php.rpmnew
fi

%preun
%systemd_preun jam-cleanup.service
if [ "$1" = "0" ]; then
  # Check if there are files under the web directory and delete them
  if [ -d %{_sysconfdir}/jobarranger/web ]; then
    rm -rf %{_sysconfdir}/jobarranger/web/*
  fi
fi

%postun
%systemd_postun_with_restart jam-cleanup.service

%changelog
* Fri Jan 17 2025 Copyright Daiwa Institute of Research Ltd. All Rights Reserved. <https://www.jobarranger.info/jaz/jaz_release_note.html> 6.1.9
- Lastest RPM relese
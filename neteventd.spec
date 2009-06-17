Name:		neteventd
Version:	0.3.3
Release:	1
Summary:	Track network events for debug purposes
Group:		System Environment/Libraries
License:	GPL
Source0:	%{name}-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:	gcc, autoconf >= 2.59, wireless-tools

%if 0%{?fedora_version} >= 5
BuildRequires:  gcc, autoconf >= 2.59, wireless-tools-devel
%endif

%description
Track network events in the Linux kernel for debug purposes

%prep
%setup -q

%build
./configure --prefix=%{_prefix} --libdir=%{_libdir}
make CFLAGS="$RPM_OPT_FLAGS" %{?_smp_mflags}

%install
make DESTDIR=$RPM_BUILD_ROOT install-strip




%files
	%defattr(-,root,root,-)
	%{_bindir}/neteventd
	%{_libdir}/libnetevent.*
	%{_includedir}/netevent/

%changelog
* Mon Jun 15 2009 Rui Ferreira <rferreira@av.it.pt> 0.3.3
  - Added new files from version 0.3.3
* Tue Jan 06 2009 Rui Ferreira <rferreira@av.it.pt> 0.3.2
  - Initial specfile for neteventd 0.3.2

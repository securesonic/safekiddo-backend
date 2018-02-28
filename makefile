
export PROJECT_ROOT := $(shell pwd)

#the following parameters can be passed only in commandline. If not passed, the values below will be used
BITS := 64
VERSION_MAJOR := 1
VERSION_MINOR := 4
VERSION_UPDATE := 1
PLUGIN_VERSION := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_UPDATE)
VENDOR_STYPE := SafeKiddo 
SAFEKIDDO_VERSION := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_UPDATE)

#the following parameters can be passed in commandline or in environment. If not passed, the values below will be used

ifndef CONFIG
	CONFIG := debug_64
endif

ifndef BUILD_ICAP_PLUGIN
	BUILD_ICAP_PLUGIN := 1
endif
ifndef BUILD_BACKEND
	BUILD_BACKEND := 1
endif

ifndef SK_MAKE_JOBS_NUMBER
	SK_MAKE_JOBS_NUMBER := 4
endif

# these environment variables should be readable in plugin/SConscript
export CONFIG
export BUILD_ICAP_PLUGIN
export BUILD_BACKEND

# HS_COMPILE_WITH_ALLOCATOR can be defined for SafeKiddo
ifndef HS_COMPILE_WITH_ALLOCATOR
	export HS_COMPILE_WITH_ALLOCATOR := 0
endif


_S_ARGS :=
ifdef S_ARGS
	_S_ARGS += $(S_ARGS)
endif
ifeq ($(S_TARGETS),  )
	S_TARGETS := all
endif

ifeq ($(MONIT_OFF),1)
	D_MONIT =
else
	D_MONIT = -d monit 
endif

ifeq ($(AUTOSSH),1)
	AUTOSSH_CONFIG = --config-files $(MONIT_SCRIPTS_DIR)startAutossh.sh 
	AUTOSSH_MONIT = ../c-icap/scripts/monit/monit.d/autossh=/etc/monit/conf.d/
	AUTOSSH_START = ../c-icap/scripts/monit/startAutossh.sh=$(MONIT_SCRIPTS_DIR)
	AUTOSSH_STOP = ../c-icap/scripts/monit/stopAutossh.sh=$(MONIT_SCRIPTS_DIR)
else
	AUTOSSH_CONFIG =
	AUTOSSH_MONIT = 
	AUTOSSH_START =
	AUTOSSH_STOP =
	
endif

.PHONY: daemon plugin plugins allscons integrationTests perfTests \
	protocolPerfTestPlugin emacsBrowser

all: allscons

allscons:
	./compile/scons -j $(SK_MAKE_JOBS_NUMBER) all

#protocolPerfTestPlugin:
#	./compile/scons -j $(OST_MAKE_JOBS_NUMBER) protocolPerfTestPlugin
#
#protocolTestPlugin:
#	./compile/scons -j $(OST_MAKE_JOBS_NUMBER) protocolTestPlugin
#
#integrationTests:
#	./compile/scons -j $(OST_MAKE_JOBS_NUMBER) integrationTests

clean:
	rm bin obj -Rf 2> /dev/null

cleanall:
	make clean
	rm -rf packages/*
	rm -rf run/*
	rm -rf rpm/*
	find . -name "*.pyc" -exec rm -f {} \;
	find . -name "*~" -exec rm -f {} \;
	if [ -e external/projects/zeromq-3.2.4/Makefile ] ; \
	then \
		$(MAKE) -C external/projects/zeromq-3.2.4 distclean ; \
	fi;
	if [ -e external/projects/c_icap-0.3.4/Makefile ] ; \
	then \
		$(MAKE) -C external/projects/c_icap-0.3.4 distclean ; \
	fi;
	rm -rf external/include
	rm -rf external/lib

scons:
	./compile/scons --no-cache -j $(SK_MAKE_JOBS_NUMBER) $(_S_ARGS) $(S_TARGETS)
	
dist-chaos:
	mkdir -p dist/
	cd dist; \
	fpm -t deb -s dir --name "chaos-pp" --version 1.0 --architecture noarch --maintainer "marcin.marzec@safekiddo.com" \
		--license "Boost Software License" --vendor "marcin.marzec@safekiddo.com" --force \
		--url "http://sourceforge.net/projects/chaos-pp/" --description "Chaos preprocessor" \
		--verbose /usr/include/chaos

BIN_DIR := /sk/bin/
CONF_DIR := /sk/etc/
MONIT_SCRIPTS_DIR := /sk/scripts/monit/
LIBRARY_SCRIPTS_DIR := /sk/scripts/library/
SQL_DIR := /sk/sql/
#SAFEKIDDO_VERSION := $(shell git describe | sed s/_/-/)

dist-common-monit-off:
	MONIT_OFF=1 make dist-common 

dist-common:
	mkdir -p dist
	cd dist; \
	fpm -t deb -s dir --name "safekiddo-common" --version $(SAFEKIDDO_VERSION) --architecture noarch --maintainer "safekiddo@9livesdata.com" \
		--license "Ardura License" --vendor "safekiddo@9livesdata.com" --force \
		--url "http://safekiddo.com/" --description "SafeKiddo common files" \
		--config-files /etc/monit/conf.d/monitrc \
		--after-install ../scripts/packaging/common/afterInstall.sh \
		$(D_MONIT) \
		--verbose \
		../scripts/monit/monitrc=/etc/monit/conf.d/ \
		../scripts/sysconfig/sysctl.d/60-safekiddo.conf=/etc/sysctl.d/60-safekiddo.conf

dist-credentials:
	mkdir -p dist
	cd dist; \
	fpm -t deb -s dir --name "safekiddo-backend-credentials" --version $(SAFEKIDDO_VERSION) --architecture noarch --maintainer "safekiddo@9livesdata.com" \
		--license "Ardura License" --vendor "safekiddo@9livesdata.com" --force \
		--url "http://safekiddo.com/" --description "SafeKiddo backend credentials" \
		--verbose \
		../scripts/packaging/pgpass=$(BIN_DIR)

dist-product:
	make cleanall
	echo "Building backend package for version" $(SAFEKIDDO_VERSION)
	CONFIG=release_64_sym_pg93 BUILD_ICAP_PLUGIN=0 BUILD_BACKEND=1 make all
	mkdir -p dist
	cd dist; \
	fpm -t deb -s dir --name "safekiddo-backend" --version $(SAFEKIDDO_VERSION) --architecture amd64 --maintainer "marcin.marzec@safekiddo.com" \
		--license "Ardura License" --vendor "safekiddo@9livesdata.com" --force \
		--url "http://safekiddo.com/" --description "SafeKiddo backend, URL query webservice, scripts and system configuration" \
		--config-files /etc/logrotate.d/safekiddo \
		--config-files $(MONIT_SCRIPTS_DIR)/configuration.sh \
		--config-files $(MONIT_SCRIPTS_DIR)/reference_child_profile_for_icap.out \
		--config-files $(SQL_DIR)/check_child_profile_for_icap.sql \
		--config-files $(CONF_DIR)/admissionControlCustomSettings.accs \
		--after-install ../scripts/packaging/afterInstall.sh \
		-d ntp \
		-d "openssl > 1" -d "libmicrohttpd10 >= 0.9.33" -d libprotobuf8 -d logrotate -dlibpq5 -d xdelta3  -d libzmq3 \
		-d python -d python-dialog \
		-d safekiddo-common \
		-d safekiddo-backend-credentials \
		--verbose \
		../bin/release_64_sym_pg93/backend.exe=$(BIN_DIR) \
		../bin/release_64_sym_pg93/bcdatabase=$(BIN_DIR) \
		../bin/release_64_sym_pg93/bcsdk.cfg=$(BIN_DIR) \
		../bin/release_64_sym_pg93/httpd.exe=$(BIN_DIR) \
		../bin/release_64_sym_pg93/libutils.so=$(BIN_DIR) \
		../bin/release_64_sym_pg93/libBcSdk.so=$(BIN_DIR) \
		../bin/release_64_sym_pg93/libBCUrl.so=$(BIN_DIR) \
		../bin/release_64_sym_pg93/libstats.so=$(BIN_DIR) \
		../bin/release_64_sym_pg93/libwebrootCpp.so=$(BIN_DIR) \
		../bin/release_64_sym_pg93/webrootQuery.exe=$(BIN_DIR) \
		../bin/release_64_sym_pg93/thawteCA.pem=$(BIN_DIR) \
		../scripts/packaging/admissionControlCustomSettings.accs=$(CONF_DIR) \
		../scripts/packaging/date_time_zonespec.csv=$(CONF_DIR) \
		../scripts/monit/backend.sh=$(MONIT_SCRIPTS_DIR) \
		../scripts/monit/child_profile_test.sh=$(MONIT_SCRIPTS_DIR) \
		../scripts/monit/createCrashReport.sh=$(MONIT_SCRIPTS_DIR) \
		../scripts/monit/httpd.sh=$(MONIT_SCRIPTS_DIR) \
		../scripts/monit/reference_child_profile_for_icap.out=$(MONIT_SCRIPTS_DIR) \
		../scripts/monit/configuration.sh=$(MONIT_SCRIPTS_DIR) \
		../scripts/monit/startBackend.sh=$(MONIT_SCRIPTS_DIR) \
		../scripts/monit/stopBackend.sh=$(MONIT_SCRIPTS_DIR) \
		../scripts/monit/httpdHealthCheckFailed.sh=$(MONIT_SCRIPTS_DIR) \
		../scripts/library/__init__.py=$(LIBRARY_SCRIPTS_DIR) \
		../scripts/library/mgmtLib.py=$(LIBRARY_SCRIPTS_DIR) \
		../scripts/monit/startHttpd.sh=$(MONIT_SCRIPTS_DIR) \
		../scripts/monit/stopHttpd.sh=$(MONIT_SCRIPTS_DIR) \
		../scripts/monit/monit.d/=/etc/monit/conf.d \
		../scripts/logrotate/logrotate.d/safekiddo=/etc/logrotate.d/safekiddo \
		../scripts/configurator/configureBackend.py=$(BIN_DIR) \
		../sql/check_child_profile_for_icap.sql=$(SQL_DIR)
	

dist-reports:
	echo "Building reports package for version" $(SAFEKIDDO_VERSION)
	make python-pb
	rm -f web/reports/reportsServer/*.pyc
	rm -f web/reports/reportsServer/reports/*.pyc
	mkdir -p dist
	cd dist; \
	fpm -t deb -s dir --name "safekiddo-reports" --version $(SAFEKIDDO_VERSION) --architecture amd64 --maintainer "safekiddo@9livesdata.com" \
		--license "Ardura License" --vendor "safekiddo@9livesdata.com" --force \
		--url "http://safekiddo.com/" --description "SafeKiddo report webservice" \
		--after-install ../web/reports/scripts/packaging/afterInstall.sh \
		--config-files $(BIN_DIR)web/reports/reportsServer/reportsServer.cfg \
		-d python -d python-cherrypy3 -d python-dateutil -d python-psycopg2 -d python-protobuf \
		-d safekiddo-common \
		-d safekiddo-backend-credentials \
		--verbose \
		../web/reports/reportsServer.py=$(BIN_DIR)web/reports/\
		../web/reports/reportsServer/configs/dist/=$(BIN_DIR)web/reports/reportsServer/\
		../web/reports/reportsServer/reports/=$(BIN_DIR)web/reports/reportsServer/reports/\
		../web/reports/reportsServer/__init__.py=$(BIN_DIR)web/reports/reportsServer/\
		../web/reports/reportsServer/__main__.py=$(BIN_DIR)web/reports/reportsServer/\
		../web/reports/reportsServer/configDbAccess.py=$(BIN_DIR)web/reports/reportsServer/\
		../web/reports/reportsServer/constants.py=$(BIN_DIR)web/reports/reportsServer/\
		../web/reports/reportsServer/dbAccess.py=$(BIN_DIR)web/reports/reportsServer/\
		../web/reports/reportsServer/healthCheck.py=$(BIN_DIR)web/reports/reportsServer/\
		../web/reports/reportsServer/runServer.py=$(BIN_DIR)web/reports/reportsServer/\
		../web/reports/reportsServer/safekiddo_pb2.py=$(BIN_DIR)web/reports/reportsServer/\
		../web/reports/reportsServer/wsUrlResponses.py=$(BIN_DIR)web/reports/reportsServer/\
		../web/reports/scripts/monit/startReportsServer.sh=$(MONIT_SCRIPTS_DIR)\
		../web/reports/scripts/monit/stopReportsServer.sh=$(MONIT_SCRIPTS_DIR)\
		../web/reports/scripts/monit/monit.d/reportsServer=/etc/monit/conf.d/

dist-mdm:
	echo "Building MDM server package for version" $(SAFEKIDDO_VERSION)
	mkdir -p dist
	cd dist; \
	fpm -t deb -s dir --name "safekiddo-mdm" --version $(SAFEKIDDO_VERSION) --architecture amd64 --maintainer "safekiddo@9livesdata.com" \
		--license "Ardura License" --vendor "safekiddo@9livesdata.com" --force \
		--url "http://safekiddo.com/" --description "SafeKiddo MDM server" \
		--after-install ../mdm/scripts/packaging/afterInstall.sh \
		--config-files $(BIN_DIR)mdm/wp8/configs/mdmServer.cfg \
		--config-files $(BIN_DIR)mdm/wp8/configs/blockedBrowsers.py \
		-d python \
		-d python-cherrypy3 -d python-dateutil -d python-psycopg2 -d python-m2crypto \
		-d safekiddo-common \
		--verbose \
		../mdm/wp8/configs/__init__.py=$(BIN_DIR)mdm/wp8/configs/__init__.py\
		../mdm/wp8/configs/packageConfigMdmServer.cfg=$(BIN_DIR)mdm/wp8/configs/mdmServer.cfg\
		../mdm/wp8/configs/blockedBrowsers.py=$(BIN_DIR)mdm/wp8/configs/blockedBrowsers.py\
		../mdm/wp8/discovery.py=$(BIN_DIR)mdm/wp8/\
		../mdm/wp8/enrollment.py=$(BIN_DIR)mdm/wp8/\
		../mdm/wp8/GeoTrust_Global_CA.cer=$(BIN_DIR)mdm/wp8/\
		../mdm/wp8/globalFunctions.py=$(BIN_DIR)mdm/wp8/\
		../mdm/wp8/mdmServer.py=$(BIN_DIR)mdm/wp8/\
		../mdm/wp8/omaDM.py=$(BIN_DIR)mdm/wp8/\
		../mdm/wp8/pushSender.py=$(BIN_DIR)mdm/wp8/\
		../mdm/wp8/xmlDoc.py=$(BIN_DIR)mdm/wp8/\
		../mdm/scripts/monit/startMdmServer.sh=$(MONIT_SCRIPTS_DIR)\
		../mdm/scripts/monit/stopMdmServer.sh=$(MONIT_SCRIPTS_DIR)\
		../mdm/scripts/monit/monit.d/mdmServer=/etc/monit/conf.d/

dist-icap32:
	CONFIG=release_32 AUTOSSH=0 ARCHITECTURE=i386 make dist-icap-generic
	
dist-icap64:
	CONFIG=release_64 AUTOSSH=1 ARCHITECTURE=amd64 make dist-icap-generic

dist-icap-generic:
	make cleanall
	echo "Building c-icap plugin package for version" $(SAFEKIDDO_VERSION)
	BUILD_ICAP_PLUGIN=1 BUILD_BACKEND=0 make all
	mkdir -p dist
	cd dist; \
	fpm -t deb -s dir --name "safekiddo-icap" --version $(SAFEKIDDO_VERSION) --architecture $(ARCHITECTURE) --maintainer "safekiddo@9livesdata.com" \
		--license "Ardura License" --vendor "safekiddo@9livesdata.com" --force \
		--url "http://safekiddo.com/" --description "SafeKiddo ICAP server" \
		--config-files /sk/c-icap/conf/safekiddo-c-icap.conf \
		--config-files /etc/logrotate.d/safekiddo-icap \
		$(AUTOSSH_CONFIG)\
		--after-install ../c-icap/scripts/packaging/afterInstall.sh \
		-d safekiddo-common \
		-d autossh \
		-d logrotate \
		--verbose \
		../c-icap/configs/dist-c-icap.conf=/sk/c-icap/conf/safekiddo-c-icap.conf \
		../c-icap/c-icap.magic=/sk/c-icap/ \
		../bin/$(CONFIG)/libIcapService.so=/sk/c-icap/lib/ \
		../bin/$(CONFIG)/libutils.so=/sk/c-icap/lib/ \
		../c-icap/scripts/monit/startIcap.sh=$(MONIT_SCRIPTS_DIR)\
		../c-icap/scripts/monit/stopIcap.sh=$(MONIT_SCRIPTS_DIR)\
		$(AUTOSSH_START)\
		$(AUTOSSH_STOP)\
		../c-icap/scripts/monit/monit.d/icap=/etc/monit/conf.d/\
		$(AUTOSSH_MONIT)\
		../c-icap/scripts/logrotate/logrotate.d/safekiddo-icap=/etc/logrotate.d/safekiddo-icap
		
python-pb:
	protoc --proto_path=src/modules/proto/ --python_out=web/reports/reportsServer/ src/modules/proto/safekiddo.proto 

dist-all: dist-common dist-credentials dist-product dist-reports dist-mdm
dist-all-icap: dist-common dist-icap

#preReleaseTests: all
#	../build/scripts/runTests -d preReleaseTests


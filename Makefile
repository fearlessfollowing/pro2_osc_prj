# topdir Makefile

include ./Rules.mk


all: monitor catlog logserver pro2service updateapp voldserver
	@echo 'Finished building target: $@'

monitor:
	make -C $(MONITOR_DIR)/ clean
	make -C $(MONITOR_DIR)/ 

monitor_clean:
	@cd ./build/aarch64-linux-gnu/obj/pro2_service
	@rm $(shell find -name *.[od])

catlog:
	#make -C $(LOGCAT_DIR)/ clean
	make -C $(LOGCAT_DIR)/

logserver:
	make -C $(LOGD_DIR)/

pro2service:
	make -C $(PRO2_SERVICE)/

updatecheck:
	make -C $(UPDATE_CHECK)/

updateapp:
	make -C $(UPDATE_APP)/

voldserver:
	make -C $(VOLD_DIR)/

clean:
	@cd ./build
	@rm $(shell find -name *.[od])
	@cd -
help:
	@echo "----------------------------------------------------"
	@echo "make all: >>>>>>>>>>>>>>>>>> [BUILD all moudles]"
	@echo "make monitir: >>>>>>>>>>>>>> [BUILD monitor module]"
	@echo "make monitir_clean: >>>>>>>> [BUILD monitor clean module]"
	@echo "make catlog: >>>>>>>>>>>>>>> [BUILD logcat module]"
	@echo "make logserver: >>>>>>>>>>>> [BUILD logd module]" 
	@echo "make pro2service: >>>>>>>>>> [BUILD pro2_service module]" 
	@echo "make updatecheck: >>>>>>>>>> [BUILD update_check module]" 
	@echo "make updateapp: >>>>>>>>>>>> [BUILD update_app module]" 
	@echo "make voldserver: >>>>>>>>>>>>> [BUILD vold module]" 
	@echo "make clean: >>>>>>>>>>>>>>>> [CLEAN all module module]" 
	@echo "----------------------------------------------------"


	

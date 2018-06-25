# common_cc.mk
# Copyright 2016-2020 BitEye, Inc.  All rights reserved.
#
#

.PHONY: $(CEXEC) $(CPPEXEC) $(LIB)

LDLIBS += $(TARGET_LDLIBS)

ifdef CEXEC
$(CEXEC): $(BUILD)/$(CEXEC)
$(BUILD)/$(CEXEC): $(BUILD) $(COBJS)
	@$(CC) -o $@ $(COBJS) $(LDFLAGS) $(LDLIBS)
	@$(STRIP) $@
	cp $@ $(INSTALL_ROOT)/
endif

ifdef CPPEXEC
$(CPPEXEC): $(BUILD)/$(CPPEXEC)
$(BUILD)/$(CPPEXEC): $(BUILD) $(CPPOBJS) $(COBJS)
	@$(CPLUSPLUS) -o $@ $(CPPOBJS) $(COBJS) $(LDFLAGS) $(LDLIBS)
	@$(STRIP) $@
	cp $@ $(INSTALL_ROOT)/
endif


ifdef CLIB
$(LIB): $(BUILD)/$(CLIB)
$(BUILD)/$(CLIB): $(BUILD) $(CSTYLES) $(COBJS) $(CSTYLES)
	$(AR) r $@ $(COBJS)
endif

ifdef CPPLIB
$(CPPLIB): $(BUILD)/$(CPPLIB)
$(BUILD)/$(CPPLIB): $(BUILD) $(CPPOBJS) 
	$(AR) r $@ $(CPPOBJS)
endif

$(BUILD)/%.d: %.c Makefile
	@(mkdir -p $(@D); $(CC) -MM $(CPPFLAGS) $(CFLAGS) $< | \
        	sed 's,\($*\)\.o[ :]*,$(BUILD)/\1.o $@: ,g' > $@) || rm -f $@

$(BUILD)/%.d: %.cpp Makefile
	@(mkdir -p $(@D); $(CPLUSPLUS) -MM $(CPPFLAGS) $(CFLAGS) $< | \
        	sed 's,\($*\)\.o[ :]*,$(BUILD)/\1.o $@: ,g' > $@) || rm -f $@

-include $(CDEPS)
-include $(CXXDEPS)

$(BUILD)/%.o: %.c Makefile
	@echo CC $< ; \
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD)/%.o: %.cpp Makefile
	@echo CPLUSPLUS $< ; \
	$(CPLUSPLUS) -c $(CXXFLAGS) -o $@ $<


$(BUILD):
	mkdir -p $(BUILD)


$(INSTALL_ROOT):
	mkdir -p $(SDK_DIR)/out/bin

.PHONY: clean

clean:
	@rm -f $(COBJS) $(CDEPS) $(CPPOBJS) $(CPPDEPS) 
	@cd $(BUILD); rm -f $(CEXEC) $(CPPEXEC) $(LIB)

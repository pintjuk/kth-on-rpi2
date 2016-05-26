TARGET=maxflow
SRC_DIRS += src 
#INC_DIRS += ../../library/include
DEPENDS = ../../library/uc ../../drivers ../../library/mem ../../library/guest ../../library/util 
LIB_FILES=guest uc mem util drivers

LDSCRIPT = linker.ld

# get platform dependent flags
include ../../target
include ../../templates/platform/$(PLATFORM).cfg
include ../../templates/cpu/$(PLATFORM_CPU).cfg


# include templates
include ../../templates/make/toolchain.inc
include ../../templates/make/generic.inc
include ../../templates/make/application.inc


# force update when something changes
$(OBJ_FILES): Makefile ../../target

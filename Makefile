# -*- Mode: makefile-gmake -*-

.PHONY: clean all release

#
# Required packages
#
PKGS = glib-2.0 gio-2.0 gio-unix-2.0 dbus-1 bluez-libs-devel

#
# Default target
#
all: release

#
# Library name
#
NAME = sailfish-rfkill-plugin
LIB_NAME = $(NAME)
LIB_SONAME = $(LIB_NAME).so
LIB = $(LIB_SONAME)

#
# Sources
#
SRC = \
 sailfish-rfkill.c \

#
# Directories
#
SRC_DIR = src
BUILD_DIR = build
SPEC_DIR = spec
RELEASE_BUILD_DIR = $(BUILD_DIR)/release

#
# Tools and flags
#
CC = $(CROSS_COMPILE)gcc
LD = $(CC)
WARNINGS = -Wall
BASE_FLAGS = -fPIC
FULL_CFLAGS = $(BASE_FLAGS) $(CFLAGS) $(DEFINES) $(WARNINGS) \
  -MMD -MP $(shell pkg-config --cflags $(PKGS))
FULL_LDFLAGS = $(BASE_FLAGS) $(LDFLAGS) -shared -Wl,-soname -Wl,$(LIB_SONAME) \
  $(shell pkg-config --libs $(PKGS))
RELEASE_FLAGS = -g

RELEASE_LDFLAGS = $(FULL_LDFLAGS) $(RELEASE_FLAGS)
RELEASE_CFLAGS = $(FULL_CFLAGS) $(RELEASE_FLAGS) -O2

#
# Files
#
RELEASE_OBJS = $(SRC:%.c=$(RELEASE_BUILD_DIR)/%.o)

#
# Dependencies
#
DEPS = $(RELEASE_OBJS:%.o=%.d)
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(DEPS)),)
-include $(DEPS)
endif
endif

$(RELEASE_OBJS) $(RELEASE_LIB): | $(RELEASE_BUILD_DIR)

#
# Rules
#
RELEASE_LIB = $(RELEASE_BUILD_DIR)/$(LIB)

release: $(RELEASE_LIB)

clean:
	rm -f *~ $(SRC_DIR)/*~
	rm -fr $(BUILD_DIR) RPMS installroot

$(RELEASE_BUILD_DIR):
	mkdir -p $@

$(RELEASE_BUILD_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c $(RELEASE_CFLAGS) -MT"$@" -MF"$(@:%.o=%.d)" $< -o $@

$(RELEASE_LIB): $(RELEASE_OBJS)
	$(LD) $(RELEASE_OBJS) $(RELEASE_LDFLAGS) -o $@

#
# Install
#
INSTALL_PERM  = 755
INSTALL_OWNER = $(shell id -u)
INSTALL_GROUP = $(shell id -g)
INSTALL = install
INSTALL_DIRS = $(INSTALL) -d
INSTALL_FILES = $(INSTALL) -m $(INSTALL_PERM)
INSTALL_LIB_DIR = $(DESTDIR)/usr/lib/connman/plugins

install: $(INSTALL_LIB_DIR)
	$(INSTALL_FILES) $(RELEASE_LIB) $(INSTALL_LIB_DIR)

$(INSTALL_LIB_DIR):
	$(INSTALL_DIRS) $@

EXTENSION = gp_compute
VRESION = 0.1
MODULE_big = gp_compute_client
PROGRAM = gp_compute_server

GPDB_DIR = ../lib/gpdb/src/backend
PLR_DIR = ../lib/plr

PG_CONFIG = pg_config
PG_CPPFLAGS = -I$(GPDB_DIR) -I$(GPDB_DIR)/../include
DATA_DIR = ../data
DATA = $(EXTENSION).control $(DATA_DIR)/$(EXTENSION)--$(VRESION).sql $(DATA_DIR)/$(EXTENSION)--uninstall.sql

FILES = $(shell find . -type f -name "*.c")
OBJS = $(foreach FILE,$(FILES),$(subst .c,.o,$(FILE)))
OBJS += $(libdir)/libpostgres.so
OBJS += $(libdir)/postgresql/plr.so


PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
override CFLAGS += -Wextra -Wno-unused-parameter
override PG_LIBS = -lpostgres

.PHONY: $(libdir)/libpostgres.so
$(libdir)/libpostgres.so:
	@make -C $(GPDB_DIR) all-lib
	@make -C $(GPDB_DIR) install-lib

.PHONY: $(libdir)/postgresql/plr.so
$(libdir)/postgresql/plr.so:
	@USE_PGXS=1 make -C $(PLR_DIR) install

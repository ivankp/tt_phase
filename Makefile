SHELL := bash
CXX := g++
CPPFLAGS := -std=c++14 -Iinclude
CXXFLAGS := -Wall -O3 -fmax-errors=3 $(CPPFLAGS)
# CXXFLAGS := -Wall -g -fmax-errors=3 $(CPPFLAGS)
LDFLAGS :=
LDLIBS :=

SRC := src
BIN := bin
BLD := .build
EXT := .cc

.PHONY: all clean

ifeq (0, $(words $(findstring $(MAKECMDGOALS), clean)))

LDFLAGS += $(shell sed -r 's/([^:]+)(:|$$)/ -L\1/g' <<< "$$LIBRARY_PATH")

ROOT_CXXFLAGS := $(shell root-config --cflags | sed 's/ -std=c++[^ ]\+//')
ROOT_LDFLAGS  := $(shell root-config --ldflags)
ROOT_LDLIBS   := $(shell root-config --libs)

# RPATH
rpath_script := ldd $(shell root-config --libdir)/libTreePlayer.so \
  | sed -nr 's|.*=> (.+)/.+\.so[.0-9]* \(.*|\1|p' \
  | sort -u \
  | sed -nr '/^(\/usr)?\/lib/!s/^/-Wl,-rpath=/p'
ROOT_LDLIBS += $(shell $(rpath_script))

ROOT_NOLIBS := $(shell sed 's/ -[lr][^ ]\+//g' <<< '$(ROOT_LDLIBS)')

LDFLAGS += $(ROOT_LDFLAGS)

SRCS := $(shell find -L $(SRC) -type f -name '*$(EXT)')
DEPS := $(patsubst $(SRC)/%$(EXT),$(BLD)/%.d,$(SRCS))

GREP_EXES := grep -rl '^ *int \+main *(' $(SRC) --include='*$(EXT)'
EXES := $(patsubst $(SRC)%$(EXT),$(BIN)%,$(shell $(GREP_EXES)))

PO_OBJ := $(BLD)/ivanp/program_options/program_options.o

C_vars := $(ROOT_CXXFLAGS)
L_vars := $(ROOT_LDLIBS) -lTreePlayer

C_fit := -fopenmp $(ROOT_CXXFLAGS)
L_fit := -fopenmp $(ROOT_NOLIBS) -lCore -lMinuit

C_draw := $(ROOT_CXXFLAGS)
L_draw := $(ROOT_LDLIBS)

all: $(EXES)

$(EXES): $(PO_OBJ)
$(BIN)/vars: $(BLD)/glob.o

-include $(DEPS)

.SECONDEXPANSION:

$(DEPS): $(BLD)/%.d: $(SRC)/%$(EXT) | $(BLD)/$$(dir %)
	$(CXX) $(CPPFLAGS) $(C_$*) -MM -MT '$(@:.d=.o)' $< -MF $@

$(BLD)/%.o: | $(BLD)
	$(CXX) $(CXXFLAGS) $(C_$*) -c $(filter %$(EXT),$^) -o $@

$(BIN)/%: $(BLD)/%.o | $(BIN)
	$(CXX) $(LDFLAGS) $(filter %.o,$^) -o $@ $(LDLIBS) $(L_$*)

$(BIN):
	mkdir -p $@

$(BLD)/%/:
	mkdir -p $@

endif

clean:
	@rm -rfv $(BLD) $(BIN)

CXX= gcc
# CXXFLAGS= 	-g -gdwarf-2 -std=gnu++11 -Wall -Iinclude -fPIC
CXXFLAGS=   -Wall -Iinclude -fPIC -g
LDFLAGS=	-Llib
AR=			ar
ARFLAGS=	rcs

LIB_HEADERS=	$(wildcard include/sfs/*.h)
LIB_SOURCE=	$(wildcard src/library/*.c)
LIB_OBJECTS=	$(LIB_SOURCE:.c=.o)
LIB_STATIC=	lib/libsfs.a

SHELL_SOURCE=	$(wildcard src/shell/*.c)
SHELL_OBJECTS=	$(SHELL_SOURCE:.c=.o)
SHELL_PROGRAM=	bin/sfssh

all:	$(LIB_STATIC) $(SHELL_PROGRAM)

%.o:	%.c $(LIB_HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(LIB_STATIC):		$(LIB_OBJECTS) $(LIB_HEADERS)
	$(AR) $(ARFLAGS) $@ $(LIB_OBJECTS)

$(SHELL_PROGRAM):	$(SHELL_OBJECTS) $(LIB_STATIC)
	$(CXX) $(LDFLAGS) -o $@ $(SHELL_OBJECTS) -lsfs -lm

test:	$(SHELL_PROGRAM)
	@for test_script in tests/test_*.sh; do $${test_script}; done

test2:
	@for test_script in tests/double-indirect/test_*.sh; do $${test_script}; done

clean:
	rm -f $(LIB_OBJECTS) $(LIB_STATIC) $(SHELL_OBJECTS) $(SHELL_PROGRAM)

.PHONY: all clean

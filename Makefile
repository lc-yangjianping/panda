#
#

BASE = ${PWD}

MAKE_DIR  = ${BASE}/make
ESBL_DIR  = ${BASE}/esbl
LANG_DIR  = ${BASE}/lang
CUNIT_DIR = ${BASE}/cunit

TEST_DIR  = ${BASE}/test

CC = gcc
LD = gcc
RM = rm -rf
ECHO = echo
MAKE = make


CPPFLAGS = -I${BASE}
CFLAGS = -g -Wall -Werror
LDFLAGS = -L${CUNIT_DIR} -L${ESBL_DIR} -L${LANG_DIR}

export CC LD RM ECHO MAKE
export CPPFLAGS
export CFLAGS
export LDFLAGS

.PHONY: all test cunit esbl lang leo

all: test

cunit:
	${MAKE} -C cunit -f ${MAKE_DIR}/Makefile.pub

esbl:
	${MAKE} -C esbl -f ${MAKE_DIR}/Makefile.pub

lang:
	${MAKE} -C lang -f ${MAKE_DIR}/Makefile.pub

test: clean cunit esbl lang
	${MAKE} -C test -f ${MAKE_DIR}/Makefile.pub
	${TEST_DIR}/test

leo: test
	${MAKE} -C leo -f ${MAKE_DIR}/Makefile.pub

clean:
	${MAKE} -C cunit clean -f ${MAKE_DIR}/Makefile.pub
	${MAKE} -C esbl clean -f ${MAKE_DIR}/Makefile.pub
	${MAKE} -C lang clean -f ${MAKE_DIR}/Makefile.pub
	${MAKE} -C test clean -f ${MAKE_DIR}/Makefile.pub
	${MAKE} -C leo  clean -f ${MAKE_DIR}/Makefile.pub


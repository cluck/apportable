
MACOSPY = $(shell find "$(HOME)/Library/Python/" -name bin -type d -maxdepth 2 -exec echo -n "{}:" \; 2>/dev/null)
WINPY = $(shell find "/$(SYSTEMDRIVE)/"Python* -name bin -type d -maxdepth 2 -exec echo -n "{}:" \; 2>/dev/null)
PATHPREP = $(PATH):$(HOME)/.local/bin:$(MACOSPY):$(WINPY)
PYTHON := $(shell PATH="$(PATHPREP)" which python3 python2 python | head -n1)
PATH := $(PATHPREP)
ifeq ($(OS),Windows_NT)
	BINEXT = .exe
else
	BINEXT =
endif

apportable_demo: apportable.c apportable_demo.c
	$(CC) -liconv -DAPPORTABLE -o apportable_demo apportable.c apportable_demo.c

apportable_demo.exe: apportable.c apportable_demo.c
	$(CC) -DAPPORTABLE -o apportable_demo.exe apportable.c apportable_demo.c

demo: apportable_demo$(BINEXT)

all: demo

build/lib/.build_stamp: setup.py apportable.c apportable_pyext.c
	mkdir -p build
	$(PYTHON) setup.py build_ext -b build/lib
	touch build/lib/.build_stamp

build_ext: build/lib/.build_stamp

test: build_ext
	PYTHONPATH=build/lib $(PYTHON) test_apportable.py --verbose

clean:
	rm -f apportable build/lib/* build/lib/.build_stamp apportable_demo apportable_demo.exe



.PHONY: all test build_ext


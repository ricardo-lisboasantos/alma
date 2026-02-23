.PHONY: all debug release test clean install

all: release

debug:
	meson setup build -Dbuildtype=debug -Dprefix=$$(pwd)
	meson compile -C build
	meson install -C build

release:
	meson setup build -Dbuildtype=release -Dprefix=$$(pwd)
	meson compile -C build
	meson install -C build

test:
	meson compile -C build
	meson test -C build

clean:
	rm -rf build dist

install: release

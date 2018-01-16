
import sys

from setuptools import setup, Extension

if sys.platform != 'win32':
	ext_libs = ['iconv']
else:
	ext_libs = []


_apportable = Extension(
		'_apportable',
		define_macros = [('APPORTABLE', '1')],
		libraries = [] + ext_libs,
        sources = ['apportable.c', 'apportable_pyext.c']
	)


setup(
		name = 'libapportable-test',
		version = '1.0',
		description = 'Apportable tests',
		test_suite = "tests",
		ext_modules = [_apportable],
	)

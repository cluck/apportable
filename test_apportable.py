#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function

try:
	unicode
except:
	unicode = str

import sys
import os
import subprocess

import unittest

try:
	import apportable
except ImportError:
	sys.path.append("build/lib")
	import apportable


class TestClass(unittest.TestCase):

	t1 = u"abcde"
	t2 = u"äβ©☃☂"

	def test_strndup(self):
		a = apportable

		for t in (self.t1, self.t2):
			self.assertEqual( a.strndup(t, 0), t )
			for x in range(1, len(t)+1):
				# self.assertEqual( repr(a.strndup(t, x)), repr(t[0:x]) )
				self.assertEqual( a.strndup(t, x), t[0:x] )

	def test_wcsndup(self):
		a = apportable

		for t in (self.t1, self.t2):
			self.assertEqual( a.wcsndup(t, 0), t )
			for x in range(1, len(t)+1):
				self.assertEqual( a.wcsndup(t, x), t[0:x] )

	def test_wutf8(self):
		a = apportable

		for t in (self.t1, self.t2):
			self.assertEqual( a.wutf8(t), t)

	def test_uwchar_t(self):
		a = apportable

		for t in (self.t1, self.t2):
			self.assertEqual( a.uwchar_t(t), t)

	def test_whereis(self):
		a = apportable

		pth = unicode(os.environ.get("PATH", "/opt:/bin"))
		wherepy = subprocess.check_output("whereis python", shell=True)
		wherepy = wherepy.decode('utf-8').split('\n')[0]
		self.assertEqual(a.whereis(pth,  u"python", 1), wherepy)
		self.assertEqual(a.whereis(u"/opt:/etc",  u"hosts", 1), "hosts")
		self.assertEqual(a.whereis(u"/opt:/etc",  u"hosts", 0), "/etc/hosts")
		self.assertEqual(a.whereis(u"::::",  u"hosts", 0), "hosts")
		self.assertEqual(a.whereis(u"",  u"hosts", 0), "hosts")

	def test_pathexp(self):
		a = apportable

		b = u"/some/fixed/pgm"
		d = u"/other/place"
		t1 = u"$ORIGIN/../variable/path"
		t2 = u"$ORIGIN../variable/path"
		t3 = u"$ORIGIN"
		r = u"/some/fixed/../variable/path"
		r3 = u"/some/fixed/"

		self.assertEqual(a.pathexp(d, b), d)
		self.assertEqual(a.pathexp(t1, b), r)
		self.assertEqual(a.pathexp(t2, b), r)
		self.assertEqual(a.pathexp(t3, b), r3)
		self.assertEqual(a.pathexp(u"", u""), u"")

	def test_ugetenv(self):
		a = apportable

		for F in (a.ugetenv, a.wugetenv):

			self.assertEqual( F(u"PATH"), os.environ.get("PATH"))
			self.assertEqual( F(u"NONEXISTENT"), u"")



	# def test_idea(self):

	# 	# print( dir(apportable) )

	# 	# print( apportable.ping() )
	# 	pth = unicode(os.environ["PATH"])
	# 	# pth = "/test::/usr/bin:"

	# 	cmd = sys.executable

	# 	if sys.platform in ("linux", "linux2"):
	# 		lib = "libc.so.6"
	# 	elif sys.platform == "darwin":
	# 		lib = "/usr/lib/libSystem.B.dylib"
	# 	elif sys.platform == "win32":
	# 		lib = "advapi32.dll"
	# 	else:
	# 		raise NotImplementedError(sys.platform)

	# 	print("progfile", None, apportable.progfile(None))
	# 	print("progfile", lib, apportable.progfile(lib))
	# 	print("progfile", cmd, apportable.progfile(cmd))
	# 	cmd = os.path.basename(cmd)
	# 	print("progfile", cmd, apportable.progfile(cmd))

	# 	# ret = apportable.progfile(None)
	# 	# print(repr(ret))



if __name__ == '__main__':
	
	# print( apportable.selftest() )
	unittest.main()
	# a = apportable
	# print( a.strndup(u"äβ©☃☂", 4) )
    

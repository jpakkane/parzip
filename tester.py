#!/usr/bin/env python3

# Copyright (C) 2016 Jussi Pakkanen.
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of version 3, or (at your option) any later version,
# of the GNU General Public License as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


import os, sys, unittest, tempfile, subprocess
from zipfile import ZipFile

datadir = None
unzip_exe = None

class TestUnzip(unittest.TestCase):

    def check_same(self, zipname):
        zfile = os.path.join(datadir, zipname)
        self.assertTrue(os.path.isfile(zfile))
        with tempfile.TemporaryDirectory() as pdir:
            with tempfile.TemporaryDirectory() as testdir:
                with ZipFile(zfile) as zf:
                    zf.extractall(path=pdir)
                    subprocess.check_call([unzip_exe, zfile], cwd=testdir)
                    self.is_dir_subset(pdir, testdir)
                    self.is_dir_subset(testdir, pdir)

    def is_dir_subset(self, dir1, dir2):
        chop_ind = len(dir1)
        for root, dirs, files in os.walk(dir1):
            root2 = dir2 + root[chop_ind:]
            for d in dirs:
                d2 = os.path.isdir(os.path.join(root2, d))
                self.assertTrue(d2)
            for f in files:
                f1 = os.path.join(root, f)
                f2 = os.path.join(root2, f)
                self.assertTrue(os.path.isfile(f1))
                self.assertTrue(os.path.isfile(f2)) 
                self.files_identical(f1, f2)

    def files_identical(self, fn1, fn2):
        f1 = open(fn1, 'rb')
        d1 = f1.read()
        f1.close()
        f2 = open(fn2, 'rb')
        d2 = f2.read()
        f2.close()
        self.assertEqual(d1, d2)

    def test_deflate(self):
        self.check_same('basic.zip')

    def test_store(self):
        self.check_same('basic.zip')

if __name__ == '__main__':
    datadir = os.path.join(sys.argv[1], 'testdata')
    unzip_exe = os.path.join(sys.argv[2], 'junzip')
    if not os.path.isabs(datadir):
        datadir = os.path.join(os.getcwd(), datadir)
    if not os.path.isabs(unzip_exe):
        unzip_exe = os.path.join(os.getcwd(), unzip_exe)
    sys.argv = sys.argv[0:1] + sys.argv[3:]
    unittest.main()

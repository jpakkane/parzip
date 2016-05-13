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


import os, sys, stat, unittest, tempfile, subprocess
from zipfile import ZipFile

datadir = None
unzip_exe = None

class ZipTestBase(unittest.TestCase):

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
        self.file_content_identical(fn1, fn2)
        # Python's zipfile does not preserve metadata. :(
        # self.file_metadata_identical(fn1, fn2)

    def file_metadata_identical(self, fn1, fn2):
        stat1 = os.stat(fn1)
        stat2 = os.stat(fn2)
        self.assertEqual(stat1.st_mode, stat2.st_mode)

    def file_content_identical(self, fn1, fn2):
        f1 = open(fn1, 'rb')
        d1 = f1.read()
        f1.close()
        f2 = open(fn2, 'rb')
        d2 = f2.read()
        f2.close()
        self.assertEqual(d1, d2)

    def dirs_equal(self, dir1, dir2):
        self.is_dir_subset(dir1, dir2)
        self.is_dir_subset(dir2, dir1)

class TestUnzip(ZipTestBase):

    def check_same(self, zipname):
        zfile = os.path.join(datadir, zipname)
        self.assertTrue(os.path.isfile(zfile))
        with tempfile.TemporaryDirectory() as pdir:
            with tempfile.TemporaryDirectory() as testdir:
                with ZipFile(zfile) as zf:
                    zf.extractall(path=pdir)
                    subprocess.check_call([unzip_exe, zfile], cwd=testdir)
                    self.dirs_equal(pdir, testdir)

    def test_deflate(self):
        self.check_same('basic.zip')

    def test_store(self):
        self.check_same('small.zip')

    def test_subdir(self):
        self.check_same('subdirs.zip')

    def test_many_files(self):
        self.check_same('manyfiles.zip')

    def test_dir_entry(self):
        self.check_same('direntry.zip')

    def test_zip64(self):
        self.check_same('zip64.zip')

    def test_lzma(self):
        self.check_same('lzma.zip')

    def test_7zip_win(self):
        self.check_same('windir.zip')

    def test_unix_permissions(self):
        # Python does not preserve file permissions. Do this manually.
        # https://bugs.python.org/issue15795
        zfile = os.path.join(datadir, "unixperms.zip")
        with tempfile.TemporaryDirectory() as testdir:
            with ZipFile(zfile) as zf:
                subprocess.check_call([unzip_exe, zfile], cwd=testdir)
                outfile = os.path.join(testdir, 'script.py')
                stats = os.stat(outfile)
                self.assertEqual(stats.st_mode, 33261)

    def test_symlink(self):
        zfile = os.path.join(datadir, "symlink.zip")
        with tempfile.TemporaryDirectory() as testdir:
            with ZipFile(zfile) as zf:
                subprocess.check_call([unzip_exe, zfile], cwd=testdir)
                outfile = os.path.join(testdir, 'source.txt')
                outsymlink = os.path.join(testdir, 'symlink.txt')
                fstats = os.lstat(outfile)
                lstats = os.lstat(outsymlink)
                self.assertTrue(stat.S_ISREG(fstats.st_mode))
                self.assertTrue(stat.S_ISLNK(lstats.st_mode))
                self.assertEqual(os.readlink(outsymlink), 'source.txt')

if __name__ == '__main__':
    datadir = os.path.join(sys.argv[1], 'testdata')
    unzip_exe = os.path.join(sys.argv[2], 'junzip')
    if not os.path.isabs(datadir):
        datadir = os.path.join(os.getcwd(), datadir)
    if not os.path.isabs(unzip_exe):
        unzip_exe = os.path.join(os.getcwd(), unzip_exe)
    assert(os.path.isdir(datadir))
    assert(os.path.isfile(unzip_exe))
    sys.argv = sys.argv[0:1] + sys.argv[3:]
    unittest.main()

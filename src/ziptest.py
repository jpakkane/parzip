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
import random
from zipfile import ZipFile
from unziptest import ZipTestBase

datadir = None
unzip_exe = None
zip_exe = None

class TestUnzip(ZipTestBase):

    def test_basic(self):
        zfile = 'zfile.zip'
        datafile = 'inputdata.txt'
        with tempfile.TemporaryDirectory() as packdir:
            with tempfile.TemporaryDirectory() as unpackdir:
                with open(os.path.join(packdir, datafile), 'w') as dfile:
                    dfile.write('This is some text for compression.\n' * 10)
                subprocess.check_call([zip_exe, zfile, datafile], cwd=packdir)
                zf_abs = os.path.join(packdir, zfile)
                z = ZipFile(zf_abs)
                self.assertEqual(len(z.namelist()), 1)
                z.extractall(unpackdir)
                z.close()
                os.unlink(zf_abs)
                self.dirs_equal(packdir, unpackdir)

    def test_dir(self):
        zfile = 'zfile.zip'
        dirname = 'a_subdir'
        with tempfile.TemporaryDirectory() as packdir:
            with tempfile.TemporaryDirectory() as unpackdir:
                os.mkdir(os.path.join(packdir, dirname))
                subprocess.check_call([zip_exe, zfile, dirname], cwd=packdir)
                zf_abs = os.path.join(packdir, zfile)
                z = ZipFile(zf_abs)
                self.assertEqual(len(z.namelist()), 1)
                z.extractall(unpackdir)
                z.close()
                os.unlink(zf_abs)
                self.dirs_equal(packdir, unpackdir)

    def test_big(self):
        zfile = 'zfile.zip'
        datafile = 'inputdata.txt'
        msg = ['I', ' ', 'a', 'm', ' ', 's', 'o', 'm', 'e', ' ', 't', 'e', 'x', 't', ' ',
               't', 'o', ' ', 'b', 'e', ' ', 'r', 'a', 'n', 'd', 'o', 'm', 'i', 's', 'e', 'd.']
        with tempfile.TemporaryDirectory() as packdir:
            with tempfile.TemporaryDirectory() as unpackdir:
                with open(os.path.join(packdir, datafile), 'w') as dfile:
                    for i in range(10000):
                        random.shuffle(msg)
                        dfile.write(''.join(msg)) # Big enough to cause more than one block of output.
                subprocess.check_call([zip_exe, zfile, datafile], cwd=packdir)
                zf_abs = os.path.join(packdir, zfile)
                z = ZipFile(zf_abs)
                self.assertEqual(len(z.namelist()), 1)
                z.extractall(unpackdir)
                z.close()
                os.unlink(zf_abs)
                self.dirs_equal(packdir, unpackdir)

    def test_two(self):
        zfile = 'zfile.zip'
        datafile1 = 'inputdata.txt'
        datafile2 = 'inputdata2.txt'
        with tempfile.TemporaryDirectory() as packdir:
            with tempfile.TemporaryDirectory() as unpackdir:
                with open(os.path.join(packdir, datafile1), 'w') as dfile:
                    dfile.write('This is first text for compression.\n' * 10)
                with open(os.path.join(packdir, datafile2), 'w') as dfile:
                    dfile.write('This is second text for compression.\n' * 10)
                subprocess.check_call([zip_exe, zfile, datafile1, datafile2], cwd=packdir)
                zf_abs = os.path.join(packdir, zfile)
                z = ZipFile(zf_abs)
                self.assertEqual(len(z.namelist()), 2)
                z.extractall(unpackdir)
                z.close()
                os.unlink(zf_abs)
                self.dirs_equal(packdir, unpackdir)

    def test_subdir(self):
        zfile = 'zfile.zip'
        datadir = 'subdir'
        with tempfile.TemporaryDirectory() as packdir:
            with tempfile.TemporaryDirectory() as unpackdir:
                os.makedirs(os.path.join(packdir, 'subdir/subsubdir'))
                with open(os.path.join(packdir, 'subdir/subfile.txt'), 'w') as dfile:
                    dfile.write('This is a file in the subdirectory.\n')
                with open(os.path.join(packdir, 'subdir/subsubdir/subsubfile.txt'), 'w') as dfile:
                    dfile.write('This is a file in the subsubdir.\n')
                subprocess.check_call([zip_exe, zfile, datadir], cwd=packdir)
                zf_abs = os.path.join(packdir, zfile)
                z = ZipFile(zf_abs)
                self.assertEqual(len(z.namelist()), 4)
                z.extractall(unpackdir)
                z.close()
                os.unlink(zf_abs)
                self.dirs_equal(packdir, unpackdir)

    def test_abs(self):
        self.assertNotEqual(subprocess.call([zip_exe, 'foobar.zip', __file__]), 0)

if __name__ == '__main__':
    datadir = os.path.join(sys.argv[1], 'testdata')
    unzip_exe = os.path.join(sys.argv[2], 'junzip')
    zip_exe = os.path.join(sys.argv[2], 'jzip')
    if not os.path.isabs(datadir):
        datadir = os.path.join(os.getcwd(), datadir)
    if not os.path.isabs(unzip_exe):
        unzip_exe = os.path.join(os.getcwd(), unzip_exe)
    assert(os.path.isdir(datadir))
    assert(os.path.isfile(unzip_exe))
    assert(os.path.isfile(zip_exe))
    sys.argv = sys.argv[0:1] + sys.argv[3:]
    unittest.main()

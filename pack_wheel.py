# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Build a pyeclib wheel that contains precompiled liberasurecode libraries.

The goal is to build manylinux, abi3 wheels that are actually useful.

   - ``manylinux`` ensures installability on a variety of distributions,
     almost regardless of libc version.
   - ``abi3`` ensures compatibility across a variety of python minor versions.
   - "Actually useful" means you can not only import pyeclib, but use it to
     perform encoding/decoding.
   - Where possible, we want to bundle in ISA-L support, too.

You might expect ``auditwheel repair`` to be able to do this for us. However,
that's primarily designed around dynamic *linking* -- and while that's
necessary, it's not sufficient; liberasurecode makes extensive use of dynamic
*loading* as well, and so the dynamically loaded modules need to be included.
"""

import argparse
import base64
import errno
import functools
import hashlib
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile


ENV_KEY = ('DYLD_LIBRARY_PATH' if sys.platform == 'darwin'
           else 'LD_LIBRARY_PATH')
if ENV_KEY in os.environ:
    os.environ[ENV_KEY] += ':/usr/lib:/usr/local/lib'
else:
    os.environ[ENV_KEY] = '/usr/lib:/usr/local/lib'


def locate_library(name, missing_ok=False):
    """
    Find a library.

    :param name: The name of the library to find, not including the
                 leading "lib".
    :param missing_ok: If true, return ``None`` when the library cannot be
                       located.
    :raises RuntimeError: If the library cannot be found and ``missing_ok``
                          is ``False``.
    :returns: The full path to the library.
    """
    expr = r'[^\(\)\s]*lib%s\.[^\(\)\s]*' % re.escape(name)
    cmd = ['ld', '-t']
    if sys.platform == 'darwin':
        cmd.extend(['-arch', platform.machine()])
    libpath = os.environ.get(ENV_KEY)
    if libpath:
        for d in libpath.split(':'):
            cmd.extend(['-L', d.rstrip('/')])
    cmd.extend(['-o', os.devnull, '-l%s' % name])
    p = subprocess.run(cmd, capture_output=True, text=True)
    # Note that we don't expect the command to exit cleanly; we just want
    # to see what got loaded
    res = re.search(expr, p.stdout)
    if res:
        return os.path.realpath(res.group(0))

    if missing_ok:
        return None

    print(f'{" ".join(cmd)!r} failed with status {p.returncode}:')
    print(f'\x1b[31m{p.stderr}\x1b[0m', end='')
    raise RuntimeError(f'Failed to locate {name}')


def build_wheel(src_dir):
    """
    Build the base wheel, returning the path to the wheel.

    Caller is responsible for cleaning up the tempdir.
    """
    tmp = tempfile.mkdtemp()
    try:
        subprocess.check_call([
            sys.executable, 'setup.py',
            'bdist_wheel', '-d', tmp, '--py-limited-api=cp310',
        ], cwd=src_dir)
        files = os.listdir(tmp)
        assert len(files) == 1, files
        return os.path.join(tmp, files[0])
    except Exception:
        shutil.rmtree(tmp)
        raise


def repack_wheel(whl, so_suffix, out_whl=None, require_isal=False):
    """
    Repack a wheel to bundle in liberasurecode libraries.

    This unpacks the wheel, copies all the supporting libraries to the
    unpacked wheel, adjusts rpath etc for the libraries, rebuilds the
    dist-info to include the libraries, and rebuilds the wheel.
    """
    if out_whl is None:
        out_whl = whl

    tmp = tempfile.mkdtemp()
    try:
        # unpack wheel
        zf = zipfile.ZipFile(whl, 'r')
        zf.extractall(tmp)

        relocate_libs(tmp, so_suffix, require_isal)
        rebuild_dist_info_record(tmp)
        build_zip(tmp, out_whl)
    finally:
        shutil.rmtree(tmp)


def relocate_libs(tmp, so_suffix, require_isal=False):
    """
    Bundle libraries into a unpacked-wheel tree.

    :param tmp: the temp dir containing the tree for the unzipped wheel
    :param so_suffix: the LIBERASURECODE_SO_SUFFIX used to build
                      liberasurecode; this should be used to avoid
                      interfering with system libraries
    """
    lib_dir = 'pyeclib.libs'
    all_libs = [os.path.join(tmp, lib)
                for lib in os.listdir(tmp) if '.so' in lib]
    for lib in all_libs:  # NB: pypy builds may create multiple .so's
        update_rpath(lib, '/' + lib_dir)

    inject = functools.partial(
        inject_lib,
        tmp,
        so_suffix=so_suffix,
        lib_dir=lib_dir,
        all_libs=all_libs,
    )

    # Be sure to move liberasurecode first, so we can fix its links to
    # the others
    relocated_libec = inject(locate_library('erasurecode'))
    # Since liberasurecode links against other included libraries,
    # need to update rpath
    update_rpath(relocated_libec)

    # These guys all stand on their own, so don't need the rpath update
    inject(locate_library('nullcode'))
    inject(locate_library('Xorcode'))
    builtin_rs_vand = inject(locate_library('erasurecode_rs_vand'))
    maybe_add_needed(relocated_libec, os.path.basename(builtin_rs_vand))

    # Nobody actually links against this, but we want it anyway if available
    isal = locate_library('isal', missing_ok=not require_isal)
    if isal:
        relocated_isal = inject(isal)
        maybe_add_needed(relocated_libec, os.path.basename(relocated_isal))


def update_rpath(lib, rpath_suffix=''):
    if sys.platform == 'darwin':
        subprocess.check_call([
            'install_name_tool',
            '-add_rpath', '@loader_path' + rpath_suffix,
            lib,
        ])
    else:
        subprocess.check_call([
            'patchelf', '--set-rpath', '$ORIGIN' + rpath_suffix, lib])


def maybe_add_needed(lib, needed):
    if sys.platform == 'linux' and platform.libc_ver()[0] != 'glibc':
        # We need to do this for musl; it doesn't seem to respect RUNPATH
        # for dynamic loading, just dynamic linking
        subprocess.check_call(['patchelf', '--add-needed', needed, lib])


def inject_lib(
    whl_dir,
    src_lib,
    so_suffix='-pyeclib',
    lib_dir='pyeclib.libs',
    all_libs=None,
):
    try:
        os.mkdir(os.path.join(whl_dir, lib_dir))
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise

    if sys.platform == 'darwin':
        old_lib = src_lib
        name = os.path.basename(old_lib).split('.', 1)[0]
        new_lib = name + so_suffix + '.dylib'
    else:
        name, _, version = os.path.basename(src_lib).partition('.so')
        major = '.'.join(version.split('.', 2)[:2])
        old_lib = name + '.so' + major
        new_lib = name + so_suffix + '.so' + major

    print('Injecting ' + new_lib)
    relocated = os.path.join(whl_dir, lib_dir, new_lib)
    shutil.copy2(src_lib, relocated)
    if sys.platform == 'darwin':
        subprocess.check_call([
            'install_name_tool', '-id', new_lib, relocated])
    else:
        subprocess.check_call(['patchelf', '--set-soname', new_lib, relocated])

    if all_libs:
        # Fix linkage in the libs already moved -- this is mainly an issue for
        # liberasurecode.so. Jerasure *would* need it for GF-Complete, but it
        # seems unlikely we'd be able to include those any time soon
        if sys.platform == 'darwin':
            for lib in all_libs:
                subprocess.check_call([
                    'install_name_tool',
                    '-change', old_lib,
                    '@rpath/' + new_lib,
                    lib,
                ])
        else:
            subprocess.check_call([
                'patchelf', '--replace-needed', old_lib, new_lib] + all_libs)
        all_libs.append(relocated)

    return relocated


def rebuild_dist_info_record(tmp):
    """
    Update the dist-info RECORD information.

    There are likely new files, and pre-existing files may have changed;
    rebuild the whole thing.

    See https://packaging.python.org/en/latest/specifications/
    recording-installed-packages/#the-record-file for more info.
    """
    tmp = tmp.rstrip('/') + '/'
    dist_info_dir = [d for d in os.listdir(tmp) if d.endswith('.dist-info')]
    assert len(dist_info_dir) == 1, dist_info_dir
    record_file = os.path.join(tmp, dist_info_dir[0], 'RECORD')
    with open(record_file, 'w') as fp:
        for dir_path, _, files in os.walk(tmp):
            for file in files:
                file = os.path.join(dir_path, file)
                if file == record_file:
                    fp.write(file[len(tmp):] + ',,\n')
                    continue
                hsh, sz = sha256(file)
                fp.write('%s,sha256=%s,%d\n' % (file[len(tmp):], hsh, sz))


def sha256(file):
    hasher = hashlib.sha256()
    sz = 0
    with open(file, 'rb') as fp:
        for chunk in iter(lambda: fp.read(128 * 1024), b''):
            hasher.update(chunk)
            sz += len(chunk)
    hsh = base64.urlsafe_b64encode(hasher.digest())
    return hsh.decode('ascii').strip('='), sz


def build_zip(tmp, out_file):
    """
    Zip up all files in a tree, with archive names relative to the root.
    """
    tmp = tmp.rstrip('/') + '/'
    with zipfile.ZipFile(out_file, 'w') as zf:
        for dir_path, _, files in os.walk(tmp):
            for file in files:
                file = os.path.join(dir_path, file)
                zf.write(file, file[len(tmp):])


def repair_wheel(whl):
    """
    Run ``auditwheel repair`` to ensure appropriate platform tags.
    """
    if sys.platform == 'darwin':
        return whl  # auditwheel only works on linux
    whl_dir = os.path.dirname(whl)
    subprocess.check_call(['auditwheel', 'repair', whl, '-w', whl_dir])
    wheels = [f for f in os.listdir(whl_dir) if f != os.path.basename(whl)]
    assert len(wheels) == 1, wheels
    return os.path.join(whl_dir, wheels[0])


def fix_ownership(whl):
    uid = int(os.environ.get('UID', '1000'))
    gid = int(os.environ.get('GID', uid))
    os.chown(whl, uid, gid)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('src_dir')
    parser.add_argument('-w', '--wheel-dir', default='.')
    parser.add_argument('-s', '--so-suffix', default='-pyeclib')
    parser.add_argument('-r', '--repair', action='store_true')
    parser.add_argument('--require-isal', action='store_true')
    args = parser.parse_args()
    whl = build_wheel(args.src_dir)
    whl_dir = os.path.dirname(whl)
    try:
        repack_wheel(whl, args.so_suffix, require_isal=args.require_isal)
        if args.repair:
            whl = repair_wheel(whl)
        output_whl = os.path.join(
            args.wheel_dir, os.path.basename(whl))
        shutil.move(whl, output_whl)
        if os.geteuid() == 0:
            # high likelihood of running in a docker container or something
            fix_ownership(output_whl)
    finally:
        shutil.rmtree(whl_dir)


if __name__ == '__main__':
    main()

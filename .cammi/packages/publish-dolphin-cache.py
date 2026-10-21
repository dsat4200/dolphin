"""Publish ordinary signed Nix narinfos/NARs through GitHub release assets.

No custom downloader or unchecked executable tarballs: Nix verifies signatures
and hashes. Upstream-cache paths are left to cache.nixos.org, avoiding GitHub's
per-release asset limit. Only cache metadata URLs are flattened; URLs are not
part of Nix's signed store-path fingerprint.
"""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

REPO = 'dsat4200/dolphin'
TAG = 'nix-dolphin-cache'


def flatten_info(text):
    lines = text.splitlines()
    url = next(line[5:] for line in lines if line.startswith('URL: '))
    if Path(url).is_absolute() or '..' in Path(url).parts:
        raise ValueError('Unexpected generated NAR URL')
    name = Path(url).name
    return '\n'.join('URL: ' + name if line.startswith('URL: ') else line for line in lines) + '\n', url


def upstream_cached(text):
    return any(line.startswith('Sig: cache.nixos.org-1:') for line in text.splitlines())


def run(*args, **kwargs):
    return subprocess.run(args, check=True, **kwargs)


def resolve_outputs(paths):
    # Bare result symlink names are parsed as flake registry IDs by Nix.
    return [str(Path(path).resolve(strict=True)) for path in paths]


def main(paths):
    if not paths:
        raise ValueError('Pass a built Dolphin output path or result symlink')
    paths = resolve_outputs(paths)
    secret = os.environ.get('NIX_CACHE_SIGNING_KEY')
    if not secret:
        raise ValueError('NIX_CACHE_SIGNING_KEY is required; do not commit it')
    with tempfile.TemporaryDirectory(prefix='dolphin-cache-') as temporary:
        root = Path(temporary)
        key = root / 'secret'
        with os.fdopen(os.open(key, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600), 'w') as output:
            output.write(secret.strip() + '\n')
        # Do not propagate the secret value into signing/copy/upload subprocesses.
        os.environ.pop('NIX_CACHE_SIGNING_KEY', None)
        run('nix', 'store', 'sign', '--recursive', '--key-file', str(key), *paths)
        cache = root / 'cache'
        run('nix', 'copy', '--to', cache.as_uri() + '?compression=xz', *paths)
        upload = root / 'upload'
        upload.mkdir()
        metadata, nars = [], []
        for info in cache.glob('*.narinfo'):
            text = info.read_text()
            if upstream_cached(text):
                continue
            if not any(line.startswith('Sig: dsat4200-dolphin-1:') for line in text.splitlines()):
                raise ValueError('Missing expected cache signature')
            normalized, url = flatten_info(text)
            nar = cache / url
            flat = upload / nar.name
            if not flat.exists():
                os.link(nar, flat)
                nars.append(str(flat))
            target = upload / info.name
            target.write_text(normalized)
            metadata.append(str(target))
        if not metadata:
            raise ValueError('No downstream paths to publish')
        found = subprocess.run(['gh', 'release', 'view', TAG, '--repo', REPO], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if found.returncode:
            run('gh', 'release', 'create', TAG, '--repo', REPO, '--target', 'cammi-tabs',
                '--title', 'Signed Dolphin Nix binary cache',
                '--notes', 'Machine-readable Nix cache. Verify with the public key in modules/common.nix. Source: packages/dolphin.nix and dsat4200/dolphin cammi-tabs.')
        existing = json.loads(subprocess.check_output(['gh', 'release', 'view', TAG, '--repo', REPO, '--json', 'assets']))
        names = {asset['name'] for asset in existing['assets']}
        new_names = {Path(p).name for p in nars + metadata} | {'nix-cache-info'}
        if len(names | new_names) > 1000:
            raise ValueError('Release asset limit reached; rotate the cache release before publishing')
        def push(files):
            for start in range(0, len(files), 20):
                run('gh', 'release', 'upload', TAG, '--repo', REPO, '--clobber', *files[start:start + 20])
        # NARs first, then narinfos, then discovery: never advertise an object
        # before its content is uploaded. Existing immutable NAR assets are reused.
        push([p for p in nars if Path(p).name not in names])
        push(metadata)
        discovery = upload / 'nix-cache-info'
        discovery.write_text('StoreDir: /nix/store\nWantMassQuery: 1\nPriority: 45\n')
        push([str(discovery)])
        print(f'Published {len(metadata)} downstream store paths to https://github.com/{REPO}/releases/download/{TAG}')


if __name__ == '__main__':
    main(sys.argv[1:])

import importlib.util
import os
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('publisher', Path(__file__).resolve().parents[1] / 'packages/publish-dolphin-cache.py')
publisher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(publisher)


class CacheMetadataTests(unittest.TestCase):
    def test_relative_result_symlink_is_resolved_before_nix(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'output'
            output.mkdir()
            link = Path(directory) / 'result-dolphin'
            link.symlink_to(output, target_is_directory=True)
            self.assertEqual(publisher.resolve_outputs([os.path.relpath(link)]), [str(output.resolve())])

    def test_missing_output_fails_before_signing(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(FileNotFoundError):
                publisher.resolve_outputs([str(Path(directory) / 'missing')])

    def test_flat_url_preserves_signed_metadata(self):
        source = 'StorePath: /nix/store/example\nURL: nar/hash.nar.xz\nNarHash: sha256:abc\nSig: dsat4200-dolphin-1:signature\n'
        result, url = publisher.flatten_info(source)
        self.assertEqual(url, 'nar/hash.nar.xz')
        self.assertEqual(result, source.replace('URL: nar/', 'URL: '))

    def test_rejects_unexpected_paths(self):
        for path in ('/absolute', '../escape', 'nar/../../escape'):
            with self.assertRaises(ValueError):
                publisher.flatten_info('URL: ' + path)

    def test_only_upstream_signatures_skip_publication(self):
        self.assertTrue(publisher.upstream_cached('Sig: cache.nixos.org-1:abc\nSig: dsat4200-dolphin-1:xyz\n'))
        self.assertFalse(publisher.upstream_cached('Sig: dsat4200-dolphin-1:xyz\n'))


if __name__ == '__main__':
    unittest.main()

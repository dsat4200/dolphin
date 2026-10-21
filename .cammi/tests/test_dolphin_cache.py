import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('publisher', Path(__file__).resolve().parents[1] / 'packages/publish-dolphin-cache.py')
publisher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(publisher)


class CacheMetadataTests(unittest.TestCase):
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

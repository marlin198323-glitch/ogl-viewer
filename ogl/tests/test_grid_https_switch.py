"""Offline tests for the secure Stadt NexVerse login migration gate."""
import importlib.util
from pathlib import Path
from unittest import TestCase
from unittest.mock import patch
import urllib.error

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/activate-grid-https.py"
spec = importlib.util.spec_from_file_location("ogl_grid_https", SCRIPT)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class Response:
    status = 200

    def __init__(self, login):
        self.login = login

    def geturl(self):
        return module.GRIDINFO_URL

    def read(self, count):
        return (
            f"<gridinfo><gridname>Stadt NexVerse</gridname>"
            f"<login>{self.login}</login></gridinfo>"
        ).encode()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        return False


class GridHTTPSMigrationTests(TestCase):
    def test_secure_verified_grid_can_pass(self):
        with patch.object(module.urllib.request, "urlopen",
                          return_value=Response(module.SECURE_LOGIN)):
            self.assertEqual(module.verify_grid(), module.SECURE_LOGIN)

    def test_insecure_grid_info_is_rejected(self):
        with patch.object(module.urllib.request, "urlopen",
                          return_value=Response("http://hg.stadt-nexverse.de:80/")):
            with self.assertRaisesRegex(ValueError, "not the verified secure"):
                module.verify_grid()

    def test_tls_failure_rejects_activation(self):
        with patch.object(module.urllib.request, "urlopen",
                          side_effect=urllib.error.URLError("connection refused")):
            with self.assertRaises(urllib.error.URLError):
                module.verify_grid()

    def test_existing_source_stays_http_until_verified(self):
        source = module.HEADER.read_text()
        self.assertIn(module.OLD_DECLARATION, source)
        self.assertNotIn(module.NEW_DECLARATION, source)

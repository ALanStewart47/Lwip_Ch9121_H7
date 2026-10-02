import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPT_DIR))

from publish_release import read_c_source, strip_comments_preserving_header, write_c_source  # noqa: E402


class SourceEncodingTests(unittest.TestCase):
    def test_preserves_gbk_source_encoding(self):
        source = "/* 文件说明 */\nint value = 1; // 删除\n"
        with tempfile.TemporaryDirectory() as temporary_directory:
            path = Path(temporary_directory) / "example.c"
            path.write_bytes(source.encode("gbk"))

            text, encoding = read_c_source(path)
            write_c_source(path, strip_comments_preserving_header(text), encoding)

            self.assertEqual(encoding, "gbk")
            self.assertIn("文件说明", path.read_bytes().decode("gbk"))
            self.assertNotIn("删除", path.read_bytes().decode("gbk"))


if __name__ == "__main__":
    unittest.main()

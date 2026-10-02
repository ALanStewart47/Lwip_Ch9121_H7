import sys
import unittest
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPT_DIR))

from publish_release import PublishError, reject_unsafe_comment_splices  # noqa: E402


class SplicedCommentTests(unittest.TestCase):
    def test_rejects_comment_opening_delimiter_split_across_lines(self):
        with self.assertRaises(PublishError):
            reject_unsafe_comment_splices("int value = 1; /\\\n* hidden */")

    def test_rejects_comment_closing_delimiter_split_across_lines(self):
        with self.assertRaises(PublishError):
            reject_unsafe_comment_splices("/* hidden *\\\n/ int value = 1;")

    def test_allows_a_normal_multiline_macro(self):
        reject_unsafe_comment_splices("#define ADD(a, b) \\\n+    ((a) + (b))\n")


if __name__ == "__main__":
    unittest.main()

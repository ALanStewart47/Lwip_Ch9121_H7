import sys
import unittest
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPT_DIR))

from publish_release import (  # noqa: E402
    PublishError,
    has_disallowed_comments,
    is_excluded_path,
    is_processed_source,
    require_zero_error_keil_build,
    strip_comments_preserving_header,
    token_sequence,
    uv4_build_command,
)


class CommentStrippingTests(unittest.TestCase):
    def test_preserves_file_header_and_removes_later_comments(self):
        source = """/* Copyright CST */
/* File description */
#include \"main.h\" // include comment
int/**/value = 1; /* implementation note */
"""

        result = strip_comments_preserving_header(source)

        self.assertTrue(result.startswith("/* Copyright CST */\n/* File description */"))
        self.assertNotIn("include comment", result)
        self.assertNotIn("implementation note", result)
        self.assertIn("int value = 1;", result)
        self.assertEqual(token_sequence(source), token_sequence(result))
        self.assertFalse(has_disallowed_comments(result))

    def test_does_not_treat_comment_markers_in_literals_or_macros_as_comments(self):
        source = """#define URL \"http://device/*path*/\"
const char slash = '/';
const char *message = \"// not a comment\"; // remove me
const char quote = '\\\\'; /* remove me too */
"""

        result = strip_comments_preserving_header(source)

        self.assertIn('#define URL "http://device/*path*/"', result)
        self.assertIn('"// not a comment"', result)
        self.assertIn("const char slash = '/';", result)
        self.assertIn("const char quote = '\\\\';", result)
        self.assertNotIn("remove me", result)
        self.assertEqual(token_sequence(source), token_sequence(result))

    def test_detects_code_token_changes(self):
        original = "int value = 1;"
        changed = "int value = 2;"

        self.assertNotEqual(token_sequence(original), token_sequence(changed))

    def test_strips_line_splice_inside_block_comment_body(self):
        source = """int keep = 1;
/* commented \\
   if (udp_recbuf[0] == 'C' && udp_recbuf[1] == 'H') */
int after = 2;
"""

        result = strip_comments_preserving_header(source)

        self.assertNotIn("commented", result)
        self.assertNotIn("udp_recbuf", result)
        self.assertIn("int keep = 1;", result)
        self.assertIn("int after = 2;", result)
        self.assertEqual(token_sequence(source), token_sequence(result))
        self.assertFalse(has_disallowed_comments(result))


class CopyScopeTests(unittest.TestCase):
    def test_excludes_internal_knowledge_and_keeps_build_inputs(self):
        self.assertTrue(is_excluded_path(Path("_Doc/findings.md")))
        self.assertTrue(is_excluded_path(Path("release-publish-skill/SKILL.md")))
        self.assertTrue(is_excluded_path(Path("README.md")))
        self.assertTrue(is_excluded_path(Path("协议代码文档 -2025-11-11（模块化新增）/protocol.md")))
        self.assertFalse(is_excluded_path(Path("App/src/app.c")))
        self.assertFalse(is_excluded_path(Path("Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c")))
        self.assertFalse(is_excluded_path(Path("MDK-ARM/STM32G431.uvprojx")))

    def test_strips_handwritten_code_but_not_cubemx_core(self):
        self.assertTrue(is_processed_source(Path("App/src/app.c")))
        self.assertTrue(is_processed_source(Path("Bsp/src/bsp_timer.c")))
        self.assertTrue(is_processed_source(Path("Protocol/protocol_public.c")))
        self.assertFalse(is_processed_source(Path("Core/Src/main.c")))
        self.assertFalse(is_processed_source(Path("Core/Inc/gpio.h")))
        self.assertFalse(is_processed_source(Path("Core/Src/stm32g4xx_it.c")))
        self.assertFalse(is_processed_source(Path("Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c")))


class KeilBuildGateTests(unittest.TestCase):
    def test_accepts_zero_errors_with_warnings_and_exit_code_1(self):
        log = '"STM32G431\\STM32G431_.axf" - 0 Error(s), 5 Warning(s).\n'
        errors, warnings = require_zero_error_keil_build(1, log)
        self.assertEqual((errors, warnings), (0, 5))

    def test_accepts_clean_build_exit_code_0(self):
        log = '"STM32G431\\STM32G431_.axf" - 0 Error(s), 0 Warning(s).\n'
        errors, warnings = require_zero_error_keil_build(0, log)
        self.assertEqual((errors, warnings), (0, 0))

    def test_rejects_nonzero_errors_in_uv4_log(self):
        log = '"STM32G431\\STM32G431_.axf" - 2 Error(s), 1 Warning(s).\n'
        with self.assertRaises(PublishError):
            require_zero_error_keil_build(2, log)

    def test_rejects_log_without_error_summary(self):
        with self.assertRaises(PublishError):
            require_zero_error_keil_build(1, "UV4 started\n")

    def test_rejects_fatal_uv4_exit_code(self):
        log = '"STM32G431\\STM32G431_.axf" - 0 Error(s), 0 Warning(s).\n'
        with self.assertRaises(PublishError):
            require_zero_error_keil_build(3, log)

    def test_uses_last_error_summary_from_log(self):
        log = (
            'compiling app.c... 1 Error(s), 0 Warning(s).\n'
            '"STM32G431\\STM32G431_.axf" - 0 Error(s), 4 Warning(s).\n'
        )
        errors, warnings = require_zero_error_keil_build(1, log)
        self.assertEqual((errors, warnings), (0, 4))

    def test_uv4_command_writes_log_file(self):
        command = uv4_build_command(
            r"D:\Keil_v5\UV4\UV4.exe",
            Path(r"D:\proj\MDK-ARM\STM32G431.uvprojx"),
            "STM32G431",
            Path(r"D:\proj\MDK-ARM\release_publish_uv4.log"),
        )
        self.assertIn("-o", command)
        self.assertIn(r"D:\proj\MDK-ARM\release_publish_uv4.log", command)
        self.assertIn("-j0", command)


if __name__ == "__main__":
    unittest.main()

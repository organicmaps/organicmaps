import contextlib
import io
from pathlib import Path
import tempfile
import unittest

from sort_categories import parse_and_sort_categories


class SortCategoriesTest(unittest.TestCase):
    def sort(self, content):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "categories.txt"
            output = Path(directory) / "sorted.txt"
            source.write_text(content, encoding="utf-8")
            with contextlib.redirect_stdout(io.StringIO()):
                parse_and_sort_categories(str(source), str(output))
            return output.read_text(encoding="utf-8")

    def test_colon_type_headers(self):
        headers = (
            "area:highway-footway|place-square",
            "piste:type-downhill",
            "xmas:feature-market",
        )
        for header in headers:
            with self.subTest(header=header):
                source = f"{header}\nda:Plads\nen:Square\ncs:Náměstí\ncy:Sgwâr\n\n"
                expected = f"{header}\nen:Square\ncs:Náměstí\ncy:Sgwâr\nda:Plads\n\n"
                self.assertEqual(self.sort(source), expected)

    def test_comments_follow_translations_and_blank_lines_keep_their_positions(self):
        source = (
            "# File comment\n\n@category_food\n"
            "cy:Nwyddau groser\n# Translation comment\nen:Groceries\nca:Queviures\n\n\n"
            "shop-supermarket|@category_food\npt-BR:Mercado\nen:Supermarket\nar:سوبرماركت\n\n"
        )
        expected = (
            "# File comment\n\n@category_food\n"
            "# Translation comment\nen:Groceries\nca:Queviures\ncy:Nwyddau groser\n\n\n"
            "shop-supermarket|@category_food\nen:Supermarket\nar:سوبرماركت\npt-BR:Mercado\n\n"
        )
        result = self.sort(source)
        self.assertEqual(result, expected)
        self.assertEqual(self.sort(result), result)

    def test_consecutive_and_indented_comments_follow_translations(self):
        source = (
            "# Category comment\n@category_food\n"
            "# Welsh translation\n  # More Welsh context\ncy:Nwyddau groser\n"
            "en:Groceries\n# Catalan translation\nca:Queviures\n\n"
        )
        expected = (
            "# Category comment\n@category_food\n"
            "en:Groceries\n# Catalan translation\nca:Queviures\n"
            "# Welsh translation\n  # More Welsh context\ncy:Nwyddau groser\n\n"
        )
        result = self.sort(source)
        self.assertEqual(result, expected)
        self.assertEqual(self.sort(result), result)

    def test_trailing_comments_stay_at_the_end_of_the_block(self):
        for terminator in ("\n", "\n\n"):
            with self.subTest(terminator=terminator):
                source = (
                    "@category_food\ncy:Nwyddau groser\nen:Groceries\n"
                    "# Trailing comment\n  # More trailing context" + terminator
                )
                expected = (
                    "@category_food\nen:Groceries\ncy:Nwyddau groser\n"
                    "# Trailing comment\n  # More trailing context" + terminator
                )
                self.assertEqual(self.sort(source), expected)

    def test_comment_only_and_header_only_blocks_are_unchanged(self):
        source = (
            "# File comment\n \t\n# Category comment\nnatural-water|@waterbody\n"
            "# Trailing comment\n\n@shop\n\n# Final comment\n"
        )
        self.assertEqual(self.sort(source), source)

    def test_comments_follow_translations_without_english(self):
        source = "@shop\n# Welsh translation\ncy:Siop\n# Arabic translation\nar:متجر\n\n"
        expected = "@shop\n# Arabic translation\nar:متجر\n# Welsh translation\ncy:Siop\n\n"
        self.assertEqual(self.sort(source), expected)

    def test_final_block_without_blank_terminator(self):
        self.assertEqual(
            self.sort("shop-supermarket\ncy:Archfarchnad\nen:Supermarket\n"),
            "shop-supermarket\nen:Supermarket\ncy:Archfarchnad\n",
        )


if __name__ == "__main__":
    unittest.main()

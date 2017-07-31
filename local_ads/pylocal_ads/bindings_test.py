import unittest

from pylocal_ads import (Campaign, serialize, deserialize)


class PyLocalAdsTest(unittest.TestCase):

    def test_smoke(self):
        campaigns = [
            Campaign(10, 10, 10, 10, 0),
            Campaign(1000, 100, 20, 17, 7),
            Campaign(120003, 456, 15, 13, 6)
        ]

        serialized = serialize(campaigns)
        result = deserialize(serialized)

        self.assertEqual(campaigns.sort(), result.sort())


if __name__ == "__main__":
    unittest.main()

import unittest

from pylocal_ads import (Campaign, Version, serialize, deserialize)


class PyLocalAdsTest(unittest.TestCase):

    def assert_equal_campaigns(self, lhs, rhs):
        self.assertEqual(len(lhs), len(rhs))

        for i in range(0, len(lhs)):
            self.assertEqual(lhs[i].m_featureId, rhs[i].m_featureId)
            self.assertEqual(lhs[i].m_iconId, rhs[i].m_iconId)
            self.assertEqual(lhs[i].m_daysBeforeExpired, rhs[i].m_daysBeforeExpired)
            self.assertEqual(lhs[i].m_minZoomLevel, rhs[i].m_minZoomLevel)
            self.assertEqual(lhs[i].m_priority, rhs[i].m_priority)

    def test_smoke(self):
        campaigns_v1 = [
            Campaign(10, 10, 10),
            Campaign(1000, 100, 20),
            Campaign(120003, 456, 15)
        ]

        campaigns_v2 = [
            Campaign(10, 10, 10, 10, 0),
            Campaign(1000, 100, 20, 17, 7),
            Campaign(120003, 456, 15, 13, 6)
        ]

        serialized = serialize(campaigns_v1, Version.V1)
        result_v1 = deserialize(serialized)

        self.assert_equal_campaigns(campaigns_v1, result_v1)

        serialized = serialize(campaigns_v2, Version.V2)
        result_v2 = deserialize(serialized)

        self.assert_equal_campaigns(campaigns_v2, result_v2)

        serialized = serialize(campaigns_v2, Version.LATEST)
        result_latest = deserialize(serialized)

        self.assert_equal_campaigns(campaigns_v2, result_latest)
        self.assert_equal_campaigns(result_v2, result_latest)


if __name__ == "__main__":
    unittest.main()

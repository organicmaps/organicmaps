from pylocal_ads import (Campaign, serialize, deserialize)


def smoke():
    campaigns = [
        Campaign(10, 10, 10, 10, 0),
        Campaign(1000, 100, 20, 17, 7),
        Campaign(120003, 456, 15, 13, 6)
    ]

    serialized = serialize(campaigns)
    result = deserialize(serialized)

    if campaigns.sort() == result.sort():
        return True

    return False


def main():
    if smoke():
        print "Smoke OK"
    else:
        print "Smoke FAIL"

if __name__ == "__main__":
    main()

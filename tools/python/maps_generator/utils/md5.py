import hashlib


def md5sum(name, block_size=4096):
    d = hashlib.md5()
    with open(name, mode="rb") as f:
        buf = f.read(block_size)
        while len(buf) > 0:
            d.update(buf)
            buf = f.read(block_size)
    return d.hexdigest()


def write_md5sum(fname, name):
    with open(name, mode="w") as f:
        md5 = md5sum(fname)
        f.write(md5)


def check_md5(fname, name):
    h = md5sum(fname)
    with open(name, "r") as f:
        data = f.read()
        assert len(data) != 0, f"The file {name} is empty"
        if data.split()[0] == h:
            return True
    return False


def md5_ext(name):
    return f"{name}.md5"

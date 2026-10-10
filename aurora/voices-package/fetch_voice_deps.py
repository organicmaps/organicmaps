import urllib.request, re, os, tarfile, io

BASE = "https://conan.omp.ru/artifactory/public/aurora"
OUT = "/tmp/opencode/voice/deps"
DEPS = [("abseil", "20240116.1"), ("cpuinfo", "cci.20231129"), ("date", "3.0.1"),
        ("flatbuffers", "23.5.26"), ("nsync", "1.26.0"), ("onnx", "1.16.0"),
        ("protobuf", "3.21.12"), ("pthreadpool", "cci.20231129"), ("re2", "20231101"),
        ("xnnpack", "cci.20240229")]


def get(url):
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    return urllib.request.urlopen(req, timeout=120).read()


def listing(path):
    try:
        html = get(f"{BASE}/{path}/").decode("utf-8", "ignore")
    except Exception as e:
        print("  listing fail", path, e)
        return []
    return [m for m in re.findall(r'href="([^"?]+)/"', html) if m != "../"]


def want(arch):
    return "a7" if arch == "armv7" else ("a8" if arch in ("armv8", "aarch64") else None)


# Settings must match onnxruntime's build profile exactly.
NEED = {"build_type": "Release", "compiler": "gcc", "compiler.version": "8",
        "compiler.cppstd": "gnu17", "compiler.libcxx": "libstdc++11", "os": "Linux"}


def settings_ok(ci):
    kv = dict(re.findall(r"^(\w[\w.]*)=(.+)$", ci, re.M))
    for k, v in NEED.items():
        if k in kv and kv[k] != v:
            return False
    opt = re.search(r"\[options\](.*?)(?=\n\[|\Z)", ci, re.S)
    if opt and "shared=" in opt.group(1):
        if not re.search(r"shared=True", opt.group(1)):
            return False
    return True


for name, ver in DEPS:
    print(f"### {name}/{ver}", flush=True)
    for u in listing(f"{name}/{ver}"):
        for rr in listing(f"{name}/{ver}/{u}"):
            base = f"{name}/{ver}/{u}/{rr}"
            for pid in listing(f"{base}/package"):
                for rev in listing(f"{base}/package/{pid}"):
                    p = f"{base}/package/{pid}/{rev}"
                    try:
                        ci = get(f"{BASE}/{p}/conaninfo.txt").decode("utf-8", "ignore")
                    except Exception as e:
                        print("   ci fail", p, e)
                        continue
                    m = re.search(r"arch=(\w+)", ci)
                    arch = m.group(1) if m else "?"
                    tgt = want(arch)
                    if not tgt or not settings_ok(ci):
                        continue
                    print(f"   {arch:8} {pid[:12]}/{rev[:8]}", flush=True)
                    try:
                        blob = get(f"{BASE}/{p}/conan_package.tgz")
                    except Exception as e:
                        print("   dl fail", e)
                        continue
                    dest = os.path.join(OUT, tgt)
                    os.makedirs(dest, exist_ok=True)
                    tf = tarfile.open(fileobj=io.BytesIO(blob), mode="r:gz")
                    n = 0
                    for mem in tf.getmembers():
                        if mem.name.startswith("lib/") and not mem.isdir():
                            fn = os.path.basename(mem.name)
                            if ".so" in fn:
                                f = tf.extractfile(mem)
                                with open(os.path.join(dest, fn), "wb") as o:
                                    o.write(f.read())
                                n += 1
                    print(f"   -> extracted {n} libs to {tgt}", flush=True)
print("DONE")

import argparse
import importlib.util
import sys

def _usage():
    print("pytrack_generator_tests.py \
     --module_path path/to/pytrack_generator.so \
     --data_path path/to/omim/data \
    --user_resource_path path/to/omim/data")

def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--module_path',
        type=str
    )
    parser.add_argument(
        '--data_path',
        type=str
    )

    parser.add_argument(
        '--user_resources_path',
        type=str,
    )

    args = parser.parse_args(sys.argv[1:])
    if not args.module_path or not args.data_path or not args.user_resources_path:
        _usage()
        sys.exit(2)

    spec = importlib.util.spec_from_file_location("pytrack_generator", args.module_path)
    ge = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(ge)

    params = ge.Params(args.data_path, args.user_resources_path)
    generator = ge.Generator(params)

    points = ge.LatLonList()
    points.append(ge.LatLon(55.796993, 37.537640))
    points.append(ge.LatLon(55.798087, 37.539002))

    result = generator.generate(points)
    assert len(result) > len(points)

    try:
        invalid_points = ge.LatLonList()
        invalid_points.append(ge.LatLon(20, 20))
        invalid_points.append(ge.LatLon(20, 20))
        generator.generate(invalid_points)
    except ge.RouteNotFoundException as ex:
        print(ex)
        return

    assert False

if __name__ == '__main__':
    _main()

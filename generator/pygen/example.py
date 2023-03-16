#!/usr/bin/env python
import argparse
import os
import timeit

from pygen import classif
from pygen import mwm


def example__storing_features_in_a_collection(path):
    ft_list = [ft for ft in mwm.Mwm(path)]
    print("List size:", len(ft_list))

    ft_tuple = tuple(ft for ft in mwm.Mwm(path))
    print("Tuple size:", len(ft_tuple))

    def slow():
        ft_with_metadata_list = []
        for ft in mwm.Mwm(path):
            if ft.metadata():
                ft_with_metadata_list.append(ft)
        return ft_with_metadata_list

    ft_with_metadata_list = slow()
    print("Features with metadata:", len(ft_with_metadata_list))
    print("First three are:", ft_with_metadata_list[:3])

    def fast():
        ft_with_metadata_list = []
        for ft in mwm.Mwm(path, False):
            if ft.metadata():
                ft_with_metadata_list.append(ft.parse())
        return ft_with_metadata_list

    tslow = timeit.timeit(slow, number=100)
    tfast = timeit.timeit(fast, number=100)
    print("Slow took {}, fast took {}.".format(tslow, tfast))


def example__features_generator(path):
    def make_gen(path):
        return (ft for ft in mwm.Mwm(path))

    cnt = 0
    print("Names of several first features:")
    for ft in make_gen(path):
        print(ft.names())
        if cnt == 5:
            break

        cnt += 1

    def return_ft(num):
        cnt = 0
        for ft in mwm.Mwm(path):
            if cnt == num:
                return ft

            cnt += 1

    print(return_ft(10))


def example__sequential_processing(path):
    long_names = []
    for ft in mwm.Mwm(path):
        if len(ft.readable_name()) > 100:
            long_names.append(ft.readable_name())

    print("Long names:", long_names)


def example__working_with_features(path):
    it = iter(mwm.Mwm(path))
    ft = it.next()
    print("Feature members are:", dir(ft))

    print("index:", ft.index())
    print(
        "types:",
        ft.types(),
        "redable types:",
        [classif.readable_type(t) for t in ft.types()],
    )
    print("metadata:", ft.metadata())
    print("names:", ft.names())
    print("readable_name:", ft.readable_name())
    print("rank:", ft.rank())
    print("population:", ft.population())
    print("road_number:", ft.road_number())
    print("house_number:", ft.house_number())
    print("layer:", ft.layer())
    print("geom_type:", ft.geom_type())
    print("center:", ft.center())
    print("geometry:", ft.geometry())
    print("limit_rect:", ft.limit_rect())
    print("__repr__:", ft)

    for ft in it:
        geometry = ft.geometry()
        if ft.geom_type() == mwm.GeomType.area and len(geometry) < 10:
            print("area geometry", geometry)
            break


def example__working_with_mwm(path):
    map = mwm.Mwm(path)
    print("Mwm members are:", dir(map))

    print("version:", map.version())
    print("type:", map.type())
    print("bounds:", map.bounds())
    print("sections_info:", map.sections_info())


def main(path):
    example__storing_features_in_a_collection(path)
    example__features_generator(path)
    example__sequential_processing(path)
    example__working_with_features(path)
    example__working_with_mwm(path)


if __name__ == "__main__":
    resource_path = os.path.join(
        os.path.dirname(os.path.realpath(__file__)), "..", "..", "data"
    )

    classif.init_classificator(resource_path)

    main(os.path.join(resource_path, "minsk-pass.mwm",))

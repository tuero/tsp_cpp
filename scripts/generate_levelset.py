# Source from: https://github.com/nathangrinsztajn/Box-World/

import argparse
import os
from multiprocessing import Manager, Pool

import numpy as np


def sample_from_set(s, rng):
    return list(s)[rng.choice(len(s))]


def sampling_pairs(num_pair, n, rng):
    possibilities = set(range(1, n * (n - 1)))
    keys = []
    locks = []
    for _ in range(num_pair):
        key = sample_from_set(possibilities, rng)
        key_x, key_y = key // (n - 1), key % (n - 1)
        lock_x, lock_y = key_x, key_y + 1
        to_remove = (
            [key_x * (n - 1) + key_y]
            + [key_x * (n - 1) + i + key_y for i in range(1, min(2, n - 2 - key_y) + 1)]
            + [key_x * (n - 1) - i + key_y for i in range(1, min(2, key_y) + 1)]
        )

        possibilities -= set(to_remove)
        keys.append([key_x, key_y])
        locks.append([lock_x, lock_y])
    agent_pos = [sample_from_set(possibilities, rng)]
    possibilities -= set(agent_pos)
    first_key = [sample_from_set(possibilities, rng)]

    agent_pos = np.array([agent_pos[0] // (n - 1), agent_pos[0] % (n - 1)])
    first_key = first_key[0] // (n - 1), first_key[0] % (n - 1)
    return keys, locks, first_key, agent_pos


kEmpty = 0
kAgent = 1
kWall = 2
kCityUnvisited = 3


def create_map(args):
    manager_dict, n, num_cities, add_walls, seed = args
    gen = np.random.default_rng(seed)

    blocked_indices = set()
    m = [kEmpty for _ in range(n * n)]

    # Check to add walls
    if add_walls:
        for i in range((n // 2) - 1):
            idx_top_left = i * n + i
            idx_top_right = (i + 1) * n - i - 1
            idx_bottom_left = (n - 1 - i) * n + i
            idx_bottom_right = (n - i) * n - i - 1
            indices = [idx_top_left, idx_top_right, idx_bottom_left, idx_bottom_right]
            for idx in indices:
                m[idx] = kWall
                blocked_indices.add(idx)

    # Sample N cities
    city_indices = gen.choice(
        [i for i in range(n * n) if i not in blocked_indices],
        size=num_cities,
        replace=False,
    )
    for idx in city_indices:
        m[idx] = kCityUnvisited
        blocked_indices.add(idx)

    # place an agent
    agent_idx = gen.choice([i for i in range(n * n) if i not in blocked_indices])
    m[agent_idx] = kAgent

    # Make map str
    output_str = f"{n}|{n}|"
    for item in m:
        output_str += "{:02d}|".format(item)
    manager_dict[seed] = output_str[:-1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--num_train",
        help="Number of maps in train set",
        required=False,
        type=int,
        default=10000,
    )
    parser.add_argument(
        "--num_test",
        help="Number of maps in test set",
        required=False,
        type=int,
        default=1000,
    )
    parser.add_argument(
        "--map_size",
        help="Size of map width/height",
        required=False,
        type=int,
        default=10,
    )
    parser.add_argument(
        "--num_cities",
        help="Number of cities",
        required=True,
        type=int,
    )
    parser.add_argument(
        "--add_walls",
        help="Flag to add walls",
        required=True,
        action="store_true",
    )
    parser.add_argument(
        "--export_path", help="Export path for file", required=True, type=str
    )
    args = parser.parse_args()

    manager = Manager()
    data = manager.dict()
    with Pool(32) as p:
        p.map(
            create_map,
            [
                (
                    data,
                    args.map_size,
                    args.num_cities,
                    args.add_walls,
                    i,
                )
                for i in range(args.num_train + args.num_test)
            ],
        )

    # Parse and save to file
    if not os.path.exists(args.export_path):
        os.makedirs(args.export_path)

    export_train_file = os.path.join(args.export_path, "train.txt")
    export_test_file = os.path.join(args.export_path, "test.txt")

    map_idx = 0
    with open(export_train_file, "w") as file:
        for _ in range(args.num_train):
            file.write(data[map_idx])
            map_idx += 1
            file.write("\n")

    with open(export_test_file, "w") as file:
        for _ in range(args.num_test):
            file.write(data[map_idx])
            map_idx += 1
            file.write("\n")


if __name__ == "__main__":
    main()

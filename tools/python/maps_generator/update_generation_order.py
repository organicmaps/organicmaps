import argparse
from multiprocessing.pool import ThreadPool
from typing import Tuple

from maps_generator.checks.logs import logs_reader


def get_args():
    parser = argparse.ArgumentParser(
        description="This script generates file with countries that are "
                    "ordered by time needed to generate them."
    )
    parser.add_argument(
        "--output", type=str, required=True, help="Path to output file.",
    )
    parser.add_argument(
        "--logs", type=str, required=True, help="Path to logs directory.",
    )
    return parser.parse_args()


def process_log(log: logs_reader.Log) -> Tuple[str, float]:
    stage_logs = logs_reader.split_into_stages(log)
    stage_logs = logs_reader.normalize_logs(stage_logs)
    d = sum(s.duration.total_seconds() for s in stage_logs if s.duration is not None)
    return log.name, d


def main():
    args = get_args()
    with ThreadPool() as pool:
        order = pool.map(
            process_log,
            (log for log in logs_reader.LogsReader(args.logs) if log.is_mwm_log),
        )

        order.sort(key=lambda v: v[1], reverse=True)
        with open(args.output, "w") as out:
            out.write("# Mwm name\tGeneration time\n")
            out.writelines("{}\t{}\n".format(*line) for line in order)


if __name__ == "__main__":
    main()

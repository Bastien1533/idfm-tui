#!/usr/bin/env python3
"""Build data/gtfs2/ = data/gtfs/ pruned to Metro (route_type=1) trips.

Keeps every file untouched except trips.txt and stop_times.txt:
  * trips.txt      -> only trips whose route_id is a route with route_type == 1
  * stop_times.txt -> only rows whose trip_id is in the pruned trip set

Pure stdlib, streams the big files row-by-row.
"""
import csv
import os
import sys
import shutil

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "data", "gtfs")
DST = os.path.join(HERE, "..", "data", "gtfs2")

METRO_ROUTE_TYPE = "1"

SRC_TRIPS = os.path.join(SRC, "trips.txt")
DST_TRIPS = os.path.join(DST, "trips.txt")
SRC_STOP_TIMES = os.path.join(SRC, "stop_times.txt")
DST_STOP_TIMES = os.path.join(DST, "stop_times.txt")

HEADERS = {
    "routes.txt": "route_id",
    "trips.txt": "trip_id",
    "stop_times.txt": "trip_id",
}


def col_index(header, name):
    for i, col in enumerate(header):
        if col == name:
            return i
    sys.exit(f"column {name!r} not found in header: {header}")


def metro_route_ids():
    path = os.path.join(SRC, "routes.txt")
    with open(path, newline="") as f:
        r = csv.reader(f)
        header = next(r)
        i_route = col_index(header, "route_id")
        i_type = col_index(header, "route_type")
        metro = {row[i_route] for row in r if row[i_type] == METRO_ROUTE_TYPE}
    return metro


def filter_trips(metro_routes):
    """Write pruned trips, return set of kept trip_ids."""
    keep = set()
    with open(SRC_TRIPS, newline="") as fin, open(DST_TRIPS, "w", newline="") as fout:
        r = csv.reader(fin)
        header = next(r)
        i_route = col_index(header, "route_id")
        i_trip = col_index(header, "trip_id")
        w = csv.writer(fout)
        w.writerow(header)
        for row in r:
            if row[i_route] in metro_routes:
                keep.add(row[i_trip])
                w.writerow(row)
    return keep


def filter_stop_times(keep):
    n = kept = 0
    with open(SRC_STOP_TIMES, newline="") as fin, open(DST_STOP_TIMES, "w", newline="") as fout:
        r = csv.reader(fin)
        header = next(r)
        i_trip = col_index(header, "trip_id")
        w = csv.writer(fout)
        w.writerow(header)
        for row in r:
            n += 1
            if row[i_trip] in keep:
                kept += 1
                w.writerow(row)
            if n % 2_000_000 == 0:
                print(f"  stop_times: {n:,} rows scanned, {kept:,} kept", file=sys.stderr)
    return n, kept


def copy_rest():
    for name in os.listdir(SRC):
        if name in HEADERS:
            continue
        shutil.copy2(os.path.join(SRC, name), os.path.join(DST, name))


def main():
    if os.path.isdir(DST):
        sys.exit(f"refusing to clobber existing dir: {DST}")
    os.makedirs(DST)

    print(f"metro route_ids from routes.txt ...", file=sys.stderr)
    metro_routes = metro_route_ids()
    print(f"  {len(metro_routes):,} metro routes", file=sys.stderr)

    print(f"filtering trips.txt ...", file=sys.stderr)
    keep = filter_trips(metro_routes)
    print(f"  {len(keep):,} metro trips", file=sys.stderr)

    print(f"filtering stop_times.txt (one pass) ...", file=sys.stderr)
    n, kept = filter_stop_times(keep)
    print(f"  {kept:,}/{n:,} stop_times kept", file=sys.stderr)

    print(f"copying remaining files ...", file=sys.stderr)
    copy_rest()

    print(f"done -> {DST}")


if __name__ == "__main__":
    main()

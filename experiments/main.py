#!/usr/bin/env python3
"""Decode GTFS route_id + direction_id into a station-sequence graph viz.

Usage:
    python experiments/main.py --routes
    python experiments/main.py <route_id> <direction_id> [--transitive] [--by-headsign]

--transitive   also render a transitively-reduced graph alongside the original
--by-headsign  dissect trips by trip_headsign, one graph per headsign, written
               under experiments/route_<route_id>/

Reads data/gtfs/{trips,stop_times,stops}.csv (pure stdlib).
"""
import argparse
import csv
import os
import re
import sys
from collections import defaultdict, OrderedDict

HERE = os.path.dirname(os.path.abspath(__file__))
GTFS = os.path.join(HERE, "..", "data", "gtfs")


def load_stops():
    stops = {}
    path = os.path.join(GTFS, "stops.csv")
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            stops[row["stop_id"]] = row
    return stops


def station_name(stop, stops):
    pid = (stop.get("parent_station") or "").strip()
    if pid:
        parent = stops.get(pid)
        if parent and parent.get("location_type") == "1" and parent.get("stop_name"):
            return parent["stop_name"]
    return stop["stop_name"]


def list_routes():
    routes = defaultdict(list)
    path = os.path.join(GTFS, "trips.csv")
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            key = (row["route_id"], row["direction_id"])
            if row["trip_headsign"] not in routes[key]:
                routes[key].append(row["trip_headsign"])
    for (route, direction), heads in sorted(routes.items()):
        print(f"{route}\tdirection={direction}\theadsigns={heads[:3]}")


def trips_for_route(route_id, direction_id):
    wanted = {}
    path = os.path.join(GTFS, "trips.csv")
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            if row["route_id"] == route_id and int(row["direction_id"]) == direction_id:
                wanted[row["trip_id"]] = row.get("trip_headsign") or ""
    return wanted


def stop_times_for(wanted):
    by_trip = defaultdict(dict)
    path = os.path.join(GTFS, "stop_times.csv")
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            tid = row["trip_id"]
            if tid in wanted:
                by_trip[tid][int(row["stop_sequence"])] = row["stop_id"]
    return by_trip


def build_graph(wanted, by_trip, stops):
    nodes = OrderedDict()
    edges = defaultdict(int)
    seqs = []

    def add_node(sid):
        if sid not in nodes:
            nodes[sid] = {"name": station_name(stops[sid], stops), "min_seq": 10**9}

    for tid, seqmap in by_trip.items():
        order = [seqmap[k] for k in sorted(seqmap)]
        seqs.append(order)
        for i, sid in enumerate(order):
            add_node(sid)
            nodes[sid]["min_seq"] = min(nodes[sid]["min_seq"], i)
            if i > 0:
                edges[(order[i - 1], sid)] += 1
    return nodes, edges, seqs


def transitive_reduction(edges):
    """Remove edge u->v when a path u -> ... -> v of length >= 2 exists (DAG)."""
    adj = defaultdict(list)
    for a, b in edges:
        adj[a].append(b)

    reach = {}

    def dfs(u):
        if u in reach:
            return reach[u]
        r = set()
        for v in adj[u]:
            r.add(v)
            r |= dfs(v)
        reach[u] = r
        return r

    reduced = {}
    for (a, b), w in edges.items():
        redundant = any(b in dfs(x) for x in adj[a] if x != b)
        if not redundant:
            reduced[(a, b)] = w
    return reduced


def render_svg(route_id, direction_id, headsigns, nodes, edges, out_path, suffix=""):
    dx, dy = 220, 70
    mx, my = 60, 60
    rank_groups = defaultdict(list)
    for sid, n in nodes.items():
        rank_groups[n["min_seq"]].append(sid)
    max_rank = max(rank_groups) if rank_groups else 0
    max_cols = max(len(g) for g in rank_groups.values()) if rank_groups else 0
    W = mx * 2 + max_rank * dx + 180
    H = my * 2 + max_cols * dy + 80

    pos = {}
    for rank, sids in rank_groups.items():
        n = len(sids)
        for i, sid in enumerate(sids):
            y = H / 2 + (i - (n - 1) / 2) * dy
            pos[sid] = (mx + rank * dx, y)

    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" font-family="Helvetica,Arial,sans-serif">']
    parts.append(f'<rect width="100%" height="100%" fill="#fafafa"/>')
    parts.append(f'<text x="{W/2}" y="28" text-anchor="middle" font-size="16" font-weight="bold">{route_id} · direction {direction_id}{suffix}</text>')
    heads = " / ".join(sorted(set(h for h in headsigns if h))[:3])
    if heads:
        parts.append(f'<text x="{W/2}" y="48" text-anchor="middle" font-size="12" fill="#666">{heads}</text>')

    for (a, b), w in edges.items():
        x1, y1 = pos[a]
        x2, y2 = pos[b]
        if y1 == y2:
            parts.append(f'<line x1="{x1+70}" y1="{y1}" x2="{x2-70}" y2="{y2}" stroke="#888" stroke-width="1.6"/>')
            mx_l = (x1 + 70 + x2 - 70) / 2
            parts.append(f'<line x1="{mx_l-6}" y1="{y2-6}" x2="{mx_l+6}" y2="{y2}" stroke="#888" stroke-width="1.6"/>')
        else:
            cx = (x1 + x2) / 2
            d = f'M {x1+70} {y1} Q {cx} {y1}, {cx} {(y1+y2)/2} T {x2-70} {y2}'
            parts.append(f'<path d="{d}" fill="none" stroke="#888" stroke-width="1.6"/>')

    for sid, (x, y) in pos.items():
        n = nodes[sid]
        tw = max(120, 9 * len(n["name"]) + 16)
        parts.append(
            f'<rect x="{x-tw/2}" y="{y-22}" width="{tw}" height="44" rx="8" fill="#fff" '
            f'stroke="#2b6cb0" stroke-width="1.5"/>'
        )
        parts.append(f'<text x="{x}" y="{y-2}" text-anchor="middle" font-size="12" fill="#1a202c">{n["name"]}</text>')
        parts.append(f'<text x="{x}" y="{y+12}" text-anchor="middle" font-size="9" fill="#999">{sid.replace("IDFM:","")}</text>')

    parts.append("</svg>")
    with open(out_path, "w") as f:
        f.write("\n".join(parts))
    return out_path


def write_graph(route_id, direction_id, headsigns, nodes, edges, out_stem, transitive):
    written = [render_svg(route_id, direction_id, headsigns, nodes, edges, f"{out_stem}.svg")]
    if transitive:
        reduced = transitive_reduction(edges)
        written.append(render_svg(route_id, direction_id, headsigns, nodes, reduced,
                                  f"{out_stem}_reduced.svg", suffix=" · transitive reduction"))
    return written


def print_chains(nodes, by_trip):
    seen = set()
    for tid, seqmap in sorted(by_trip.items()):
        order = [seqmap[k] for k in sorted(seqmap)]
        names = tuple(nodes[s]["name"] for s in order)
        if names in seen:
            continue
        seen.add(names)
        print("  ", " -> ".join(names))


def main():
    ap = argparse.ArgumentParser(description="GTFS route -> station graph")
    ap.add_argument("route_id", nargs="?")
    ap.add_argument("direction_id", nargs="?", type=int)
    ap.add_argument("--routes", action="store_true", help="list available route_id / direction pairs")
    ap.add_argument("--transitive", action="store_true", help="also emit transitively-reduced graph")
    ap.add_argument("--by-headsign", action="store_true", help="dissect trips by trip_headsign into a per-route folder")
    args = ap.parse_args()

    if args.routes:
        list_routes()
        return

    if args.route_id is None or args.direction_id is None:
        ap.error("provide route_id and direction_id (or use --routes)")

    print(f"loading stops...", file=sys.stderr)
    stops = load_stops()
    print(f"filtering trips for {args.route_id} dir={args.direction_id}...", file=sys.stderr)
    wanted = trips_for_route(args.route_id, args.direction_id)
    if not wanted:
        sys.exit(f"no trips for route_id={args.route_id} direction_id={args.direction_id}")
    print(f"{len(wanted)} trips, scanning stop_times (855MB, one pass)...", file=sys.stderr)
    by_trip = stop_times_for(wanted)

    safe = re.sub(r"[^A-Za-z0-9]+", "_", args.route_id)
    written = []

    if args.by_headsign:
        groups = defaultdict(dict)
        for tid, seqmap in by_trip.items():
            groups[wanted[tid] or "(no headsign)"][tid] = seqmap
        folder = os.path.join(HERE, f"route_{safe}")
        os.makedirs(folder, exist_ok=True)
        for head, trips in sorted(groups.items()):
            nodes, edges, _ = build_graph(wanted, trips, stops)
            head_safe = re.sub(r"[^A-Za-z0-9]+", "_", head).strip("_") or "none"
            stem = os.path.join(folder, f"dir{args.direction_id}_{head_safe}")
            written += write_graph(args.route_id, args.direction_id, [head], nodes, edges, stem, args.transitive)
            print(f"\nheadsign: {head}  ({len(trips)} trips, {len(nodes)} stations)")
            print_chains(nodes, trips)

    nodes, edges, _ = build_graph(wanted, by_trip, stops)
    heads = sorted({wanted[t] for t in by_trip} - {""})
    print(f"\nroute {args.route_id} direction {args.direction_id}  ({len(by_trip)} trips)")
    print_chains(nodes, by_trip)
    written += write_graph(args.route_id, args.direction_id, heads, nodes, edges,
                           os.path.join(HERE, f"route_{safe}_dir{args.direction_id}"), args.transitive)

    print(f"\n{len(nodes)} stations, {len(edges)} directed edges (original)")
    if args.transitive:
        print(f"{len(transitive_reduction(edges))} edges after transitive reduction")
    for w in written:
        print("  ->", w)


if __name__ == "__main__":
    main()

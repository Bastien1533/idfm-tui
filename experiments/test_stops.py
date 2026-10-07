import argparse
import csv
import sys
from collections import Counter


def validate_duplicate_stops(file_path: str) -> bool:
    stops_without_parent = []

    with open(file_path, mode="r", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f)

        if "stop_name" not in reader.fieldnames:
            print("Error: CSV file must contain a 'stop_name' column.", file=sys.stderr)
            sys.exit(2)

        for row in reader:
            parent = row.get("parent_station", "").strip()
            stop_name = row.get("stop_name", "").strip()

            # Check for stops with no parent station specified
            if not parent and stop_name:
                stops_without_parent.append(stop_name)

    counts = Counter(stops_without_parent)
    duplicates = {name: count for name, count in counts.items() if count > 1}

    if duplicates:
        print(f"FAILED: Found {len(duplicates)} duplicate stop name(s) among stops with no parent station:")
        for name, count in duplicates.items():
            print(f"  - '{name}': {count} occurrences")
        return False

    print("PASSED: No duplicate stop names found among stops without a parent station.")
    return True


def main():
    parser = argparse.ArgumentParser(
        description="Validate duplicate stop names for stops with no parent station in GTFS CSV data."
    )
    parser.add_argument("file_path", help="Path to the stops.txt or CSV file")
    args = parser.parse_args()

    success = validate_duplicate_stops(args.file_path)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
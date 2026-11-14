#!/usr/bin/env python3
import argparse
import csv
import os
import sys

REQUIRED_FEATURES = {"shapes"}

COUNTY_ALIASES = {
    ("kings county", "new york"): "New York City (Brooklyn)",
    ("queens county", "new york"): "New York City (Queens)",
    ("bronx county", "new york"): "New York City (Bronx)",
    ("new york county", "new york"): "New York City (Manhattan)",
    ("richmond county", "new york"): "New York City (Staten Island)",
    ("san diego county", "california"): "San Diego",
    ("los angeles county", "california"): "Los Angeles",
    ("cook county", "illinois"): "Chicago",
    ("miami-dade county", "florida"): "Miami",
}


def compute_importance_score(row: dict) -> int:
    features = {f.lower().strip() for f in parse_features(row.get("features"))}
    score = 0
    # Premium features indicate major multi-modal and rail networks
    if "transfers" in features:
        score += 40
    if "location types" in features:
        score += 30
    if "route colors" in features:
        score += 15
    if "headsigns" in features:
        score += 10
    if "feed information" in features:
        score += 5

    # Major transit networks & primary modes
    prov = (row.get("provider") or "").lower()
    name = (row.get("name") or "").lower()
    major_keywords = [
        "metro",
        "subway",
        "rer",
        "mta",
        "tfl",
        "ratp",
        "sncf",
        "idfm",
        "renfe",
        "cta",
        "bart",
        "mbta",
        "wmata",
        "vbb",
        "transit authority",
    ]
    if any(k in prov or k in name for k in major_keywords):
        score += 25

    # Geographic bounding box extent (larger network area = higher importance)
    try:
        min_lat = float(row.get("location.bounding_box.minimum_latitude") or 0)
        max_lat = float(row.get("location.bounding_box.maximum_latitude") or 0)
        min_lon = float(row.get("location.bounding_box.minimum_longitude") or 0)
        max_lon = float(row.get("location.bounding_box.maximum_longitude") or 0)
        area = abs(max_lat - min_lat) * abs(max_lon - min_lon)
        if 0.01 <= area <= 25.0:
            score += min(20, int(area * 10))
    except (ValueError, TypeError):
        pass

    return score


def parse_features(raw_features: str | None) -> list[str]:
    if not raw_features:
        return []
    return [feat.strip() for feat in raw_features.split("|") if feat.strip()]


def matches_criteria(row: dict[str, str]) -> bool:
    if row.get("data_type") != "gtfs":
        return False

    if str(row.get("is_official", "")).strip().lower() != "true":
        return False

    if row.get("status") == "deprecated":
        return False

    download_url = row.get("urls.latest") or row.get("urls.direct_download")
    if not download_url:
        return False

    features = {f.lower() for f in parse_features(row.get("features"))}
    return REQUIRED_FEATURES.issubset(features)


def write_preview_file(
    out_dir: str,
    feed_id: str,
    country: str,
    city: str,
    provider: str,
    name: str,
    url: str,
    features: list[str],
) -> None:
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, f"{feed_id}.txt")
    with open(out_path, "w", encoding="utf-8") as f:
        f.write(f"ID:          {feed_id}\n")
        f.write(f"Country:     {country}\n")
        f.write(f"City/Region: {city}\n")
        f.write(f"Provider:    {provider}\n")
        if name:
            f.write(f"Name:        {name}\n")
        f.write(f"Download:    {url}\n\n")
        f.write("Features:\n")
        features_str_lines = [f"  - {feat}" for feat in features]
        f.writelines(features_str_lines)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Filter GTFS feeds by official status and required features."
    )
    parser.add_argument(
        "--input",
        "-i",
        default="feeds_v2.csv",
        help="Path to the feeds CSV file (default: feeds_v2.csv)",
    )
    parser.add_argument(
        "--preview-dir",
        "-p",
        default=None,
        help="Optional directory to write fzf preview text files",
    )
    args = parser.parse_args()

    if not os.path.isfile(args.input):
        sys.stderr.write(f"Error: input file not found: {args.input}\n")
        return 1

    feeds_list = []
    with open(args.input, mode="r", encoding="utf-8", errors="ignore") as f:
        reader = csv.DictReader(f)
        for row in reader:
            if not matches_criteria(row):
                continue

            feed_id = row.get("id", "unknown")
            country = row.get("location.country_code", "??")
            subdiv = " ".join(row.get("location.subdivision_name", "").split())
            muni = " ".join(row.get("location.municipality", "").split())

            # Check county alias (e.g. Bronx County, NY -> New York City (Bronx))
            alias_key = (muni.lower(), subdiv.lower())
            if alias_key in COUNTY_ALIASES:
                city = f"{COUNTY_ALIASES[alias_key]}, {subdiv}"
            elif muni and subdiv and muni.lower() != subdiv.lower():
                city = f"{muni}, {subdiv}"
            elif muni:
                city = muni
            elif subdiv:
                city = subdiv
            else:
                city = "Unknown"

            provider = row.get("provider", "Unknown")
            name = row.get("name", "")
            download_url = row.get("urls.latest") or row.get("urls.direct_download")
            features = parse_features(row.get("features"))
            score = compute_importance_score(row)

            # Normalize all whitespace for clean TSV columns
            clean_city = " ".join(city.split())
            clean_provider = " ".join(provider.split())
            clean_name = " ".join(name.split())

            feeds_list.append(
                (
                    score,
                    feed_id,
                    country,
                    clean_city,
                    clean_provider,
                    clean_name,
                    download_url,
                    features,
                )
            )

    # Sort feeds so the most important / feature-rich networks appear first
    feeds_list.sort(key=lambda x: x[0], reverse=True)

    for (
        score,
        feed_id,
        country,
        clean_city,
        clean_provider,
        clean_name,
        download_url,
        features,
    ) in feeds_list:
        if args.preview_dir:
            write_preview_file(
                out_dir=args.preview_dir,
                feed_id=feed_id,
                country=country,
                city=clean_city,
                provider=clean_provider,
                name=clean_name,
                url=download_url,
                features=features,
            )

        sys.stdout.write(
            f"{feed_id}\t{country}\t{clean_city}\t{clean_provider}\t{clean_name}\t{download_url}\n"
        )

    sys.stderr.write(f"Found {len(feeds_list)} matching official feeds.\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())

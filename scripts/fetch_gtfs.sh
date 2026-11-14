#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CSV_FILE="${PROJECT_ROOT}/feeds_v2.csv"
PREVIEW_DIR="/tmp/idfm_gtfs_previews"
FILTER_SCRIPT="${SCRIPT_DIR}/filter_feeds.py"

if [ ! -f "$CSV_FILE" ]; then
    echo "Error: feeds_v2.csv not found at ${CSV_FILE}" >&2
    exit 1
fi

if ! command -v fzf &>/dev/null; then
    echo "Error: fzf is required for the interactive menu." >&2
    exit 1
fi

if ! command -v python3 &>/dev/null; then
    echo "Error: python3 is required." >&2
    exit 1
fi

mkdir -p "$PREVIEW_DIR"
rm -f "$PREVIEW_DIR"/*.txt 2>/dev/null || true

# Run Python filter and populate preview files
FEEDS=$("${FILTER_SCRIPT}" --input "$CSV_FILE" --preview-dir "$PREVIEW_DIR")

if [ -z "$FEEDS" ]; then
    echo "No matching feeds found." >&2
    exit 1
fi

# Select feed via fzf (search scoped strictly to city/region: --nth=3, ranked by importance: --tiebreak=begin,index)
SELECTED=$(echo "$FEEDS" | fzf \
    --delimiter=$'\t' \
    --nth=3 \
    --tiebreak=begin,index \
    --with-nth=1,2,3,4,5 \
    --prompt="Search by city > " \
    --header="[ID] [COUNTRY] [CITY/REGION] [PROVIDER] [NAME] (ESC to cancel)" \
    --preview="cat ${PREVIEW_DIR}/{1}.txt 2>/dev/null" \
    --preview-window=bottom:8:wrap)

if [ -z "$SELECTED" ]; then
    echo "Selection cancelled."
    exit 0
fi

FEED_ID=$(echo "$SELECTED" | cut -f1)
COUNTRY=$(echo "$SELECTED" | cut -f2)
CITY=$(echo "$SELECTED" | cut -f3 | tr ' /' '__' | tr -cd '[:alnum:]_-')
URL=$(echo "$SELECTED" | cut -f6)

TARGET_DIR="${PROJECT_ROOT}/data/gtfs_${FEED_ID}_${CITY}"
mkdir -p "$TARGET_DIR"

echo "Downloading ${FEED_ID} (${CITY})..."
echo "Source: ${URL}"
curl -L --progress-bar "$URL" -o "$TARGET_DIR/gtfs.zip"

echo "Extracting archive to ${TARGET_DIR}..."
unzip -q -o "$TARGET_DIR/gtfs.zip" -d "$TARGET_DIR"
rm -f "$TARGET_DIR/gtfs.zip"

echo "Dataset extracted to: ${TARGET_DIR}"

#!/bin/bash

WORK_DIR=$(dirname "$0")
DATADIR="$WORK_DIR"/../data

mkdir -p "$DATADIR"
python3 "$WORK_DIR"/muldata.py > "$DATADIR"/mul.csv
python3 "$WORK_DIR"/divdata.py > "$DATADIR"/div.csv

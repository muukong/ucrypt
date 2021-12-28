#!/bin/bash

WORK_DIR=$(dirname "$0")
DATADIR="$WORK_DIR"/../data

mkdir -p "$DATADIR"

MUL_DATA_FILE="$DATADIR"/mul.csv
if [ ! -f "$MUL_DATA_FILE" ]; then
  python3 "$WORK_DIR"/muldata.py > "$DATADIR"/mul.csv
fi

DIV_DATA_FILE="$DATADIR"/div.csv
if [ ! -f "$DIV_DATA_FILE" ]; then
  python3 "$WORK_DIR"/divdata.py > "$DATADIR"/div.csv
fi

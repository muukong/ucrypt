#!/bin/bash

WORK_DIR=$(dirname "$0")
DATADIR="$WORK_DIR"/../data

mkdir -p "$DATADIR"

MUL_DATA_FILE="$DATADIR"/mul.csv
if [ ! -f "$MUL_DATA_FILE" ]; then
  python3 "$WORK_DIR"/muldata.py > "$MUL_DATA_FILE"
fi

DIV_DATA_FILE="$DATADIR"/div.csv
if [ ! -f "$DIV_DATA_FILE" ]; then
  python3 "$WORK_DIR"/divdata.py > "$DIV_DATA_FILE"
fi

EXPMOD_DATA_FILE="$DATADIR"/expmod.csv
if [ ! -f "$EXPMOD_DATA_FILE" ]; then
  python3 "$WORK_DIR"/expmoddata.py > "$EXPMOD_DATA_FILE"
fi
#! /usr/bin/env python3

import os
from data import data

for key in data:
	os.system(f'../out/vt -c "set {key} {data[key]}"')
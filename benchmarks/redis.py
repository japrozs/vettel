#! /usr/bin/env python3
import os
from data import data

for key in data:
	os.system(f'redis-cli set {key} "{data[key]}"')
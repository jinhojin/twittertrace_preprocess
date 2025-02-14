# Twitter Trace Preprocessing

## Overview
This repository contains Python scripts for preprocessing Twitter trace data. The scripts clean, normalize, and transform raw Twitter trace into a structured format for further analysis.


## Code Explanation

### `preprocess_trace.cpp`
This code is responsible for the main data preprocessing steps. It:
1. Reads the input CSV file.
2. Fix the invalid key which has comma inside.
3. Delete lines except for get and delete operation.
4. Delete lines which has zero value size.

Usage:
```bash
./preprocess_trace input_trace output_trace
```

### `merge_traces.cpp`
This code is responsible for merging multiple twitter traces into one trace. It:
1. Reads the input CSV files.
2. Merge multiple twitter trace file into one trace in order of timestamp.
3. Delete the unnecessary headers and columns while merging.

Usage:
```bash
./merge_traces input_trace1 input_trace2 ..... merged_trace_name
```
### `sampling.cpp`
This code is responsible for sampling the big twitter trace file. It:
1. Reads the input CSV files.
2. Reduce the file by 1/n by picking one random line for every n lines.

Usage:
```bash
./sampling input_trace output_trace
```
### `trace_info.cpp`
This code is responsible for showing various trace information. It:
1. Reads the input trace files.
2. Calculate various information about the trace and make output text file.

Usage:
```bash
./trace_info input_trace output_textfile
```

### `hash_key.cpp`
This code is responsible for hashing tracefile keys. It:
1. Reads the input trace file key column.
2. Hash the keys using MD5 and truncate it to 16 character.

Usage:
```bash
./hash_key input_trace output_textfile
```

### `check_hash_conflict.cpp`
This code is responsible for checking hash conflict before hashing the keys. It:
1. Reads the input trace file key column.
2. Hash the keys using MD5 and truncate it to n character.
3. If there is hash conflict, alert the user and stop checking  

Usage:
```bash
./check_hash_conflict input_trace
```

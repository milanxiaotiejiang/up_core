#!/bin/bash

# Determine the path to the script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
# Navigate to the project root directory assuming the script is in the 'scripts' subdirectory
echo $SCRIPT_DIR

cd "$SCRIPT_DIR"/..

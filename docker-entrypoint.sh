#!/bin/bash
set -e

# Use defaults if variables are not set
MODEL_SIZE=${MODEL_SIZE}
MODEL_DIR=${MODEL_DIR}

echo "Configured Model: $MODEL_SIZE"
echo "Target Directory: $MODEL_DIR"

# Check if the directory is empty or doesn't exist
if [ ! -d "$MODEL_DIR" ] || [ -z "$(ls -A "$MODEL_DIR" 2>/dev/null)" ]; then
    echo "Model not found. Downloading $MODEL_SIZE model to $MODEL_DIR..."
    mkdir -p "$MODEL_DIR"
    # Run the provided script with the env vars
    ./download_model.sh --model "$MODEL_SIZE" --dir "$MODEL_DIR"
else
    echo "Model already exists in $MODEL_DIR, skipping download."
fi

echo "Running: ./qwen_asr -d $MODEL_DIR $@"
exec ./qwen_asr -d "$MODEL_DIR" "$@"
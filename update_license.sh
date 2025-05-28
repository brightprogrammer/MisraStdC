#!/bin/bash

# This script updates all copyright notices in source files to reflect public domain dedication

# Update files with "/// copyright : Copyright (c)" format
find . -type f -name "*.c" -o -name "*.h" | xargs grep -l "/// copyright : Copyright (c)" | while read file; do
  sed -i '' 's|/// copyright : Copyright (c) [0-9]\{4\}, Siddharth Mishra.*|/// This is free and unencumbered software released into the public domain.|g' "$file"
done

# Update files with "* Copyright (c)" format
find . -type f -name "*.h" | xargs grep -l "* Copyright (c)" | while read file; do
  sed -i '' 's|* Copyright (c) [0-9]\{4\} Siddharth Mishra.*|* This is free and unencumbered software released into the public domain.|g' "$file"
done

echo "All copyright notices have been updated to public domain dedication." 
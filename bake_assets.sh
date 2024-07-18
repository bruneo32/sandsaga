#!/bin/bash

source_dir="res"
destination_dir="src/assets/res"
mkdir -p "$destination_dir"

# Find all image files in the source directory
img_files=$(find "$source_dir" -type f -iname "*.png")

# Bitmap Fonts
font_files=$(find "$source_dir" -type f -iname "*.F08")

# Loop through each image file and generate a header file
for file in $img_files; do
    # Optimize image to reduce binary size
    optipng -o7 -strip all $file -out $file -backup -clobber

    filename=$(basename "$file")

    # Generate a header file using xxd
    xxd -i "$file" > "$destination_dir/${filename}.h"
done

for file in $font_files; do
    filename=$(basename "$file")
    xxd -i "$file" > "$destination_dir/${filename}.h"
done

echo "Conversion complete. Header files in to $destination_dir"

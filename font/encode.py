#!/usr/bin/python

import os
import argparse
from PIL import Image

def process_images(folder_path):
    # List to store the pixel arrays of each image
    images_pixel_data = {}

    # Iterate through all files in the specified folder
    for filename in os.listdir(folder_path):
        # Check if the file has a .png extension
        if filename.lower().endswith('.png'):
            # Open the image file
            img_path = os.path.join(folder_path, filename)
            with Image.open(img_path) as img:
                # Convert image to grayscale (black and white)
                bw_img = img.convert('L')

                # Convert the image to a list of lists representing the pixels
                pixel_data = [ 0 if p == 255 else 1 for p in list(bw_img.getdata()) ]

                # Reshape the pixel data into a 2D list (list of lists) with the image dimensions
                pixel_array = [pixel_data[i * bw_img.width:(i + 1) * bw_img.width] for i in range(bw_img.height)]

                # Append the pixel array of the image to the main list
                images_pixel_data[filename[:-4]] = pixel_array

    return images_pixel_data

def main():
    # Argument parser to get folder path from command line
    parser = argparse.ArgumentParser(description="Process PNG images in a folder to black and white pixel arrays.")
    parser.add_argument('folder', type=str, help="Path to the folder containing PNG images")
    args = parser.parse_args()

    # Process images and get pixel data
    images_pixel_data = process_images(args.folder)

    keys = sorted(images_pixel_data.keys())

    h = ""
    h += "#ifndef __FONT_H__\n"
    h += "#define __FONT_H__\n\n"

    # Print or return the result as needed
    for k in keys:
        pixel_array = images_pixel_data[k]
        data = f"const uint8_t FONT_{k}[ROWS][COLS] = " + "{\n"
        for row in pixel_array:
            data += "\t" + str(row).replace("[", "{").replace("]", "}") + ",\n"
        data = data[:-2] + "\n" + "};\n\n"
        h += data

    h += "#endif // __FONT_H__"
    print(h)

if __name__ == "__main__":
    main()


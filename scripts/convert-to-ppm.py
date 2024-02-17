import os
import json
import requests
from PIL import Image
from io import BytesIO


def convert_to_ppm(url, filename, target_height=64):
    # Download the image
    response = requests.get(url)
    if response.status_code != 200:
        print(f"Failed to download image. Status code: {response.status_code}")
        return False

    # Load the image from the downloaded bytes
    img = Image.open(BytesIO(response.content))

    # Calculate new width while maintaining the aspect ratio
    aspect_ratio = img.width / img.height
    new_width = int(target_height * aspect_ratio)

    # Resize the image using LANCZOS for high-quality downsampling
    img_resized = img.resize((new_width, target_height), Image.LANCZOS)

    # Create the images folder if it doesn't exist
    output_dir = "images"
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Save the resized image in PPM format
    output_path = os.path.join(output_dir, f"{filename}.ppm")
    img_resized.save(output_path, format="PPM")

    print(f"Image has been converted and saved to {output_path}")
    return True


# Function to process all images from a JSON file
def process_images_from_json(json_file):
    with open(json_file, "r") as file:
        urls = json.load(file)

    for url in urls["images"]:
        # Extract the image name from the URL
        image_name = os.path.splitext(os.path.basename(url))[0]
        convert_to_ppm(url, image_name)


# Example usage
json_file = "scripts/images.json"  # This should be the path to your JSON file containing a list of URLs
process_images_from_json(json_file)

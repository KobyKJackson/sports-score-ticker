import requests
from PIL import Image
from io import BytesIO


def convert_to_ppm(url, output_path, target_height=64):
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

    # Save the resized image in PPM format
    img_resized.save(output_path, format="PPM")

    print(f"Image has been converted and saved to {output_path}")
    return True


# Example usage
url = "https://a.espncdn.com/i/teamlogos/nba/500/scoreboard/lal.png"  # Replace with the actual URL
output_path = "output.ppm"
convert_to_ppm(url, output_path)

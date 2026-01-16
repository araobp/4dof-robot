import cv2
import numpy as np
import argparse
import sys

if __name__ == "__main__":
    """
    Generate ArUco marker images.
    Args:
        --id (int): Marker ID (default: 10).
        --size (int): Marker size (default: 300).
        --darkness (int): Darkness level (pixel value 0-255, default: 0).
    """
    parser = argparse.ArgumentParser(description="Generate ArUco marker images")
    parser.add_argument("--id", type=int, default=10, help="Marker ID (default: 10)")
    parser.add_argument("--size", type=int, default=300, help="Marker size (default: 300)")
    parser.add_argument("--darkness", type=int, default=0, help="Darkness level (pixel value 0-255, default: 0)")

    if len(sys.argv) == 1:
        parser.print_help(sys.stderr)
        sys.exit(1)

    args = parser.parse_args()

    marker_id = args.id
    marker_size = args.size

    # Load the predefined dictionary (4x4 bits, 50 markers)
    # Handle OpenCV 4.7.0+ API changes
    try:
        aruco_dict = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_4X4_50)
    except AttributeError:
        # For older OpenCV versions
        aruco_dict = cv2.aruco.Dictionary_get(cv2.aruco.DICT_4X4_50)

    # Initialize image for legacy support (older OpenCV versions require a pre-allocated image for drawMarker)
    img = np.zeros((marker_size, marker_size, 1), dtype="uint8")

    # Generate the marker
    # OpenCV 4.7.0+ uses generateImageMarker, older versions use drawMarker
    try:
        img = cv2.aruco.generateImageMarker(aruco_dict, marker_id, marker_size)
    except AttributeError:
        # For older OpenCV versions
        img = cv2.aruco.drawMarker(aruco_dict, marker_id, marker_size, img, 1)

    # Set darkness level
    darkness = args.darkness
    img[img == 0] = darkness

    # Save the generated marker image
    output_filename = f"marker_ID_{marker_id}.png"
    cv2.imwrite(output_filename, img)

    print(f"Generated ArUco marker with ID {marker_id} and saved as {output_filename}")

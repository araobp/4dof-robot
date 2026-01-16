import cv2
import numpy as np
import argparse

def generate_chessboard(cols=9, rows=6, square_size_px=100, margin_px=100, darkness=0):
    """
    Generate a chessboard image with the specified parameters.
    :param cols: Number of columns (number of squares).
    :param rows: Number of rows (number of squares).
    :param square_size_px: Size of one square in pixels.
    :param margin_px: Outer margin in pixels.
    :param darkness: Pixel value for the dark squares (0-255).
    :return: Chessboard image as a NumPy array.
    """
    # Calculate total image size including margins
    width = cols * square_size_px + 2 * margin_px
    height = rows * square_size_px + 2 * margin_px
    image = np.ones((height, width), dtype=np.uint8) * 255

    # Draw the black squares
    for r in range(rows):
        for c in range(cols):
            # Paint the square black if (row + col) is an odd number
            if (r + c) % 2 == 1:
                y1 = margin_px + r * square_size_px
                y2 = y1 + square_size_px
                x1 = margin_px + c * square_size_px
                x2 = x1 + square_size_px
                image[y1:y2, x1:x2] = darkness
    return image

def main():
    """
    Main function to parse arguments and generate the chessboard.
    """
    parser = argparse.ArgumentParser(description="Generate a chessboard pattern image for camera calibration.", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("--cols", type=int, default=10, help="Number of columns (squares).")
    parser.add_argument("--rows", type=int, default=7, help="Number of rows (squares).")
    parser.add_argument("--size", type=int, default=150, help="Size of each square in pixels.")
    parser.add_argument("--margin", type=int, default=100, help="Margin around the board in pixels.")
    parser.add_argument("--darkness", type=int, default=0, help="Darkness level for black squares (0-255).")
    parser.add_argument("--output", type=str, help="Output file name. Defaults to 'chessboard_{cols}x{rows}.png'")
    parser.add_argument("--no-display", action="store_true", help="Do not display the generated image.")

    args = parser.parse_args()

    # Determine output filename
    if args.output:
        file_name = args.output
    else:
        file_name = f"chessboard_{args.cols}x{args.rows}.png"

    # A note for the user about calibration pattern size
    print(f"Generating a {args.cols}x{args.rows} square board.")
    print(f"Note: For OpenCV calibration, the pattern size will be ({args.cols - 1}, {args.rows - 1}).")

    # Generate the image
    chessboard_img = generate_chessboard(
        cols=args.cols,
        rows=args.rows,
        square_size_px=args.size,
        margin_px=args.margin,
        darkness=args.darkness
    )

    # Save the image
    cv2.imwrite(file_name, chessboard_img)
    print(f"Successfully saved chessboard to: {file_name}")

    # Display the image unless suppressed
    if not args.no_display:
        cv2.imshow("Chessboard Pattern", chessboard_img)
        print("Press any key to close the window.")
        cv2.waitKey(0)
        cv2.destroyAllWindows()

if __name__ == '__main__':
    main()

import numpy as np
import cv2
import argparse

def run_calibration(chessboard_size=(9, 6), square_size=25.0, output_filename="calibration_data.npz", camera_source=0):
    """
    Runs camera calibration using a chessboard pattern.

    Args:
        chessboard_size (tuple): Number of inner corners per a chessboard row and column (corners_x, corners_y).
        square_size (float): Size of a single square edge in millimeters.
        output_filename (str): Path to save the calibration result (.npz file).
        camera_source (int or str): Camera index (int) or device path (str) for cv2.VideoCapture.
    """
    # Prepare object points, like (0,0,0), (1,0,0), (2,0,0) ....,(6,5,0)
    objp = np.zeros((chessboard_size[0] * chessboard_size[1], 3), np.float32)
    objp[:, :2] = np.mgrid[0:chessboard_size[0], 0:chessboard_size[1]].T.reshape(-1, 2) * square_size

    # Arrays to store object points and image points from all the images.
    objpoints, imgpoints = [], []
    # Initialize camera
    cap = cv2.VideoCapture(camera_source)

    print("Capture 20 images with 's' key. Move the board to different angles/distances for each shot. 'q' to quit.")
    count = 0
    gray = None
    last_captured_frame = None
    while count < 20:
        ret, frame = cap.read()
        if not ret: break
        original_frame = frame.copy()
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        # Find the chess board corners
        ret_found, corners = cv2.findChessboardCorners(gray, chessboard_size, None)
        if ret_found:
            cv2.drawChessboardCorners(frame, chessboard_size, corners, ret_found)
        cv2.imshow('Calibration', frame)
        key = cv2.waitKey(1) & 0xFF
        # 's' to save the current frame for calibration
        if key == ord('s') and ret_found:
            objpoints.append(objp)
            imgpoints.append(corners)
            last_captured_frame = original_frame
            count += 1
            print(f"Captured {count}/20. Now move the board.")
        elif key == ord('q'): break

    cap.release()
    cv2.destroyAllWindows()

    # Perform calibration if we have enough data
    if len(objpoints) > 0 and gray is not None:
        ret, mtx, dist, rvecs, tvecs = cv2.calibrateCamera(objpoints, imgpoints, gray.shape[::-1], None, None)
        # Save the camera matrix and distortion coefficients
        np.savez(output_filename, mtx=mtx, dist=dist)
        print(f"Saved {output_filename}")

        # Visualize the result on the last captured frame
        if last_captured_frame is not None:
            h, w = last_captured_frame.shape[:2]
            # Refine the camera matrix based on the image size
            newcameramtx, roi = cv2.getOptimalNewCameraMatrix(mtx, dist, (w, h), 1, (w, h))
            # Undistort
            dst = cv2.undistort(last_captured_frame, mtx, dist, None, newcameramtx)
            cv2.imshow('Original', last_captured_frame)
            cv2.imshow('Undistorted', dst)
            cv2.waitKey(0)
            cv2.destroyAllWindows()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Camera calibration using a chessboard pattern.")
    parser.add_argument("--corners-x", type=int, default=9, help="Number of inner corners along the x-axis (default: 9).")
    parser.add_argument("--corners-y", type=int, default=6, help="Number of inner corners along the y-axis (default: 6).")
    parser.add_argument("--size", type=float, default=25.0, help="Size of a square edge in millimeters (default: 25.0).")
    parser.add_argument("--output", type=str, default="calibration_data.npz", help="Output file for calibration data (default: calibration_data.npz).")
    parser.add_argument("--source", default="0", help="Camera source (index or device path, default: 0).")
    args = parser.parse_args()

    chessboard_size = (args.corners_x, args.corners_y)
    
    # If the source argument is a digit, convert to int (for index), otherwise keep as string (for path)
    source = int(args.source) if args.source.isdigit() else args.source

    print(f"Looking for a {chessboard_size[0]}x{chessboard_size[1]} corner pattern.")

    run_calibration(
        chessboard_size=chessboard_size,
        square_size=args.size,
        output_filename=args.output,
        camera_source=source
    )

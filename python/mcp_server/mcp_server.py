import cv2
import cv2.aruco as aruco
import numpy as np
from fastmcp import FastMCP

# Configuration settings for the physical table dimensions (in millimeters)
TABLE_WIDTH = 800.0  # mm
TABLE_HEIGHT = 600.0 # mm

mcp = FastMCP("Physical-AI-Server")

# Load camera calibration data (camera matrix and distortion coefficients) from the saved file
try:
    data = np.load("calibration_data.npz")
    MTX, DIST = data["mtx"], data["dist"]
except:
    print("Warning: calibration_data.npz not found.")

# Initialize ArUco marker dictionary (4x4 markers, 50 variants) and detector parameters
dictionary = aruco.getPredefinedDictionary(aruco.DICT_4X4_50)
parameters = aruco.DetectorParameters()
H_MATRIX = None # Global variable to store the calculated Homography matrix for perspective transformation
NEW_MTX = None  # Global variable to store the refined camera matrix

def _calibrate_table() -> dict:
    """
    Detects the 4 corners of the table (ID 0,1,2,3) and generates the transformation matrix H.
    This matrix maps image coordinates to real-world table coordinates.
    """
    global H_MATRIX, NEW_MTX
    cap = cv2.VideoCapture(0)
    ret, frame = cap.read()
    cap.release()
    if not ret: return {"status": "error", "message": "Camera error"}

    # Apply lens distortion correction to the captured frame using the calibration matrix
    h, w = frame.shape[:2]
    new_mtx, _ = cv2.getOptimalNewCameraMatrix(MTX, DIST, (w,h), 1, (w,h))
    NEW_MTX = new_mtx
    undistorted = cv2.undistort(frame, MTX, DIST, None, new_mtx)

    corners, ids, _ = aruco.detectMarkers(undistorted, dictionary, parameters=parameters)
    if ids is not None and len(ids) >= 4:
        # Calculate the center coordinate (x, y) for each detected marker ID
        pts_map = {int(id[0]): c[0].mean(axis=0) for id, c in zip(ids, corners)}
        try:
            src_pts = np.array([pts_map[0], pts_map[1], pts_map[2], pts_map[3]], dtype="float32")
            dst_pts = np.array([[0,0], [TABLE_WIDTH,0], [TABLE_WIDTH,TABLE_HEIGHT], [0,TABLE_HEIGHT]], dtype="float32")
            H_MATRIX, _ = cv2.findHomography(src_pts, dst_pts)
            return {"status": "success", "ids": list(pts_map.keys())}
        except KeyError:
            return {"status": "error", "message": "IDs 0-3 required"}
    return {"status": "error", "message": "Insufficient markers"}

@mcp.tool()
def calibrate_table() -> dict:
    """
    Detects the 4 corners of the table (ID 0,1,2,3) and generates the transformation matrix H.
    This matrix maps image coordinates to real-world table coordinates.
    """
    return _calibrate_table()

def _convert_uv_to_ground(u: float, v: float) -> dict:
    """
    Converts image coordinates (u,v) to real space (x,z) in mm.
    Requires calibrate_table to be run first to establish the Homography matrix.
    """
    if H_MATRIX is None: return {"error": "Run calibrate_table first"}
    
    # Step 1: Remove lens distortion from the input image coordinates (u, v)
    # Note: This maps the raw pixel coordinates to the undistorted image plane
    src_pt = np.array([[[u, v]]], dtype="float32")
    
    # Use the refined camera matrix (NEW_MTX) if available, otherwise fallback to original MTX
    # This ensures consistency with the frame used in calibrate_table
    P_matrix = NEW_MTX if NEW_MTX is not None else MTX
    undistorted_pt = cv2.undistortPoints(src_pt, MTX, DIST, P=P_matrix)
    
    # Step 2: Apply the Homography matrix to project the point onto the table plane (bird's-eye view)
    ground_pt = cv2.perspectiveTransform(undistorted_pt, H_MATRIX)
    
    return {
        "x_mm": float(ground_pt[0][0][0]),
        "z_mm": float(ground_pt[0][0][1])
    }

@mcp.tool()
def convert_uv_to_ground(u: float, v: float) -> dict:
    """
    Converts image coordinates (u,v) to real space (x,z) in mm.
    Requires calibrate_table to be run first to establish the Homography matrix.
    """
    return _convert_uv_to_ground(u, v)

def run_local_test():
    """Runs a local interactive test with video feed and mouse click coordinate conversion."""
    print("Starting local test mode...")
    print("Step 1: Calibrating table (ensure markers 0-3 are visible)...")
    res = _calibrate_table()
    if res.get("status") != "success":
        print(f"Calibration failed: {res.get('message')}")
        return

    print("Calibration successful. Starting video feed.")
    print("Click on the image to get ground coordinates. Press 'q' to quit.")

    cap = cv2.VideoCapture(0)
    click_state = {"pt": None, "ground": None}

    def on_mouse(event, x, y, flags, param):
        if event == cv2.EVENT_LBUTTONDOWN:
            click_state["pt"] = (x, y)
            result = _convert_uv_to_ground(float(x), float(y))
            if "x_mm" in result:
                click_state["ground"] = (result["x_mm"], result["z_mm"])

    cv2.namedWindow("Local Test")
    cv2.setMouseCallback("Local Test", on_mouse)

    while True:
        ret, frame = cap.read()
        if not ret: break

        if click_state["pt"]:
            cv2.circle(frame, click_state["pt"], 5, (0, 0, 255), -1)
            if click_state["ground"]:
                text = f"X: {click_state['ground'][0]:.1f}mm, Z: {click_state['ground'][1]:.1f}mm"
                cv2.putText(frame, text, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

        cv2.imshow("Local Test", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1 and sys.argv[1] == "test":
        run_local_test()
    else:
        mcp.run(transport="sse")

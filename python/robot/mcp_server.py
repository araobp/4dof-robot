"""
ロボットアームおよびビジョンシステムを統合制御するMCP (Model-Centric Protocol) サーバーです。

このサーバーは以下の機能を提供します:
- FastMCPフレームワークを介したAIエージェントとのツールベースの対話
- Arduinoベースのロボットアームとのシリアル通信
- OpenCVベースのビジョンシステム（ArUcoマーカーによる姿勢推定と座標変換）
"""
from fastmcp import FastMCP
import serial
import time
import json
import base64
import cv2
from vision_system import VisionSystem

# --- 基本設定 ---
# ロボットアーム（Arduino）が接続されているシリアルポート
SERIAL_PORT = '/dev/cu.usbmodem101'
# シリアル通信のボーレート
BAUD_RATE = 9600
# コマンド応答のタイムアウト（秒）
TIMEOUT = 45

# --- ビジョンシステム設定 ---
# カメラキャリブレーションによって得られた内部パラメータファイル
CAMERA_PARAMS_PATH = '../vision/chessboard/calibration_data.npz'
# 座標系の原点として使用するArUcoマーカーのID
ARUCO_MARKER_ID = 14
# ArUcoマーカーの物理的な一辺の長さ (cm)
ARUCO_MARKER_SIZE_CM = 6.3
# 使用するカメラのデバイスID
CAMERA_ID = 0

mcp = FastMCP(
    "RobotArmController"
)

# --- グローバルリソース ---
# VisionSystemとシリアル接続は、必要になるまで初期化しない（遅延初期化）
_vision_system = None
_serial_conn = None

# --- 内部ヘルパー関数 ---
def _fetch_workpiece_data():
    """作業対象物（ワーク）の定義情報を返す"""
    return {
        "earplug_case": {
            "name": "耳栓ケース",
            "height": 43.0,
            "approach_z_offset": 50.0,
            "description": "円筒形のケース。Z=20で把持し、移動は安全高度 Z=93 (43+50) を経由してください。"
        },
        "base_tray": {
            "name": "配置トレイ",
            "height": 5.0,
            "approach_z_offset": 60.0,
            "description": "配置用の平坦な面。Z=5で解放、Z=65で移動してください。"
        }
    }

def get_vision_system():
    """
    VisionSystemのシングルトンインスタンスを取得します（遅延初期化）。
    初回呼び出し時にカメラを初期化するため、不要なリソース確保を防ぎます。
    """
    global _vision_system
    if _vision_system is None:
        try:
            _vision_system = VisionSystem(
                camera_params_path=CAMERA_PARAMS_PATH,
                marker_id=ARUCO_MARKER_ID,
                marker_size_cm=ARUCO_MARKER_SIZE_CM,
                cam_id=CAMERA_ID
            )
            print("Vision system initialized successfully.")
        except Exception as e:
            print(f"Failed to initialize VisionSystem: {e}")
            return None
    return _vision_system

def get_serial():
    """
    シリアルポート接続のシングルトンインスタンスを取得します（遅延初期化）。
    Arduinoとの接続を確立し、リセット後の安定待機を行います。
    """
    global _serial_conn
    if _serial_conn and _serial_conn.is_open:
        return _serial_conn
    try:
        _serial_conn = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=TIMEOUT)
        # Arduinoはシリアル接続時にリセットがかかるため、起動シーケンスが完了するのを待つ
        time.sleep(2)
        _serial_conn.reset_input_buffer()
        return _serial_conn
    except Exception as e:
        return None

def send_command(cmd: str) -> str:
    """
    コマンドをArduinoに送信し、応答を待機する。
    
    通信プロトコル：
    1. コマンド文字列の末尾に改行コード `\\n` を付与して送信。
    2. Arduinoからの応答を一行ずつ読み込む。
    3. 応答の最後にプロンプト文字 '%' が送られてきたら、コマンド完了とみなす。
    """
    conn = get_serial()
    if not conn: return "Error: ロボットに接続できません。"
    try:
        full_cmd = cmd.strip() + "\n"
        conn.write(full_cmd.encode('utf-8'))
        response = []
        while True:
            line = conn.readline().decode('utf-8', errors='replace').strip()
            # コマンド完了の合図
            if line == '%': break
            # タイムアウト（readlineが空文字を返す）かつバッファが空の場合、ループを抜ける
            if not line and conn.in_waiting == 0: break
            if line: response.append(line)
        return "\n".join(response) if response else "Success"
    except Exception as e:
        return f"Error: {e}"

# =================================================================
# MCPツール群 (AIエージェントが利用するAPI)
# docstring（ここの説明文）が、AIの思考と行動の源泉となります。
# =================================================================

@mcp.tool()
def get_workpiece_catalog() -> str:
    """
    システムに登録されている全てのワーク（作業対象物）のカタログ（物理特性リスト）を取得します。
    JSON形式の文字列で返されます。

    【AIへの指示】
    ロボットアームで物体を操作する計画（ピック＆プレイスなど）を立てる際には、
    対象物の正確な「高さ(height)」や、衝突を避けるための「アプローチ安全オフセット(approach_z_offset)」を
    把握するため、**必ず最初にこのツールを実行してください。**

    【データ構造の詳細】
    - name: ワークの名称（日本語）
    - height: ワーク自体の物理的な高さ (mm)。アームを下降させる際の基準点となります。
    - approach_z_offset: ワークの上方でアームを水平移動させる際の、安全マージン (mm)。
    - description: ワークに関する補足情報。

    【重要：安全な経路計画のための計算式】
    ワークを持った状態で水平移動を行う際は、必ず以下の式で算出される「安全高度」までアームを上昇させてください。
    安全高度 (Z座標) = ワークの height + approach_z_offset
    """
    return json.dumps(_fetch_workpiece_data(), ensure_ascii=False, indent=2)

@mcp.tool()
def execute_sequence(commands: str) -> str:
    """
    ロボットアームに一連の動作（コマンドシーケンス）をセミコロン ';' 区切りで送信します。

    【コマンド文法】
    1. move x=<値> y=<値> z=<値> s=<速度>:
       アームの先端 (TCP: Tool Center Point) を指定の3次元座標 (mm) へ移動させます。
       sは移動速度で、0から100の範囲で指定します。
    2. grip <open|close>:
       グリッパーを開きます ('open') または閉じます ('close')。
    3. delay t=<ミリ秒>:
       指定した時間 (ミリ秒単位) だけ動作を停止します。物理的な動作が安定するのを待つために使用します。

    【AI管制官への絶対遵守ルール：経路計画】
    - **待機時間の挿入**: 把持 (grip close) や解放 (grip open) の直後には、グリッパーが完全に動作するのを待つため、**必ず 'delay t=1000' (1秒待機) を挿入してください。**
    - **安全高度の計算と利用**: 水平移動の前には、必ず `get_workpiece_catalog` を参照し、操作対象のワークに応じた安全高度を算出してください (安全高度 = height + approach_z_offset)。
    - **衝突回避**: ワークを掴んだ後の水平移動は、必ず一度「安全高度」までアームを上昇させてから行ってください。低い高度のまま直線的に移動すると、他の物体と衝突する危険があります。
    """
    return send_command(commands)

@mcp.tool()
def get_robot_status() -> str:
    """
    ロボットアームの現在の状態（TCP座標、各関節の角度など）を取得します。
    動作計画を立てる前に、アームの現在位置を正確に把握するために使用してください。
    """
    return send_command("dump")

@mcp.tool()
def get_live_image(visualize_axes: bool = True) -> str:
    """
    カメラから歪み補正済みのライブ映像を1フレームキャプチャし、Base64エンコードされたJPEG形式で返します。
    この画像は、ロボットの作業領域の現在の状況を視覚的に確認するために使用します。
    基準マーカーが検出されている場合は、画像の座標系を示す3D軸が重畳描画されます。
    返り値は `{"image_jpeg_base64": "..."}` という形式のJSON文字列です。

    Args:
        visualize_axes (bool): Trueの場合、画像に座標軸を描画します。
    """
    vs = get_vision_system()
    if not vs:
        return "Error: Vision system is not available."
    
    # 姿勢を更新し、マーカーが見える場合に軸が描画されるようにする
    vs.update_pose(visualize_axes=visualize_axes)
    
    base64_image = vs.get_undistorted_image_base64()
    if base64_image:
        return json.dumps({"image_jpeg_base64": base64_image})
    else:
        return "Error: Failed to capture image from camera."

@mcp.tool()
def convert_image_coords_to_world(u: int, v: int) -> str:
    """
    歪み補正済み画像のピクセル座標(u, v)を、ロボットの作業平面(Z=0)上の
    実世界座標(x, y) [cm] に変換します。
    このツールは、画像上でユーザーがクリックした点などを、ロボットが目標とすべき物理座標に変換するために不可欠です。

    【前提条件】
    - この座標変換は、カメラの映像内に基準となるArUcoマーカーが明確に映っている必要があります。
    - マーカーが検出できない場合、座標変換は失敗し、エラーメッセージを返します。

    Args:
        u (int): 変換したい画像の水平方向（横軸）のピクセル座標。
        v (int): 変換したい画像の垂直方向（縦軸）のピクセル座標。
    
    Returns:
        変換に成功した場合: `{"x": float, "y": float, "z": 0.0, "image_jpeg_base64": "..."}` という形式のJSON文字列。
        座標の単位はcm。`image_jpeg_base64` には、ターゲット位置を描画した画像のBase64エンコード文字列が含まれます。
        変換に失敗した場合: 失敗理由を示すエラーメッセージ文字列。
    """
    vs = get_vision_system()
    if not vs:
        return "Error: Vision system is not available."

    # 座標変換のために最新のマーカー姿勢を取得・更新する
    vs.update_pose(visualize_axes=True)

    coords, annotated_frame = vs.convert_2d_to_3d(u, v, draw_target=True)
    
    if coords:
        response_data = coords
        if annotated_frame is not None:
            _, buffer = cv2.imencode('.jpg', annotated_frame)
            base64_image = base64.b64encode(buffer).decode('utf-8')
            response_data["image_jpeg_base64"] = base64_image

        return json.dumps(response_data)
    else:
        return "Error: Could not perform coordinate conversion. Ensure the ArUco marker is clearly visible to the camera."

if __name__ == "__main__":
    try:
        # MCPサーバーを起動し、HTTP経由での接続を待ち受ける
        mcp.run(transport="streamable-http", host="0.0.0.0", port=8888)
    finally:
        # プログラム終了時に、確保したリソースを確実に解放する
        if _vision_system:
            _vision_system.release() # カメラを解放
            print("Vision system resources released.")
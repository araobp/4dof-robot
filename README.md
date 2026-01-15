# Beginning Physical AI

まだ開始したばかりのプロジェクト(2026/1/12開始)。ブレインストーミングの状態だが、構想はまとまりつつある。

ここで、ICTエンジニアが小さな投資(1万円以下)でフィジカルAIの勉強を始めるための道筋を示したい。

## 部品・ツール

### Camera Calibration用のチェスボード

=> [OpenCVでチェスボード作成とカメラキャリブレーションデータ作成](python/chessboard)

<img src="python/chessboard/chessboard_10x7.png" width="300px">

### ArUCOマーカー生成

=> [OpenCVでArUCOマーカー作成](python/aruco)

<table>
  <tr>
    <td>
      <img src="python/aruco/marker_ID_1.png" width="80px">
    </td>
    <td>
      <img src="python/aruco/marker_ID_2.png" width="80px">
    </td>
    <td>
      <img src="python/aruco/marker_ID_3.png" width="80px">
    </td>
    <td>
      <img src="python/aruco/marker_ID_4.png" width="80px">
    </td>
    <td>
      <img src="python/aruco/marker_ID_5.png" width="80px">
    </td>
    <td>
      <img src="python/aruco/marker_ID_6.png" width="80px">
    </td>
    <td>
      <img src="python/aruco/marker_ID_7.png" width="80px">
    </td>
    <td>
      <img src="python/aruco/marker_ID_8.png" width="80px">
    </td>
  </tr>
</table>

### MCPサーバ

=> [FastMCPベースのMCPサーバ](python/mcp_server)

## ゴール

- Amazonで購入中の[4軸ロボット](https://www.amazon.co.jp/dp/B0CX8QZVFQ?ref=ppx_yo2ov_dt_b_fed_asin_title)をIKで制御
- Arduino UNO R3をロボットコントローラとするが、４軸ロボットとの接続はAmazonで購入中の[この基板](https://www.amazon.co.jp/gp/product/B078YRJ8D7?ref=ppx_pt2_dt_b_prod_image)を経由。
- Arduino UNO R3をUSBシリアル経由でラズパイと接続
- ラズパイへ接続したUSBカメラ画像からロボットベースの平面座標を得る、マーカーで位置合わせする、Gemini Robotics-ER向け。
- MCPがUSBバスみたいなものと言うなら、その通りにMCPを使ってみる：ラズパイ<-MCP->SvelteKit, ラズパイ<-MCP->Unity、これは昔でいうCORBAとかRMIだね。この辺、ICTエンジニアの強み。
- SvelteKitでGemini Liveアプリをつくり、ロボットを音声で制御する
- Unity上にロボットのデジタルツインをつくり、ロボットと動作を同期させる
- STMicro製のI2C 8x8マトリクスの近接センサーと連携させる：簡易LiDAR（ピック、プレイスのx,y,z最適化）と安全管理

たぶん、これを一通りやれば、私、フィジカルAIの仕組みを一通り知っていると言えるようになる。12月に国際ロボット展で見てきた世界のミニチュア。GMO/Ugoとか、Epson、川崎重工、デンソー、安川、ファナックなどの世界。

そこに至るまで、所用２ヶ月。

これが発展し、超多間接ロボット（例えば人型ロボット）のIK動作の強化学習など行うと、本格的なフィジカルAIになる。

大谷選手は20msecの世界でホームラン打っているが、フィジカルAIはそのレベルに辿り着けるのか？

## My Arduino board

[Arduino UNO R3](https://docs.arduino.cc/hardware/uno-rev3/)

## 関連する私のGitHubプロジェクト

- Gemini Robotics-ERでUnity上の多軸ロボットを制御(Unity)　=> https://github.com/araobp/unity-robotics
- Gemini Liveで博物館内展示の自動音声応対(SvelteKit, Godot) => https://github.com/araobp/godot-museum

## 進め方

### 必要なもの

- 4DoF Robot Kit
- Arduino UNO
- Mac or PC
- RaspberryPi (SvelteKit(Node.js)-Arduino接続用)
- 広角USBカメラ
- 電子部品、基板、近接センサー
- Arudino IDE2
- SvelteKit, Python(OpenCV)?, OpenCV.js?
- Unity

### 最初の第一歩

たぶん、フィジカルAI始める時、みんなが通る道。

- Step1: ArduinoのオンボードLEDでLチカ、シリアルモニターから"1"を送信すると点滅、"0"を送信すると停止 => [コード](test/sketch_apr8a)
- Step2: ブレッドボード上のLEDをArduinoからLチカ、今回はタイマー割り込みでLチカを制御する => [コード](test/sketch_jan12a)
- Step3: ブレッドボードを挟んでサーボモータSG90とArduinoを接続しサーボの回転を制御、VDD-GND間に470uFの電解コンデンサを挟んで回転動作を安定化
- Step4: オシロスコープでArduinoが発するPWMの波形を可視化
- Step5: Arduino向け三角関数ライブラリの評価、IK(Inverse Kinemaics)の計算
- Step6: USBカメラ画像の座標を平面座標へ変換

### ロボットコントローラの設計と開発

...

### ロボットのデジタルツイン設計と開発

...

### Gemini Robotics-ERでロボット制御

...

### Gemini Liveでロボット制御

Function Calling経由で...

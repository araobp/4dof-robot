# Beginning Physical AI

まだ開始したばかりのプロジェクト(2026/1/12開始)。ブレインストーミングの状態だが、構想はまとまりつつある。2025年12月に国際ロボット展参加し、フィジカルAIへの興味が増大。

ここ8年間にわたるIoT/画像処理/AI/デジタルツイン/AR/生成AI関連プロトタイピング活動(Minimum Viable Products)で獲得してきたスキルの集大成。MVPは使い捨てなので、全ての活動において、アジャイル、低予算、少人数で内製を貫いてきた。MVPとは、本物の動きを確認出来て、その先のイメージを掴むのが目的。

マイルストーンごとにQiitaへ記事を投稿する。ICTエンジニアが小さな投資(1万円以下)でフィジカルAIの勉強を始めるための道筋を示したい。簡易LiDARや赤外線アレイセンサまでやると2万円を超えてしまうが。

## Qiita記事投稿

(1) [Lチカから始めるフィジカルAI: Gemini Live + SvelteKit + Arduino UNO](https://qiita.com/araobp/items/5ac9b141c64e4967b61e)
(2) ...

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
- Panasonic製のI2C 8x8マトリクス赤外線アレイセンサーと連携させる：異常検知

## My Arduino board

[Arduino UNO R3](https://docs.arduino.cc/hardware/uno-rev3/)

## 関連する私のGitHubプロジェクト

- Gemini Robotics-ERでUnity上の多軸ロボットを制御(Unity)　=> https://github.com/araobp/unity-robotics
- Gemini Liveで博物館内展示の自動音声応対(SvelteKit, Godot) => https://github.com/araobp/godot-museum
- Arduinoで赤外線アレイセンサー(異常検知に使える？) => https://github.com/araobp/arduino-infrared-array-sensor
- ロボット巡回(Godotの3D機能が弱く中断) => https://github.com/araobp/airport

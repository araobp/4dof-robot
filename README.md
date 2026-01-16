# Beginning Physical AI

<img src="docs/4DoF_Robot_Kit.jpg" width="500px">

本プロジェクトは、2025年12月の国際ロボット展での知見をきっかけに始動した、フィジカルAIの可能性を模索する試みです。現在はまだ構想段階（ブレインストーミング中）ではありますが、徐々に実装の方向性が固まりつつあります。

## 背景とコンセプト

本活動は、過去8年間にわたりIoT、画像処理、デジタルツイン、生成AIなどの領域で行ってきたプロトタイピング活動（MVP開発）の集大成として位置づけています。

これまでの活動では、以下のスタイルを一貫して守ってきました。

- 内製へのこだわり: アジャイルかつ少人数・低予算での開発
- MVP（Minimum Viable Product）の本質: 「実際に動くもの」を通じて、次なるビジョンを具体化すること

数多くのMVPを積み重ねるプロセスは、世の中のITシステムと物理的な仕組みの相互作用を深く理解するための貴重な経験となりました。

## プロジェクトの目的

- ICTエンジニアが、過度な負担なく「フィジカルAI」の学習を始められるロードマップを提示することを目指しています。
- 学習のアクセシビリティ: 1万円程度のスモールスタートを想定（LiDAR等の高価なセンサーを除いた構成）。
- フルスタックな構成: 電子工作、Arduino、Python、HTML5、Unity、そして物理演算までを網羅し、小さなシステムとして完結させます。

## 今後の展開

開発の節目ごとに、技術的な知見をQiitaにて公開していく予定です。フィジカルAIの第一歩を踏み出そうとする方の、ささやかな道標となれば幸いです。

## Qiita記事投稿

1. [Lチカから始めるフィジカルAI: Gemini Live + SvelteKit + Arduino UNO](https://qiita.com/araobp/items/5ac9b141c64e4967b61e)
2. ...

## 構成

```
[USBカメラ]-----------------USB---------------------------------------------+
                                                                           |
[サーボモータコントローラ]---I2C---[4DoFロボットコントローラ]---USB Serial---[MCP Server]-----MCP-----[MCP Client]
                                      |  Arduino UNO                   ラズパイ              SvelteKitベースウエブアプリ
[センサ]-------------------------------+                                                     Unityベース4DoFロボットデジタルツイン



```

## 部品・ツール

### 4DoFロボットアーム

Amazonで購入：[4DoFロボット](https://www.amazon.co.jp/dp/B0CX8QZVFQ?ref=ppx_yo2ov_dt_b_fed_asin_title)

組み立てマニュアル：
- https://www.makerbuying.com/docs/4dofarm/over-view
- https://www.youtube.com/watch?v=Q9JOKQaIR1w

Google検索エンジンの言語設定を英語にして、"SNAM 1200 1300 1500 4 DOF ARM acrylic robot arm"で検索すると、たくさん出てくる。

### NXP PCA9685 PWMコントローラ

Amazonで購入：[KKHMF PCA9685 16チャンネル 12-ビット PWM Servo モーター ドライバー IIC モジュール](https://www.amazon.co.jp/dp/B078YRJ8D7)

I2C address: 0x40 (A0 - A5 の端子を半田付けしてクローズすることでアドレス変更可能だが、今回はこのままで良し）。

- ガイド：https://en.hwlibre.com/Complete-guide-to-the-PCA9685-controller-with-Arduino-and-more/
- データシート：https://cdn-shop.adafruit.com/datasheets/PCA9685.pdf
- Arduinoライブラリ：https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library

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

### Mac/PC/ラズパイ上で動作させる MCP Server

=> [FastMCPベースのMCPサーバ](python/mcp_server)

### Arduinoベースの4DoFロボットコントローラ

...

### SvelteKitベースのウエブアプリ (MCP Client)

...

### Unityベースの4DoFロボットデジタルツイン (MCP Client)

...

## ゴール

- Arduino UNO R3をロボットコントローラとするが、４DoFロボットとの接続はAmazonで購入中の[この基板](https://www.amazon.co.jp/gp/product/B078YRJ8D7?ref=ppx_pt2_dt_b_prod_image)を経由。
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

## 読みたい論文
- [Gemini Robotics](https://arxiv.org/pdf/2503.20020)
- [Gemini Robotics 1.5](https://arxiv.org/pdf/2510.03342)
- [Robot Learning: A Tutorial](https://arxiv.org/pdf/2510.12403v1)

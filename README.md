# Beginning Physical AI

まだ、始まったばかりのプロジェクト。

<img src="docs/4DoF_Robot_Kit.jpg" width="500px">

## 背景

個人の自己研鑽として低予算（1万円以下）でフィジカルAIを始めるには、
- Unityと物理エンジンでシミュレーションから始める、Unityを基盤とする
- 4DoFロボットアームキットで電子工作から始める、Arduinoやラズパイを基盤とする

の二つの始め方がある。両方で始めれば、より、フィジカルAIの仕組みを理解できる。

このような始め方でも、AIの学習のところ（模倣学習、強化学習など）を除けば、網羅的に学習できる。Gemini Robotics-ERなど、学習済みAIを活用すれば、それなりの動作を期待できる。

個々の仕組みを理解すると、フィジカルAIの現状が見えてくる：まだ、人間の指示にしたがい、移動するとか、物を運ぶとか、検査するとか、監視するとか、その程度しか出来ていない。それらは、これまでは、IoT+AIカメラ＋AR＋ドローン＋移動ロボット＋人手で解決しようとしていた部分。HAL 9000が宇宙船全体を制御するくらいの世界を考えたい。そういう意味で、オフィス内で使われるAIエージェントとフィジカルAIを区別する必要もない。最初から一緒に考える。そういう意味で、MCPが重要な要素になるかもしれない。ただ、IoTとかOTのネットワークは、pubsubで通信するケースが多い。

シミュレーションからの始め方については開発済み：https://github.com/araobp/unity-robotics

ここでは、電子工作からの始め方を開発する。

開発の節目ごとに、技術的な知見をQiitaにて公開していく予定。

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

ArduinoのI2C通信では、Wire.begin();を実行すると、デジタルピン2と3（SDA, SCL）に内部プルアップ抵抗が自動的に有効になりますが、これはおよそ20kΩ〜50kΩと抵抗値が比較的高めなので、より確実な通信のためには、通常4.7kΩ〜10kΩ程度の外付けプルアップ抵抗をSDA/SCLラインに接続するのが推奨されます。Wire.begin()だけでは不十分な場合があるため、外部抵抗の追加を検討しましょう。

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

## 関連する私のGitHubプロジェクト

- Gemini Robotics-ERでUnity上の多軸ロボットを制御(Unity)　=> https://github.com/araobp/unity-robotics
- Gemini Liveで博物館内展示の自動音声応対(SvelteKit, Godot) => https://github.com/araobp/godot-museum
- Arduinoで赤外線アレイセンサー(異常検知に使える？) => https://github.com/araobp/arduino-infrared-array-sensor
- ロボット巡回(Godotの3D機能が弱く中断) => https://github.com/araobp/airport

## 読みたい論文
- [Gemini Robotics](https://arxiv.org/pdf/2503.20020)
- [Gemini Robotics 1.5](https://arxiv.org/pdf/2510.03342)
- [Robot Learning: A Tutorial](https://arxiv.org/pdf/2510.12403v1)

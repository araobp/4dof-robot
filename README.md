# 4 DoF ロボット

まだ開始したばかりのプロジェクト(2026/1/12開始)。

## ゴール

- Amazonで購入中の[4軸ロボット](https://www.amazon.co.jp/dp/B0CX8QZVFQ?ref=ppx_yo2ov_dt_b_fed_asin_title)を制御
- Arduino UNO R4をロボットコントローラとするが、４軸ロボットとの接続は、自作電源ボードを挟む（USB電源アダプターから給電、逆流防止、過電流防止回路付き）。
- Arduino UNO R4をUSBシリアル経由でPC上のSvelteKitアプリやUnityアプリと接続させる
- SvelteKitでGemini Liveアプリをつくり、ロボットを音声で制御する
- Unity上にロボットのデジタルツインをつくり、ロボットと動作を同期させる

## My Arduino board

[Arduino UNO R3](https://docs.arduino.cc/hardware/uno-rev3/)

## 関連する私のGitHubプロジェクト

- Gemini Robotics-ERでUnity上の多軸ロボットを制御(Unity)　=> https://github.com/araobp/unity-robotics
- Gemini Liveで博物館内展示の自動音声応対(SvelteKit, Godot) => https://github.com/araobp/godot-museum

## 進め方

### 最初の第一歩

たぶん、フィジカルAI始める時、みんなが通る道。

- Step1: ArduinoのオンボードLEDでLチカ、シリアルモニターから"1"を送信すると点滅、"0"を送信すると停止 => [コード](test/sketch_apr8a)
- Step2: ブレッドボード上のLEDをArduinoからLチカ、今回はタイマー割り込みでLチカを制御する
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

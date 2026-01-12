# Software PWM Live Control

This is a SvelteKit application that demonstrates how to control an Arduino via Web Serial using voice commands processed by Google's Gemini Multimodal Live API.

## Project Overview

The app creates a bridge between your voice and hardware. You can speak naturally to the AI (e.g., "Make the light blink fast", "Set brightness to 50%"), and it will translate your intent into serial commands sent to a connected **Arduino UNO**, specifically controlling its **on-board LED** (usually on pin 13).

## Features

- **Voice Control**: Uses Gemini Live (via WebSocket) for real-time, low-latency voice interaction.
- **Web Serial API**: Connects directly to the Arduino from the browser without needing a local server or bridge.
- **Function Calling**: The AI understands specific tools (`set_blinking`, `set_brightness`, `set_interval`) to control the hardware precisely.
- **Real-time Audio**: Implements a custom AudioWorklet to stream raw PCM audio to the Gemini API.

## Prerequisites

### Hardware
- An Arduino board (Uno, Nano, etc.) with an LED connected to a PWM pin (or using the built-in LED).
- The Arduino must be running a sketch that accepts serial commands like `blink=1`, `brightness=5`, and `interval=100`.

### Software
- **Node.js**: v18 or later.
- **Browser**: Chrome, Edge, or Opera (support for Web Serial API is required).
- **Gemini API Key**: You need an API key from [Google AI Studio](https://aistudio.google.com/) with access to the **Gemini 2.5 Flash Native Audio Preview** model.

## Setup & Usage

1. **Install Dependencies**:
   ```bash
   npm install
   ```

2. **Run the App**:
   ```bash
   npm run dev -- --open
   ```

3. **Connect**:
   - Click "Connect Arduino" and select your device from the port chooser.
   - Enter your Gemini API Key.
   - Click "Start Live" and allow microphone access.

4. **Speak**:
   - Try saying: *"Turn on the blinking light"*
   - *"Make it brighter"*
   - *"Blink slower"*

## Architecture

- **SvelteKit**: Frontend framework.
- **`src/routes/+page.svelte`**: Main UI and logic bridge. Handles Serial I/O and maps Gemini tool calls to serial commands.
- **`src/lib/gemini.js`**: Handles the WebSocket connection to Gemini, audio streaming (Microphone -> AudioWorklet -> PCM -> WebSocket), and audio playback.

## License

MIT

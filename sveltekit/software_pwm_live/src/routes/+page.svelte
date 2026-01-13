<script>
	import { GeminiLiveClient } from "$lib/gemini";
	import { onMount } from "svelte";

	// --------------------------------------------------------------------------
	// Architecture Overview
	// --------------------------------------------------------------------------
	// This component bridges three systems:
	// 1. The Browser UI (Svelte): Handles user input (buttons, sliders).
	// 2. The Serial API (Navigator): Communicates directly with the Arduino via USB.
	// 3. Gemini Live API (WebSocket): Processes voice commands and triggers tools.
	//
	// Data Flow:
	// User Voice -> Gemini -> Tool Call (JSON) -> Svelte Function -> Serial Command (Text) -> Arduino
	// Arduino -> Serial Output (Text) -> Svelte Reader -> UI Display
	// --------------------------------------------------------------------------

	/** @type {any} */
	let port;
	/** @type {ReadableStreamDefaultReader<Uint8Array> | undefined} */
	let reader;
	/** @type {WritableStreamDefaultWriter<Uint8Array> | undefined} */
	let writer;
	let output = $state("");
	/** @type {HTMLPreElement} */
	let outputElement;
	let blinking = $state(false);
	let brightness = $state(5);
	let interval = $state(200);
	let micLevel = $state(0);
	let speakerLevel = $state(0);

	let apiKey = $state("");
	let liveActive = $state(false);
	/** @type {GeminiLiveClient | null} */
	let geminiClient = null;
	let geminiStatus = $state("Disconnected");

	onMount(() => {
		// Restore API key from local storage if available
		const storedKey = localStorage.getItem("gemini_api_key");
		if (storedKey) apiKey = storedKey;
	});

	$effect(() => {
		// Register 'output' as a dependency for this effect.
		// This ensures the effect runs whenever 'output' changes, scrolling the log to the bottom.
		output; // Dependency registration
		if (outputElement) {
			outputElement.scrollTop = outputElement.scrollHeight;
		}
	});

	const connect = async () => {
		try {
			// Request the user to select a serial port
			// @ts-ignore - navigator.serial is not in standard lib yet
			port = await navigator.serial.requestPort();
			// Open the port at 9600 baud rate (standard for Arduino)
			await port.open({ baudRate: 9600 });
			writer = port.writable.getWriter();
			startReading();
			output += "Connected to Arduino\n";
			// Wait for Arduino to reset and then synchronize status
			setTimeout(getStatus, 2000);
		} catch (error) {
			// @ts-ignore
			output += `Error: ${error.message}\n`;
		}
	};

	const startReading = async () => {
		try {
			if (!port || !port.readable) return;
			// Create a reader and lock it to the port.
			// The Serial API requires the readable stream to be "locked" to a reader,
			// preventing other code from reading simultaneously.
			const serialReader = port.readable.getReader();
			// Store reference globally for cleanup/cancellation if needed later.
			reader = serialReader;
			const decoder = new TextDecoder();
			let buffer = "";

			// Constant read loop
			while (true) {
				const { value, done } = await serialReader.read();
				if (done) {
					break;
				}
				// Decode the Uint8Array to a string. { stream: true } keeps the internal state
				// of the decoder if a multi-byte character is split across chunks.
				const chunk = decoder.decode(value, { stream: true });
				output += chunk;
				buffer += chunk;
				// Split by newline to process complete commands
				const lines = buffer.split("\n");
				// Keep the last partial line in the buffer for the next chunk
				buffer = lines.pop() || "";

				lines.forEach(parseStatus);
			}
		} catch (error) {
			// @ts-ignore
			output += `Error reading from serial port: ${error.message}\n`;
		}
	};

	/**
	 * @param {string} command
	 * Sends a command string to the Arduino via the serial port.
	 */
	const sendCommand = async (command) => {
		if (!writer) {
			output += "Not connected to serial\n";
			return;
		}
		// Encode the command string to bytes
		const data = new TextEncoder().encode(command + "\n");
		await writer.write(data);
	};

	// Helpers to send specific commands using current state values
	const updateBlink = () => {
		sendCommand(`blink=${blinking ? 1 : 0}`);
	};

	const updateBrightness = () => {
		// Maps the 0-10 slider value to the 'brightness=X' command expected by the C++ code
		sendCommand(`brightness=${brightness}`);
	};

	const updateInterval = () => {
		sendCommand(`interval=${interval}`);
	};

	const getStatus = () => {
		sendCommand("status");
	};

	const parseStatus = (/** @type {string} */ line) => {
		// Parse the status string returned by the Arduino (e.g., "blink=1,brightness=5,interval=200")
		const match = line.match(/blink=(\d+),brightness=(\d+),interval=(\d+)/);
		if (match) {
			blinking = match[1] === "1";
			brightness = parseInt(match[2], 10);
			interval = parseInt(match[3], 10);
		}
	};

	/**
	 * Toggles the Gemini Live connection.
	 * If connected, it disconnects.
	 * If disconnected, it initializes the client and connects.
	 */
	const toggleLive = async () => {
		if (liveActive) {
			// Disconnect if currently active
			if (geminiClient) geminiClient.disconnect();
			liveActive = false;
			geminiStatus = "Disconnected";
		} else {
			if (!apiKey) {
				alert("Please enter a Gemini API Key");
				return;
			}
			// Save API key for next time
			localStorage.setItem("gemini_api_key", apiKey);

			// Initialize the Gemini Live Client
			geminiClient = new GeminiLiveClient(apiKey, {
				onConnect: () => {
					geminiStatus = "Connected";
					liveActive = true;
				},
				onDisconnect: () => {
					geminiStatus = "Disconnected";
					liveActive = false;
					micLevel = 0;
					speakerLevel = 0;
				},
				onError: (/** @type {any} */ e) => {
					geminiStatus = `Error: ${e.message || e}`;
					liveActive = false;
				},
				onVolume: (/** @type {number} */ input, /** @type {number} */ output) => {
					micLevel = input;
					speakerLevel = output;
				},
				onToolCall: async (name, args) => {
					console.log("Tool called:", name, args);
					output += `[Gemini] Calling ${name} with ${JSON.stringify(args)}\n`;

					// Handle tool calls from Gemini and map them to Arduino commands
					// The return value string is sent back to the model as the tool execution result.
					if (name === "set_blinking") {
						blinking = args.enabled;
						updateBlink();
						return `Blinking set to ${blinking}`;
					} else if (name === "set_brightness") {
						brightness = args.level;
						updateBrightness();
						return `Brightness set to ${brightness}`;
					} else if (name === "set_interval") {
						interval = args.ms;
						updateInterval();
						return `Interval set to ${interval}`;
					} else if (name === "get_status") {
						getStatus();
						return "Status command sent to Arduino. Please check serial output.";
					}
				},
			});

			await geminiClient.connect();
		}
	};
</script>

<h1>Arduino Software PWM Control + Gemini Live</h1>
<p class="subtitle">
	Controls the on-board LED on an Arduino UNO via Serial commands.
</p>

<div class="container">
	<div class="left-panel">
		<!-- Connection Controls -->
		<!-- Main UI Layout -->
		<div class="control-group">
			<button onclick={connect}>Connect Arduino</button>
		</div>

		<div class="control-group gemini-box">
			<h2>Gemini Live</h2>
			<div class="input-row">
				<label>
					API Key:
					<input
						type="password"
						bind:value={apiKey}
						placeholder="Enter Gemini API Key"
					/>
				</label>
			</div>
			<div class="status-row">
				<span>Status: {geminiStatus}</span>
				<button onclick={toggleLive}
					>{liveActive ? "Stop Live" : "Start Live"}</button
				>
			</div>
			<div class="audio-levels">
				<div class="level-row">
					<span class="label">Mic</span>
					<div class="meter">
						<div class="fill" style="width: {Math.min(100, micLevel * 100)}%"></div>
					</div>
				</div>
				<div class="level-row">
					<span class="label">Speaker</span>
					<div class="meter">
						<div class="fill" style="width: {Math.min(100, speakerLevel * 100)}%"></div>
					</div>
				</div>
			</div>
			<p class="hint">
				Try saying: "Turn on blinking", "Set brightness to 5", "Set interval to
				100ms"
			</p>
		</div>

		<!-- Manual Controls (synced with voice commands) -->
		<div class="control-group">
			<label>
				<input
					type="checkbox"
					bind:checked={blinking}
					onchange={updateBlink}
				/>
				Blinking
			</label>
		</div>

		<div class="control-group">
			<label>
				Brightness: {brightness}
				<input
					type="range"
					min="0"
					max="10"
					bind:value={brightness}
					onchange={updateBrightness}
				/>
			</label>
		</div>

		<div class="control-group">
			<label>
				Interval (ms):
				<input type="number" bind:value={interval} onchange={updateInterval} />
			</label>
		</div>

		<div class="control-group">
			<button onclick={getStatus}>Sync Status</button>
		</div>
	</div>

	<div class="right-panel">
		<h2>Serial Output</h2>
		<pre bind:this={outputElement}>{output}</pre>
	</div>
</div>

<style>
	.container {
		display: flex;
		gap: 20px;
	}
	.left-panel {
		flex: 1;
		min-width: 300px;
	}
	.left-panel > :last-child {
		margin-bottom: 0;
	}
	.right-panel {
		flex: 1;
		display: flex;
		flex-direction: column;
	}
	.right-panel h2 {
		margin-top: 0;
		margin-bottom: 0;
	}
	.control-group {
		margin-bottom: 1.5em;
		padding: 10px;
		background: #fff;
		border: 1px solid #ddd;
		border-radius: 8px;
	}
	.gemini-box {
		background: #f0f7ff;
		border-color: #cce5ff;
	}
	.input-row {
		margin-bottom: 10px;
	}
	.status-row {
		display: flex;
		align-items: center;
		gap: 15px;
	}
	h1 {
		font-family: "Inter", sans-serif;
		color: #333;
	}
	input[type="password"] {
		padding: 5px;
		border-radius: 4px;
		border: 1px solid #ccc;
		width: 250px;
	}
	button {
		padding: 8px 16px;
		border-radius: 4px;
		border: none;
		background: #007bff;
		color: white;
		cursor: pointer;
	}
	button:hover {
		background: #0056b3;
	}
	pre {
		background-color: #222;
		color: #0f0;
		padding: 1em;
		border-radius: 5px;
		white-space: pre-wrap;
		word-wrap: break-word;
		overflow-y: auto;
		flex-grow: 1;
		min-height: 0; /* Prevents flexbox overflow issue */
		height: 600px;
	}
	.hint {
		font-size: 0.9em;
		color: #666;
		margin-top: 10px;
		font-style: italic;
	}
	.subtitle {
		text-align: center;
		color: #555;
		margin-bottom: 20px;
	}
	.audio-levels {
		margin-top: 15px;
		display: flex;
		flex-direction: column;
		gap: 8px;
	}
	.level-row {
		display: flex;
		align-items: center;
		gap: 10px;
		font-size: 0.9em;
	}
	.meter {
		flex: 1;
		height: 8px;
		background: #ddd;
		border-radius: 4px;
		overflow: hidden;
	}
	.fill {
		height: 100%;
		background: #28a745;
		transition: width 0.1s ease;
	}
</style>

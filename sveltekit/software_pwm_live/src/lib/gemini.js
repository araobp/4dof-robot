/**
 * @typedef {Object} GeminiCallbacks
 * @property {() => void} [onConnect]
 * @property {() => void} [onDisconnect]
 * @property {(error: Event|Error) => void} [onError]
 * @property {(name: string, args: Record<string, any>) => Promise<any>} [onToolCall]
 * @property {(input: number, output: number) => void} [onVolume]
 */

export class GeminiLiveClient {
    /**
     * @param {string} apiKey
     * @param {GeminiCallbacks} callbacks
     */
    constructor(apiKey, callbacks) {
        this.apiKey = apiKey;
        this.callbacks = callbacks;
        /** @type {WebSocket | null} */
        this.ws = null;
        /** @type {AudioContext | null} */
        this.audioContext = null;
        /** @type {MediaStream | null} */
        this.mediaStream = null;
        /** @type {AudioWorkletNode | null} */
        this.audioProcessor = null;
        /**
         * @type {number}
         * The timestamp (in AudioContext time) when the next scheduled audio chunk should start playing.
         */
        this.nextStartTime = 0;
        this.inputAnalyser = null;
        this.outputAnalyser = null;
        this.volumeInterval = null;
    }

    /**
     * Connects to the Gemini Live WebSocket API.
     * Sets up the WebSocket event handlers.
     */
    async connect() {
        const url = `wss://generativelanguage.googleapis.com/ws/google.ai.generativelanguage.v1alpha.GenerativeService.BidiGenerateContent?key=${this.apiKey}`;
        this.ws = new WebSocket(url);

        this.ws.onopen = () => {
            console.log("Connected to Gemini Live");
            // Send the initial configuration immediately upon connection
            this.sendSetup();
            // Start capturing and streaming audio
            this.startAudioStream();
            if (this.callbacks.onConnect) this.callbacks.onConnect();
        };

        this.ws.onmessage = async (event) => {
            const data = event.data;
            // The API sends responses as Blobs containing JSON
            if (data instanceof Blob) {
                const text = await data.text();
                const msg = JSON.parse(text);
                this.handleMessage(msg);
            } else {
                console.log("Received non-blob message:", data);
            }
        };

        this.ws.onclose = () => {
            console.log("Disconnected from Gemini Live");
            this.stopAudioStream();
            if (this.callbacks.onDisconnect) this.callbacks.onDisconnect();
        };

        this.ws.onerror = (error) => {
            console.error("WebSocket Error:", error);
            if (this.callbacks.onError) this.callbacks.onError(error);
        };
    }

    /**
     * Sends the initial setup message to Gemini.
     * defines the model, generation config, and available tools.
     */
    sendSetup() {
        const setupMsg = {
            setup: {
                model: "models/gemini-2.5-flash-native-audio-preview-12-2025",
                generation_config: {
                    response_modalities: ["AUDIO"], // Request audio output from the model
                    speech_config: {
                        voice_config: {
                            prebuilt_voice_config: {
                                voice_name: "Charon" // Select a specific voice persona
                            }
                        }
                    }
                },
                tools: [
                    {
                        // Define functions that the model can call to control the robot
                        function_declarations: [
                            {
                                name: "set_blinking",
                                description: "Turn blinking on or off",
                                parameters: {
                                    type: "object",
                                    properties: {
                                        enabled: { type: "boolean" }
                                    },
                                    required: ["enabled"]
                                }
                            },
                            {
                                name: "set_brightness",
                                description: "Set the brightness of the LED (0-10)",
                                parameters: {
                                    type: "object",
                                    properties: {
                                        level: { type: "number" }
                                    },
                                    required: ["level"]
                                }
                            },
                            {
                                name: "set_interval",
                                description: "Set the blinking interval in milliseconds",
                                parameters: {
                                    type: "object",
                                    properties: {
                                        ms: { type: "number" }
                                    },
                                    required: ["ms"]
                                }
                            },
                            {
                                name: "get_status",
                                description: "Get the current status of the Arduino",
                                parameters: {
                                    type: "object",
                                    properties: {}
                                }
                            }
                        ]
                    }
                ]
            }
        };
        if (this.ws) {
            this.ws.send(JSON.stringify(setupMsg));
        }
    }

    /**
     * Starts capturing audio from the microphone and sending it to Gemini.
     * Uses an AudioWorklet to process raw PCM audio data off the main thread.
     * Accessing raw PCM is required because Gemini expects raw bytes, not encoded files.
     */
    async startAudioStream() {
        // @ts-ignore - webkitAudioContext is non-standard but needed for Safari/older browsers
        const AudioContextClass = window.AudioContext || window.webkitAudioContext;
        this.audioContext = new AudioContextClass({
            sampleRate: 24000
        });
        
        // Create analysers for volume visualization
        this.inputAnalyser = this.audioContext.createAnalyser();
        this.outputAnalyser = this.audioContext.createAnalyser();
        // Connect output analyser to destination so we can hear the response (and visualize it)
        this.outputAnalyser.connect(this.audioContext.destination);

        try {
            this.mediaStream = await navigator.mediaDevices.getUserMedia({ audio: true });
        } catch (e) {
            console.error("Microphone access denied", e);
            return;
        }

        // Load the worklet code from a string blob.
        // We do this to avoid having to host a separate file for the worklet,
        // which simplifies the project structure and avoids path resolution issues.
        await this.audioContext.audioWorklet.addModule(
            "data:text/javascript;base64," + btoa(this.getAudioWorkletCode())
        );

        const source = this.audioContext.createMediaStreamSource(this.mediaStream);
        this.audioProcessor = new AudioWorkletNode(this.audioContext, "pcm-processor");

        this.audioProcessor.port.onmessage = (event) => {
            const pcmData = event.data; // Int16Array
            if (this.ws && this.ws.readyState === WebSocket.OPEN) {
                // Convert raw PCM to base64 for transport
                const buffer = pcmData.buffer;
                const base64 = this.arrayBufferToBase64(buffer);

                // Wrap in the expected JSON structure
                const msg = {
                    realtime_input: {
                        media_chunks: [
                            {
                                mime_type: "audio/pcm;rate=24000",
                                data: base64
                            }
                        ]
                    }
                };
                this.ws.send(JSON.stringify(msg));
            }
        };

        // Connect the audio graph: Source -> Analyser -> Processor
        // Note: The processor doesn't connect to destination to avoid feedback loop of own voice
        source.connect(this.inputAnalyser);
        source.connect(this.audioProcessor);
        
        this.startVolumeReporting();
    }

    /**
     * Stops the audio stream and closes the audio context.
     * Cleans up media tracks and worklet nodes.
     */
    stopAudioStream() {
        this.stopVolumeReporting();
        // Stop the microphone stream
        if (this.mediaStream) {
            this.mediaStream.getTracks().forEach(track => track.stop());
            this.mediaStream = null;
        }
        // Disconnect the processor
        if (this.audioProcessor) {
            this.audioProcessor.disconnect();
            this.audioProcessor = null;
        }
        // Close the audio context to release hardware resources
        if (this.audioContext) {
            this.audioContext.close();
            this.audioContext = null;
        }
        this.inputAnalyser = null;
        this.outputAnalyser = null;
    }

    /**
     * Starts an interval to calculate and report audio volume levels.
     * Uses the AnalyserNodes created in startAudioStream.
     * Reports volume every 50ms.
     */
    startVolumeReporting() {
        if (this.volumeInterval) clearInterval(this.volumeInterval);
        this.volumeInterval = setInterval(() => {
            if (this.callbacks.onVolume) {
                // Helper to calculate Root Mean Square (RMS) amplitude
                const getRMS = (/** @type {AnalyserNode | null} */ analyser) => {
                    if (!analyser) return 0;
                    const bufferLength = analyser.fftSize;
                    const dataArray = new Uint8Array(bufferLength);
                    analyser.getByteTimeDomainData(dataArray);
                    let sum = 0;
                    for (let i = 0; i < bufferLength; i++) {
                        // Convert 0-255 range to -1 to 1
                        const x = (dataArray[i] - 128) / 128.0;
                        sum += x * x;
                    }
                    return Math.sqrt(sum / bufferLength);
                };
                // Scale up a bit for better visibility
                const inputVol = Math.min(1, getRMS(this.inputAnalyser) * 2);
                const outputVol = Math.min(1, getRMS(this.outputAnalyser) * 2);
                this.callbacks.onVolume(inputVol, outputVol);
            }
        }, 50);
    }

    /**
     * Stops the volume reporting interval.
     */
    stopVolumeReporting() {
        if (this.volumeInterval) clearInterval(this.volumeInterval);
        this.volumeInterval = null;
    }

    /**
     * Returns the code for the AudioWorkletProcessor as a string.
     * This processor buffers audio input and converts it to PCM Int16 format.
     * 
     * Why Int16? 
     * The Web Audio API works in Float32 (values -1.0 to 1.0).
     * Many speech APIs (including Gemini) expect 16-bit PCM (values -32768 to 32767).
     * We perform this conversion in the worklet to offload CPU work from the main UI thread.
     * 
     * @returns {string}
     */
    getAudioWorkletCode() {
        return `
			class PCMProcessor extends AudioWorkletProcessor {
				constructor() {
					super();
					this.bufferSize = 2048;
					this.buffer = new Float32Array(this.bufferSize);
					this.bufferIndex = 0;
				}

				process(inputs, outputs, parameters) {
					const input = inputs[0];
					if (input.length > 0) {
						const inputChannel = input[0];
						// Buffer incoming audio data
						for (let i = 0; i < inputChannel.length; i++) {
							this.buffer[this.bufferIndex++] = inputChannel[i];
							if (this.bufferIndex === this.bufferSize) {
								this.flush();
							}
						}
					}
					return true; // Keep processor alive
				}

				flush() {
					// Convert Float32 (-1.0 to 1.0) to Int16 (-32768 to 32767)
					const pcmData = new Int16Array(this.bufferSize);
					for (let i = 0; i < this.bufferSize; i++) {
						const s = Math.max(-1, Math.min(1, this.buffer[i]));
						pcmData[i] = s < 0 ? s * 0x8000 : s * 0x7FFF;
					}
					// Send data back to main thread
					this.port.postMessage(pcmData);
					this.bufferIndex = 0;
				}
			}
			registerProcessor("pcm-processor", PCMProcessor);
		`;
    }

    /**
     * @param {ArrayBufferLike} buffer
     * @returns {string}
     */
    arrayBufferToBase64(buffer) {
        let binary = '';
        const bytes = new Uint8Array(buffer);
        const len = bytes.byteLength;
        // Convert byte array to binary string for btoa
        for (let i = 0; i < len; i++) {
            binary += String.fromCharCode(bytes[i]);
        }
        return window.btoa(binary);
    }

    /**
     * Handles incoming messages from the WebSocket.
     * Processes server content (audio) and tool calls.
     * @param {any} msg
     */
    handleMessage(msg) {
        // Handle audio content from the model
        if (msg.serverContent) {
            if (msg.serverContent.modelTurn) {
                const parts = msg.serverContent.modelTurn.parts;
                for (const part of parts) {
                    if (part.inlineData && part.inlineData.mimeType.startsWith("audio/pcm")) {
                        this.playAudio(part.inlineData.data);
                    }
                }
            }
        } else if (msg.toolCall) {
            // Handle function calls requested by the model
            this.handleToolCall(msg.toolCall);
        }
    }

    /**
     * Handles tool calls requested by the model.
     * Executes the registered callback for the tool and sends the response back.
     * @param {any} toolCall
     */
    async handleToolCall(toolCall) {
        const functionCalls = toolCall.functionCalls;
        const toolResponses = [];

        for (const call of functionCalls) {
            console.log("Tool call:", call.name, call.args);
            let result = {};

            if (this.callbacks.onToolCall) {
                // Execute the client-side logic for this tool
                result = await this.callbacks.onToolCall(call.name, call.args);
            }

            // Must send a generic response for Void functions to satisfy the API
            if (result === undefined) result = { result: "ok" };

            // Accumulate responses
            toolResponses.push({
                id: call.id,
                name: call.name,
                response: {
                    result: result
                }
            });
        }

        // Send all tool responses back to the model so it can generate a follow-up
        const responseMsg = {
            tool_response: {
                function_responses: toolResponses
            }
        };
        if (this.ws) this.ws.send(JSON.stringify(responseMsg));
    }

    /**
     * Queues received audio data for playback.
     * Converts base64 PCM data to Float32 for the AudioContext.
     * 
     * AudioContext expects Float32 (-1.0 to 1.0), but we receive PCM Int16 from Gemini.
     * We must decode the base64, interpret as Int16, and then normalize to Float32.
     * 
     * @param {string} base64Data
     */
    playAudio(base64Data) {
        if (!this.audioContext) return;

        // Decode Base64 to binary string
        const binaryString = window.atob(base64Data);
        const len = binaryString.length;
        const bytes = new Uint8Array(len);
        for (let i = 0; i < len; i++) {
            bytes[i] = binaryString.charCodeAt(i);
        }
        // Interpret as Int16 PCM
        const pcm16 = new Int16Array(bytes.buffer);
        // Convert to Float32 for Web Audio API
        const float32 = new Float32Array(pcm16.length);
        for (let i = 0; i < pcm16.length; i++) {
            float32[i] = pcm16[i] / 32768.0;
        }

        // Create an audio buffer
        const buffer = this.audioContext.createBuffer(1, float32.length, 24000);
        buffer.copyToChannel(float32, 0);

        const source = this.audioContext.createBufferSource();
        source.buffer = buffer;

        // Connect to output analyser (for visualization) and destination (speakers)
        // Note: In startAudioStream, outputAnalyser is already connected to destination.
        if (this.outputAnalyser) {
            source.connect(this.outputAnalyser);
        } else {
            source.connect(this.audioContext.destination);
        }

        // Schedule the audio to play at the correct time
        // If the scheduled time is in the past (e.g. latency), play immediately
        const startTime = Math.max(this.audioContext.currentTime, this.nextStartTime);
        source.start(startTime);

        // Update the next start time to be the end of this current buffer
        this.nextStartTime = startTime + buffer.duration;
    }

    /**
     * Disconnects the WebSocket and stops audio streaming.
     */
    disconnect() {
        if (this.ws) {
            this.ws.close();
            this.ws = null;
        }
        this.stopAudioStream();
    }
}

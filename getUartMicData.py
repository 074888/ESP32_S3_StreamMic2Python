import serial
import wave
import time

COM_PORT = 'COM8'
BAUD_RATE = 921600

SAMPLE_RATE = 16000
CHANNELS = 2
BITS_PER_SAMPLE = 16
DURATION = 5

OUTPUT_FILE = 'PDM_Mic2.wav'

ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)

time.sleep(1)              # 等 ESP32 上电/复位后稳定
ser.reset_input_buffer()   # 清掉启动阶段残留数据

total_bytes = SAMPLE_RATE * CHANNELS * (BITS_PER_SAMPLE // 8) * DURATION
received_data = bytearray()

print("开始录音...")
start_time = time.time()

while len(received_data) < total_bytes:
    chunk = ser.read(min(2048, total_bytes - len(received_data)))
    if chunk:
        received_data.extend(chunk)
        progress = len(received_data) / total_bytes * 100
        print(f"\r进度: {progress:.1f}%", end="")

print(f"\n接收完成，用时: {time.time() - start_time:.2f} 秒")

frame_size = CHANNELS * (BITS_PER_SAMPLE // 8)
valid_len = len(received_data) // frame_size * frame_size
received_data = received_data[:valid_len]

with wave.open(OUTPUT_FILE, 'wb') as wf:
    wf.setnchannels(CHANNELS)
    wf.setsampwidth(BITS_PER_SAMPLE // 8)
    wf.setframerate(SAMPLE_RATE)
    wf.writeframes(received_data)

ser.close()

print("WAV 保存成功：", OUTPUT_FILE)

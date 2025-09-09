import serial
import paho.mqtt.client as mqtt

SERIAL_PORT = "/dev/cu.usbmodem1101"
BAUDRATE = 115200

BROKER = "localhost"
PORT = 1883
TOPIC = "rpm"


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("MQTT connect success.")
    else:
        print(f"MQTT connect failed. code: {rc}")


client = mqtt.Client()
client.on_connect = on_connect
client.connect(BROKER, PORT, 60)

ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)

try:
    while True:
        line = ser.readline().decode("utf-8").strip()
        if line:
            print(f"Publish: {line}")
            client.publish(TOPIC, line)
        client.loop(0.01)

except KeyboardInterrupt:
    print("exit")

finally:
    ser.close()
    client.disconnect()

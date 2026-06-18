import socket
import struct
import time
import serial
import threading

import argparse

import matplotlib.pyplot as plt


ESP32_IP = "192.168.4.1"
PORT = 1234

BAUDRATE = 115200

TEST_DURATION_S = 20
SIGNAL_DURATION = 10
TIMEOUT_DURATION_S = 5



timestamp_rx = []
motor_rx = []
servo_rx = []

timestamp_tx = []
motor_tx = []
servo_tx = []

data_lock = threading.Lock()
stop_event = threading.Event()

def plot_result():
    _, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8), sharex=True)

    ax1.step(timestamp_tx, motor_tx, 'o', label='Control signal (UDP)', where='post', color='blue', alpha=0.5)
    ax1.plot(timestamp_rx, motor_rx, 'o-', label='Received control signal (UART)', color='red', markersize=3)
    ax1.set_ylabel('Motor Duty [us]')
    ax1.set_xlabel('Time [s]')
    ax1.legend()
    ax1.grid(True)

    ax2.step(timestamp_tx, servo_tx, 'o', label='Control signal (UDP)', where='post', color='green', alpha=0.5)
    ax2.plot(timestamp_rx, servo_rx, 'o-', label='Received control signal (UART)', color='orange', markersize=3)
    ax2.set_ylabel('Servo Duty [us]')
    ax2.set_xlabel('Time [s]')
    ax2.legend()
    ax2.grid(True)

    plt.suptitle("Hardware in The Loop Test")
    plt.show()

def send_command(socket: socket.socket, motor_duty: int, servo_duty: int):
    msg = struct.pack('<II', motor_duty, servo_duty)
    socket.sendto(msg, (ESP32_IP, PORT))

def serial_comm(port: str, start_time: float):
    try:
        srl = serial.Serial()
        srl.port = port
        srl.baudrate = BAUDRATE
        srl.timeout = 0.5
        srl.dtr = False
        srl.rts = False
        srl.open()

        print(f"Connected to the {port}")

        while not stop_event.is_set():
            line = srl.readline()
            if line:
                data = line.decode('utf-8', errors='ignore').strip()
                if not data.startswith("HIL:"):
                    continue

                try:
                    _, m, s = data.split(':')
                    with data_lock:
                        timestamp_rx.append(time.time() - start_time)
                        motor_rx.append(int(m))
                        servo_rx.append(int(s))
                except ValueError:
                    continue
                    

    except serial.SerialException as e:
        print(f"Serial port exception {e}")


def main():
    parser = argparse.ArgumentParser(prog="esp32_fw_hil", 
                                     description="HIL test for RC FPV car")
    parser.add_argument("port", help="Com connectio port for HIL", default="COM4")
    args = parser.parse_args()

    start_time = time.time()
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


    comm_reader = threading.Thread(target=serial_comm, args=(args.port, start_time))
    comm_reader.start()

    try:
        print("Begin test procedure")
        
        while(time.time() - start_time < TEST_DURATION_S):
            current_time = time.time() - start_time

            target_motor = 5000 + int(3000 * (current_time % 2 - 1))
            target_servo = 5000 + (1000 if (current_time % 2) < 1 else -1000)

            # TIMEOUT TEST
            if SIGNAL_DURATION < current_time < SIGNAL_DURATION + TIMEOUT_DURATION_S:
                continue    
                
            send_command(socket=sock, motor_duty=target_motor, servo_duty=target_servo)

            with data_lock:
                timestamp_tx.append(current_time)
                motor_tx.append(target_motor)
                servo_tx.append(target_servo)
            
            time.sleep(0.05)

    finally:
        print("HIL test ended")
        stop_event.set() 
        comm_reader.join(timeout=2)
        sock.close()

    print("Rendering plot")
    plot_result()


if __name__ == '__main__':
    main()
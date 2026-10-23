import socket, time

UDP_PORT = 4210
PING_MSG = b"AMVISH_PING"
PONG_MSG = b"AMVISH_PONG"

def discover_esp32(timeout=10):
    print("[DISCOVERY] Searching for ESP32...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.settimeout(1)

    start = time.time()
    while time.time() - start < timeout:
        sock.sendto(PING_MSG, ("255.255.255.255", UDP_PORT))
        try:
            data, addr = sock.recvfrom(1024)
            if data == PONG_MSG:
                print(f"[DISCOVERY] ESP32 found at {addr[0]}")
                sock.close()
                return addr[0]
        except socket.timeout:
            pass

    sock.close()
    print("[DISCOVERY] ESP32 not found")
    return None

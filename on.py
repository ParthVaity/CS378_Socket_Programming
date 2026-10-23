import select
import socket
import struct
import sys
import time

ON_PORT     = 5000
MAXN        = 26
CONNECT_LEN = 6
TICK        = 10.0
POLL        = 1.0

class VN:
    def __init__(self, sock, addr):
        self.sock = sock
        self.addr = addr


def read_file(path):
    try:
        with open(path) as f:
            return f.read()
    except OSError:
        return ""


def parse_config(text):
    rows = []
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        try:
            rows.append([int(x) for x in line.split()])
        except ValueError:
            return None
    n = len(rows)
    if n == 0 or n + 1 > MAXN:
        return None
    for i, row in enumerate(rows):
        if len(row) != n - i:
            return None
        if any(c == 0 or c < -1 for c in row):
            return None
    n = n + 1
    cost = [[-1] * n for _ in range (n)]
    for i in range (n):
        cost [i][i] = 0
    for i, row in enumerate(rows):
        for k,c in enumerate(row):
            j = i + 1 + k
            cost[i][j] = cost[j][i] = c
    return n,cost


def tuple_bytes(i, addr, cost):
    return bytes([65 + i]) + addr + struct.pack("!I", cost)

def live(vns):
    return sum(1 for v in vns if v.sock is not None)


def send_link_state(i, n, cost, vns):
    if vns[i].sock is None:
        return
    if i < n:
        tuples = [tuple_bytes(j, vns[j].addr, cost[i][j]) for j in range(n) if cost[i][j] >= 0]
    else:
        tuples = [tuple_bytes(i, vns[i].addr, 0)]
    try:
        vns[i].sock.sendall(bytes([len(tuples)]) + b"".join(tuples))
    except OSError:
        print(f"Sending to {chr(65+i)} failed")

def broadcast_link_state(n, cost, vns):
    for i in range(len(vns)):
        send_link_state(i, n, cost, vns)
    print("LINK-STATE pushed", flush=True)


def print_graph(n, cost, vns, reason):
    current_conns = live(vns)
    print(f"Current Graph ({reason}): {n} nodes, {current_conns} connected")
    print("\t"+"\t".join(chr(65+i) for i in range(n)))
    for i in range(n):
        print(chr(65+i) + "\t" + "\t".join(str(c) for c in cost[i]))
    print(flush=True)


def main():
    if len(sys.argv) != 2:
        print("Usage: python3 on.py <config-file>")
        return 1
    path = sys.argv[1]

    last_text = read_file(path)
    parsed_file = parse_config(last_text)
    if parsed_file is None:
        print("Bad Config File")
        return 1
    n, cost = parsed_file

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR,1)
    srv.bind(("", ON_PORT))
    srv.listen(32)
    print(f"ON Listening on TCP {ON_PORT}, expecting {n} VN nodes", flush=True)

    vns = []
    ready = False
    last_tick = time.monotonic()

    while True:
        watch = [srv] + [v.sock for v in vns if v.sock is not None]
        timeout = min(POLL, max(0.0, last_tick + TICK - time.monotonic()))
        read, _, _ = select.select(watch, [], [], timeout)

        if srv in read:
            connection, _ = srv.accept()
            addr = None
            if len(vns) < MAXN :
                connection.settimeout(2)
                try:
                    data = b""
                    while len(data) < CONNECT_LEN:
                        current_chunk = connection.recv(CONNECT_LEN - len(data))
                        if not current_chunk:
                            break
                        data += current_chunk
                    if len(data) == CONNECT_LEN:
                        addr = data
                except OSError:
                    pass

            if addr is None:
                print("Connection rejected")
                connection.close()
            else:
                connection.settimeout(None)
                index = next((k for k,v in enumerate(vns) if v.sock is None), len(vns))
                if index == len(vns):
                    vns.append(VN(connection, addr))
                else:
                    vns[index] = VN(connection, addr)
                ip = socket.inet_ntoa(addr[:4])
                port = struct.unpack("!H", addr[4:])[0]
                print(f"CONNECT from node {chr(65+index)} {ip}:{port}")
                print(f"{live(vns)} connected", flush=True)

                if not ready :
                    if len(vns) >= n and all(v.sock is not None for v in vns[:n]): 
                        ready = True
                        broadcast_link_state(n, cost, vns)
                elif index < n:
                    broadcast_link_state(n, cost, vns)
                else:
                    send_link_state(index, n, cost, vns)
        for i, vn in enumerate(vns):
            if vn.sock is not None and vn.sock in read:
                try:
                    data = vn.sock.recv(1)
                except OSError:
                    data = b""
                if not data:
                    print(f"Node {chr(65+i)} disconnected")
                    print(f"{live(vns)} connected", flush= True)
                    vn.sock.close()
                    vn.sock = None
        current_text = read_file(path)
        if current_text and current_text != last_text:
            last_text = current_text
            print("Config Changed", flush= True)
            parsed_file = parse_config(current_text)
            if parsed_file is None or parsed_file[0] != n:
                print("Bad config/Node count changed, ignoring file", flush= True)
            else:
                cost = parsed_file[1]
                print_graph(n, cost, vns, "Config Changed")
                if ready:
                    broadcast_link_state(n, cost, vns)
                else:
                    print(f"Waiting for {n - live(vns)} more VN(s)", flush= True)

        if time.monotonic() - last_tick >= TICK:
            last_tick = time.monotonic()
            print_graph(n, cost, vns, "Tick")

if __name__=="__main__":
    sys.exit(main())
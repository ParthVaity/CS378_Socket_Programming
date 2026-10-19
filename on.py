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
        if not line or line.startswith("#"):
            continue
        try:
            rows.append([int(x) for x in line.split()])
        except ValueError:
            return None
    m = len(rows)
    if m == 0 or m + 1 > MAXN:
        return None
    for i, row in enumerate(rows):
        if len(row) != m - i:
            return None
        if any(c == 0 or c < -1 for c in row):
            return None
    n = m + 1
    cost = [[-1] * n for _ in range(n)]
    for i in range(n):
        cost[i][i] = 0
    for i, row in enumerate(rows):
        for k, c in enumerate(row):
            j = i + 1 + k
            cost[i][j] = cost[j][i] = c
    return n, cost


def tuple_bytes(j, addr, cost):
    return bytes([ord("A") + j]) + addr + struct.pack("!I", cost)


def send_link_state(i, n, cost, vns):
    if vns[i].sock is None:
        return
    if i < n:
        tuples = [tuple_bytes(j, vns[j].addr, cost[i][j])
                  for j in range(n) if cost[i][j] >= 0]
    else:
        tuples = [tuple_bytes(i, vns[i].addr, 0)]
    try:
        vns[i].sock.sendall(bytes([len(tuples)]) + b"".join(tuples))
    except OSError:
        print(f"sending to {chr(65 + i)} failed")


def broadcast_link_state(n, cost, vns):
    for i in range(len(vns)):
        send_link_state(i, n, cost, vns)
    print("LINK-STATE pushed", flush=True)


def print_graph(n, cost, vns, why):
    print(f"--- ON graph ({why}): {n} nodes, {len(vns)} connected ---")
    for i in range(n - 1):
        print("\t" * (i + 1) + "\t".join(str(c) for c in cost[i][i + 1:]))
    print(flush=True)


def main():
    if len(sys.argv) != 2:
        print("usage: python3 on.py <config-file>")
        return 1
    path = sys.argv[1]

    last_text = read_file(path)
    parsed = parse_config(last_text)
    if parsed is None:
        print("bad config file")
        return 1
    n, cost = parsed

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("", ON_PORT))
    srv.listen(32)
    print(f"ON listening on TCP {ON_PORT}, expecting {n} nodes", flush=True)

    vns = []
    ready = False
    last_tick = time.monotonic()

    while True:
        watch = [srv] + [v.sock for v in vns if v.sock is not None]
        timeout = min(POLL, max(0.0, last_tick + TICK - time.monotonic()))
        readable, _, _ = select.select(watch, [], [], timeout)

        if srv in readable:
            conn, _ = srv.accept()
            addr = None
            if len(vns) < MAXN:
                conn.settimeout(2)
                try:
                    data = b""
                    while len(data) < CONNECT_LEN:
                        chunk = conn.recv(CONNECT_LEN - len(data))
                        if not chunk:
                            break
                        data += chunk
                    if len(data) == CONNECT_LEN:
                        addr = data
                except OSError:                        
                    pass

            if addr is None:
                print("rejected a connection")
                conn.close()
            else:
                conn.settimeout(None)
                vns.append(VN(conn, addr))
                idx = len(vns) - 1
                ip = socket.inet_ntoa(addr[:4])
                port = struct.unpack("!H", addr[4:])[0]
                print(f"CONNECT from node {chr(65 + idx)} {ip}:{port}", flush=True)

                if not ready and len(vns) >= n: 
                    ready = True
                    broadcast_link_state(n, cost, vns)
                elif ready:
                    send_link_state(idx, n, cost, vns)

        for i, vn in enumerate(vns):
            if vn.sock is not None and vn.sock in readable:
                try:
                    data = vn.sock.recv(1)
                except OSError:
                    data = b""
                if not data:
                    print(f"node {chr(65 + i)} disconnected", flush=True)
                    vn.sock.close()
                    vn.sock = None

        cur = read_file(path)
        if cur and cur != last_text:
            last_text = cur
            print("config changed", flush=True)
            parsed = parse_config(cur)
            if parsed is None or parsed[0] != n:
                print("bad config (or node count changed), ignored", flush=True)
            else:
                cost = parsed[1]
                print_graph(n, cost, vns, "config changed")
                if ready:
                    broadcast_link_state(n, cost, vns)
                else:
                    print(f"waiting for {n - len(vns)} more VN(s)", flush=True)

        if time.monotonic() - last_tick >= TICK:
            last_tick = time.monotonic()
            print_graph(n, cost, vns, "tick")


if __name__ == "__main__":
    sys.exit(main())
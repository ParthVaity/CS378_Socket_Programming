# Helper script to run the ON and VNs in separate terminals. Usage:
#   ./run.sh [N] [config] [IP]

set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_PORT=5000

if [ "$1" = "stop" ]; then
    pkill -f "python3 on.py" 2>/dev/null || true
    pkill -f "$ROOT/vn/vn " 2>/dev/null || true
    echo "stopped ON and VNs"
    exit 0
fi

CONFIG="${2:-oracle-node-config.txt}"
IP="${3:-$(hostname -I | awk '{print $1}')}"

if [ -n "$1" ]; then
    N="$1"
else
    N=$(( $(grep -v '^\s*#' "$ROOT/$CONFIG" | grep -c '[^[:space:]]') + 1 ))
fi

make -C "$ROOT/vn"

spawn() {
    local title="$1" cmd="$2"
    if command -v gnome-terminal >/dev/null; then
        gnome-terminal --title="$title" -- bash -c "$cmd; exec bash"
    elif command -v konsole >/dev/null; then
        konsole -p tabtitle="$title" -e bash -c "$cmd; exec bash" &
    elif command -v xterm >/dev/null; then
        xterm -T "$title" -e bash -c "$cmd; exec bash" &
    else
        echo "No supported terminal (gnome-terminal, konsole, xterm)"; exit 1
    fi
}

echo "IP=$IP  nodes=$N  config=$CONFIG"
Q_ROOT=$(printf '%q' "$ROOT")

spawn "ON" "cd $Q_ROOT && python3 on.py $CONFIG"
sleep 1

for ((i = 0; i < N; i++)); do
    letter=$(printf "\\$(printf '%03o' $((65 + i)))")
    spawn "VN $letter" "cd $Q_ROOT/vn && ./vn $IP $IP $((BASE_PORT + i))"
    sleep 1
done
# Helper script for spawning ONs and VNs on separate terminals

CONFIG=${1:-oracle-node-config.txt}
IP=${2:-$(hostname -I | awk '{print $1}')}
N=$(( $(grep -v '^\s*#' "$CONFIG" | grep -c '[^[:space:]]') +1 ))

make -C vn || exit 1

gnome-terminal --title=ON -- bash -c "python3 on.py $CONFIG; exec bash"
sleep 1

for ((i=0;i<N;i++)) do
    letter=$(printf "\\$(printf '%03o' $((65+i)))")
    gnome-terminal --title="VN $letter" -- bash -c "cd vn && ./vn $IP $IP $((5000+i)); exec bash"
done
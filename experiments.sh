
#!/bin/bash
# This script runs a series of real-time application tests with stress-ng to simulate system load.
# Usage: ./experiments.sh
# Make sure to run this script with sudo privileges to allow real-time scheduling.

# 1. Run your real-time application without system stress 
# This section runs the real-time application without any system stress to establish a baseline performance.

sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s other -n 0 -o other_n0.txt
sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s other -n -19 -o other_n-19.txt
sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s fifo -r 50 -o fifo_p50.txt
sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s fifo -r 99 -o fifo_p99.txt
sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s rr -r 50 -o rr_p50.txt
sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s rr -r 99 -o rr_p99.txt
sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s deadline -R 5000 -D 10000 -o deadline_d10k.txt
sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s deadline -R 7000 -D 10000 -o deadline_d10k_r7k.txt

# 2. Run your real-time application with system stress
# Ensure stress-ng is installed: sudo apt install stress-ng


# 2.1. Start stress-ng in the background
# This script runs stress-ng to create a system load and then executes the real-time application tests.

stress-ng \
  --cpu 4 --cpu-method matrixprod \
  --vm 2 --vm-bytes 75% --vm-method all --verify\
  --hdd 1 --hdd-bytes 1G \
  --pipe 4 --mq 2 --sock 2 \
  --sched 2 \
  --timeout 3000s
 & STRESS_PID=$!

# 2. Wait a moment for stress-ng to warm up
sleep 2

# 3. Run the FCS real-time application
sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s other -n 0 -o other_n0_stress.txt
sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s other -n -19 -o other_n-19_stress.txt
sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s fifo -r 50 -o fifo_p50_stress.txt
sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s fifo -r 99 -o fifo_p99_stress.txt
sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s rr -r 50 -o rr_p50_stress.txt
sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s rr -r 99 -o rr_p99_stress.txt
sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s deadline -R 5000 -D 10000 -o deadline_d10k_stress.txt
sudo ./rt_app -p 10000 -c 10000 -j 10000 -a 0 -m -s deadline -R 7000 -D 10000 -o deadline_d10k_r7k_stress.txt


# 4. Stop stress-ng
sudo kill $STRESS_PID
wait $STRESS_PID # Wait for stress-ng to terminate




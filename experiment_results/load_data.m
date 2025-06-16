% read data

% read all data from experiments schedulers:
other_no_stress_n0 =  readmatrix('other_n0_no_stress.txt');
other_stress_n0 =  readmatrix('other_n0_stress.txt');

other_no_stress_n19 =  readmatrix('other_n-19_no_stress.txt');
other_stress_n19 =  readmatrix('other_n-19_stress.txt');

rr_no_stress_p50 =  readmatrix('rr_p50_no_stress.txt');
rr_stress_p50 =  readmatrix('rr_p50_stress.txt');

rr_no_stress_p99 =  readmatrix('rr_p99_no_stress.txt');
rr_stress_p99 =  readmatrix('rr_p99_stress.txt');

fifo_no_stress_p50 =  readmatrix('fifo_p50_no_stress.txt');
fifo_stress_p50 =  readmatrix('fifo_p50_stress.txt');

fifo_no_stress_p99 =  readmatrix('fifo_p99_no_stress.txt');
fifo_stress_p99 =  readmatrix('fifo_p99_stress.txt');

deadline_r400_d4000_no_stress =  readmatrix('deadline_R400_D4000_no_stress.txt');
deadline_r400_d4000_stress =  readmatrix('deadline_R400_D4000_stress.txt');

deadline_r800_d4000_no_stress =  readmatrix('deadline_R800_D4000_no_stress.txt');
deadline_r800_d4000_stress =  readmatrix('deadline_R800_D4000_stress.txt');

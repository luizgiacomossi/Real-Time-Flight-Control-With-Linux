% Read .txt files

data_stress = readmatrix('other_n-19_stress.txt');
data_no_stress = readmatrix('other_n-19_no_stress.txt');
data_0_stress = readmatrix('other_n0_stress.txt');
data_0_no_stress = readmatrix('other_n0_no_stress.txt');

data_stress = readmatrix('rr_p99_stress.txt');
data_no_stress = readmatrix('rr_p99_no_stress.txt');
data_0_stress = readmatrix('rr_p50_stress.txt');
data_0_no_stress = readmatrix('rr_p50_no_stress.txt');

data_stress = readmatrix('fifo_p99_stress.txt');
data_no_stress = readmatrix('fifo_p99_no_stress.txt');
data_0_stress = readmatrix('fifo_p50_stress.txt');
data_0_no_stress = readmatrix('fifo_p50_no_stress.txt');

data_stress = readmatrix('deadline_R800_D4000_stress.txt');
data_no_stress = readmatrix('deadline_R800_D4000_no_stress.txt');
data_0_stress = readmatrix('deadline_R400_D4000_stress.txt');
data_0_no_stress = readmatrix('deadline_R400_D4000_no_stress.txt');


data_stress = readmatrix('fifo_p99_stress.txt');
data_no_stress = readmatrix('fifo_p99_no_stress.txt');
data_0_stress = readmatrix('fifo_p50_stress.txt');
data_0_no_stress = readmatrix('fifo_p50_no_stress.txt');

% read data

% read all data from experiments schedulers:
%other_no_stress_n0 =  readmatrix('other_n0_no_stress.txt');
%other_stress_n0 =  readmatrix('other_n0_stress.txt');

%other_no_stress_n19 =  readmatrix('other_n-19_no_stress.txt');
%other_stress_n19 =  readmatrix('other_n-19_stress.txt');

%rr_no_stress_p50 =  readmatrix('rr_p50_no_stress.txt');
%rr_stress_p50 =  readmatrix('rr_p50_stress.txt');

%rr_no_stress_p99 =  readmatrix('rr_p99_no_stress.txt');
%rr_stress_p99 =  readmatrix('rr_p99_stress.txt');

%fifo_no_stress_p50 =  readmatrix('fifo_p50_no_stress.txt');
%fifo_stress_p50 =  readmatrix('fifo_p50_stress.txt');

%fifo_no_stress_p99 =  readmatrix('fifo_p99_no_stress.txt');
%fifo_stress_p99 =  readmatrix('fifo_p99_stress.txt');

%deadline_r400_d4000_no_stress =  readmatrix('deadline_R400_D4000_no_stress.txt');
%deadline_r400_d4000_stress =  readmatrix('deadline_R400_D4000_stress.txt');

%deadline_r800_d4000_no_stress =  readmatrix('deadline_R800_D4000_no_stress.txt');
%deadline_r800_d4000_stress =  readmatrix('deadline_R800_D4000_stress.txt');



% Create figure and set size
fig = figure('Position', [100, 100, 800, 600]);

fontName = 'latex';
fontSize = 18;

schedulerName = 'SCHED\_FIFO';

%% -------------- plotting for LOW priority --------------
subplot(2,1,1);
plot(data_0_stress(:, 1), 'r', 'LineWidth', 1);
hold on;
plot(data_0_no_stress(:, 1), 'b', 'LineWidth', 1);
grid on;
ylabel('Latency ($\mu$s)', 'Interpreter', 'latex', 'FontSize', fontSize);
xlabel('Iteration', 'Interpreter', 'latex', 'FontSize', fontSize);
myTitle = sprintf('Latency for Scheduler: %s p50 ', schedulerName);
title(myTitle, 'Interpreter', 'latex', 'FontSize', fontSize);
legend({'Stress', 'No Stress'}, 'Interpreter', 'latex', 'FontSize', fontSize, 'Location', 'best');
set(gca, 'FontName', fontName, 'FontSize', fontSize);
hold off;

%% -------------- plotting for HIGH priority --------------
subplot(2,1,2);
set(gca, 'FontName', fontName, 'FontSize', fontSize);

plot(data_stress(:, 1), 'r', 'LineWidth', 1);
hold on;
plot(data_no_stress(:, 1), 'b', 'LineWidth', 1);
grid on;
ylabel('Latency ($\mu$s)', 'Interpreter', 'latex', 'FontSize', fontSize);
xlabel('Iteration', 'Interpreter', 'latex', 'FontSize', fontSize);
myTitle = sprintf('Latency for Scheduler: %s p99 ', schedulerName);
title(myTitle, 'Interpreter', 'latex', 'FontSize', fontSize);
legend({'Stress', 'No Stress'}, 'Interpreter', 'latex', 'FontSize', fontSize, 'Location', 'best');
set(gca, 'FontName', fontName, 'FontSize', fontSize);
hold off;

% Export figure as PDF
exportgraphics(fig, 'latency_comparison.pdf', 'ContentType', 'vector');

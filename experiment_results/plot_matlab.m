% Read .txt files
data_stress = readmatrix('fifo_p99_stress.txt');
data_no_stress = readmatrix('fifo_p99_no_stress.txt');
data_0_stress = readmatrix('fifo_p50_stress.txt');
data_0_no_stress = readmatrix('fifo_p50_no_stress.txt');

data_stress = readmatrix('deadline_R800_D4000_stress.txt');
data_no_stress = readmatrix('deadline_R800_D4000_no_stress.txt');
data_0_stress = readmatrix('deadline_R400_D4000_stress.txt');
data_0_no_stress = readmatrix('deadline_R400_D4000_no_stress.txt');



% Create figure and set size
fig = figure('Position', [100, 100, 800, 600]);

fontName = 'latex';
fontSize = 18;

schedulerName = 'SCHED\_DEADLINE';

%% -------------- plotting for LOW priority --------------
subplot(2,1,1);
plot(data_0_stress(:, 1), 'r', 'LineWidth', 1);
hold on;
plot(data_0_no_stress(:, 1), 'b', 'LineWidth', 1);
grid on;
ylabel('Latency ($\mu$s)', 'Interpreter', 'latex', 'FontSize', fontSize);
xlabel('Iteration', 'Interpreter', 'latex', 'FontSize', fontSize);
myTitle = sprintf('Latency for Scheduler: %s R400 D4000 ', schedulerName);
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
myTitle = sprintf('Latency for Scheduler: %s R800 D4000 ', schedulerName);
title(myTitle, 'Interpreter', 'latex', 'FontSize', fontSize);
legend({'Stress', 'No Stress'}, 'Interpreter', 'latex', 'FontSize', fontSize, 'Location', 'best');
set(gca, 'FontName', fontName, 'FontSize', fontSize);
hold off;

% Export figure as PDF
exportgraphics(fig, 'latency_comparison.pdf', 'ContentType', 'vector');

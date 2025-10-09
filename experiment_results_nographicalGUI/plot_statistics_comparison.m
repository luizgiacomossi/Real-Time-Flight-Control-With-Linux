% --- Real-Time Kernel Performance Visualization (Maximized Layout) ---
% This is the final script. It generates a publication-quality two-panel
% figure with LaTeX fonts, custom aesthetics, and a manually-tuned layout
% that maximizes the use of the figure space by eliminating unnecessary
% whitespace.

clear; clc; close all;

%% --- Data Loading and Configuration ---

% Define the data to be plotted for each panel (STRESS condition only)
std_kernel_files = {
    'standard_kernel/other_n0_stress.txt';
    'standard_kernel/other_n-19_stress.txt';
    'standard_kernel/fifo_p50_stress.txt';
    'standard_kernel/fifo_p99_stress.txt';
    'standard_kernel/rr_p50_stress.txt';
    'standard_kernel/rr_p99_stress.txt';
    'standard_kernel/deadline_R400_D4000_stress.txt';
    'standard_kernel/deadline_R800_D4000_stress.txt'
};
std_kernel_labels = {'OTHER n0', 'OTHER n-19', 'FIFO p50', 'FIFO p99', 'RR p50', 'RR p99', 'DL R400', 'DL R800'};

rt_kernel_files = {
    'preempt_rt_kernel/other_n0_stress.txt';
    'preempt_rt_kernel/other_n-19_stress.txt';
    'preempt_rt_kernel/fifo_p50_stress.txt';
    'preempt_rt_kernel/fifo_p99_stress.txt';
    'preempt_rt_kernel/rr_p50_stress.txt';
    'preempt_rt_kernel/rr_p99_stress.txt';
    'preempt_rt_kernel/deadline_R400_D4000_stress.txt';
    'preempt_rt_kernel/deadline_R800_D4000_stress.txt'
};
rt_kernel_labels = {'OTHER n0', 'OTHER n-19', 'FIFO p50', 'FIFO p99', 'RR p50', 'RR p99', 'DL R400', 'DL R800'};

% Load data
std_data = cellfun(@readmatrix, std_kernel_files, 'UniformOutput', false);
rt_data = cellfun(@readmatrix, rt_kernel_files, 'UniformOutput', false);

%% --- Figure Generation ---

% Create the main figure window
figure('Position', [100, 100, 800, 600]); % Slightly taller for angled labels

% --- Manual Layout Definition ---
% Manually define positions [left, bottom, width, height] in normalized units
% to eliminate wasted space.
left_margin = 0.05;
right_margin = 0.01;
bottom_margin = 0.15; % Increased for angled labels
top_margin = 0.08;
spacing = 0.05;

plot_width = (1 - left_margin - right_margin - spacing) / 2;
plot_height = 1 - bottom_margin - top_margin;

pos1 = [left_margin, bottom_margin, plot_width, plot_height];
pos2 = [left_margin + plot_width + spacing, bottom_margin, plot_width, plot_height];

% --- Create Axes and Plot ---
% Instead of subplot(), we create axes at our precise positions.
ax1 = axes('Position', pos1);
create_latex_boxplot(ax1, std_data, std_kernel_labels, ...
    '\textbf{(a) Standard Kernel Under Stress}', 'log');

ax2 = axes('Position', pos2);
create_latex_boxplot(ax2, rt_data, rt_kernel_labels, ...
    '\textbf{(b) PREEMPT\_RT Kernel Under Stress}', 'linear');

%% --- The Final, LaTeX-Enabled Plotting Function ---

function create_latex_boxplot(ax, data_cells, labels, title_str, y_scale_type)
    values = vertcat(data_cells{:});
    group = [];
    for i = 1:numel(data_cells)
        group = [group; repmat(i, size(data_cells{i}, 1), 1)];
    end

    fontName = 'Times New Roman';
    fontSize = 16; 
    
    boxplot(ax, values, group, ...
        'Labels', labels, 'Symbol', '', 'Whisker', 100, 'Widths', 0.6);
    
    % Custom Aesthetics
    h_boxes = findobj(ax, 'Tag', 'Box');
    h_medians = findobj(ax, 'Tag', 'Median');
    h_whiskers = findobj(ax, 'Tag', 'Whisker');
    colors = lines(numel(h_boxes));
    
    for j = 1:numel(h_boxes)
        k = numel(h_boxes) - j + 1;
        patch(ax, get(h_boxes(j), 'XData'), get(h_boxes(j), 'YData'), colors(k,:), 'FaceAlpha', 0.5);
        set(h_medians(j), 'Color', 'k', 'LineWidth', 1.5);
    end
    set(h_whiskers, 'LineStyle', '-', 'Color', 'k');
    h_caps = findobj(ax, 'Tag', 'Cap');
    set(h_caps, 'Color', 'k');

    % General Styling and LaTeX Font Rendering
    title(title_str, 'FontSize', fontSize+2, 'Interpreter', 'latex'); % Title slightly larger
    ylabel('Latency ($\mu$s)', 'FontSize', fontSize, 'Interpreter', 'latex');
    grid on;
    box on;
    set(ax, 'FontName', fontName, 'FontSize', fontSize, 'TickLabelInterpreter', 'latex');
    xtickangle(ax, 45);
    
    if strcmp(y_scale_type, 'log')
        set(ax, 'YScale', 'log');
        ylim(ax, [1, 15000]);
        yticks(ax, [10, 100, 1000, 4000, 10000]);
        yline(ax, 4000, '--k', '\textbf{250 Hz Deadline (4ms)}', 'LineWidth', 2, 'LabelVerticalAlignment', 'bottom', 'FontSize', fontSize, 'Color', [0.8 0.3 0.3], 'Interpreter', 'latex');
    else % linear
        set(ax, 'YScale', 'log');
        ylim(ax, [0, 1000]);
        yticks(ax, [5, 10, 20, 30 ,50, 100, 250, 1000, 4000, 10000]);
        yline(ax, 4000, '--k', '\textbf{250 Hz Deadline (4ms)}', 'LineWidth', 2, 'FontSize', fontSize, 'Color', [0.8 0.3 0.3], 'Interpreter', 'latex');
    end
end
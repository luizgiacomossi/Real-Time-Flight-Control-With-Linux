% Read all data
all_data = {
    readmatrix('other_n0_no_stress.txt')        % 1
    readmatrix('other_n0_stress.txt')           % 2
    readmatrix('other_n-19_no_stress.txt')      % 3
    readmatrix('other_n-19_stress.txt')         % 4
    readmatrix('rr_p50_no_stress.txt')          % 5
    readmatrix('rr_p50_stress.txt')             % 6
    readmatrix('rr_p99_no_stress.txt')          % 7
    readmatrix('rr_p99_stress.txt')             % 8
    readmatrix('fifo_p50_no_stress.txt')        % 9
    readmatrix('fifo_p50_stress.txt')           % 10
    readmatrix('fifo_p99_no_stress.txt')        % 11
    readmatrix('fifo_p99_stress.txt')           % 12
    readmatrix('deadline_R400_D4000_no_stress.txt') % 13
    readmatrix('deadline_R400_D4000_stress.txt')    % 14
    readmatrix('deadline_R800_D4000_no_stress.txt') % 15
    readmatrix('deadline_R800_D4000_stress.txt')    % 16
};

% Labels
labels_other = {'other-n0', 'other-n19'};
labels_rest = {'RR-p50', 'RR-p99', 'FIFO-p50', 'FIFO-p99', 'DL-R400', 'DL-R800'};

% Indices for 'other' scheduler
other_no_stress_indices = [1 3];  % other_n0_no_stress, other_n-19_no_stress
other_stress_indices = [2 4];     % other_n0_stress, other_n-19_stress

% Indices for other schedulers (excluding 'other')
rest_no_stress_indices = [5 7 9 11 13 15];  % RR, FIFO, Deadline - no stress
rest_stress_indices = [6 8 10 12 14 16];    % RR, FIFO, Deadline - stress

%% Figure 1: Only 'Other' scheduler
figure('Position', [100, 100, 800, 600]);
set(gca, 'YScale', 'log')

% Subplot 1: Other - No Stress
subplot(1, 2, 1);
plot_box_subplot(all_data(other_no_stress_indices), labels_other, 'Other Scheduler – No Stress', [0.2 0.6 0.8]);
set(gca, 'YScale', 'log')

% Subplot 2: Other - Stress
subplot(1, 2, 2);
plot_box_subplot(all_data(other_stress_indices), labels_other, 'Other Scheduler – Stress', [0.8 0.2 0.2]);
set(gca, 'YScale', 'log')

%% Figure 2: All other schedulers (excluding 'Other')
figure('Position', [100, 100, 800, 600]);
set(gca, 'YScale', 'log')

% Subplot 1: Rest - No Stress
subplot(1, 2, 1);
plot_box_subplot(all_data(rest_no_stress_indices), labels_rest, 'RT Schedulers – No Stress', [0.2 0.6 0.2]);
set(gca, 'YScale', 'log')

% Subplot 2: Rest - Stress
subplot(1, 2, 2);
plot_box_subplot(all_data(rest_stress_indices), labels_rest, 'RT Schedulers – Stress', [0.8 0.2 0.2]);
set(gca, 'YScale', 'log')

%% FUNCTION for subplots
function plot_box_subplot(data_cells, labels, title_str, color)
    values = vertcat(data_cells{:});
    group = arrayfun(@(i) repmat(i, length(data_cells{i}), 1), 1:numel(data_cells), 'UniformOutput', false);
    group = vertcat(group{:});

    fontName = 'latex';
    fontSize = 18; 
    
    fig= boxplot(values, group, ...
        'Labels', labels, ...
        'Colors', color, ...
        'Symbol', 'x', ...
        'Whisker', inf, ...
        'Widths', 0.5)
    
    xtickangle(45)
    yticks([100 125 150 156 160 175 200 225 250 300 500 1000 2000 4000 10000])
    set(gca, 'YScale', 'log', 'fontName', fontName, 'FontSize', fontSize)
    title(title_str, 'FontSize', fontSize, 'Interpreter', 'latex', 'FontName', fontName)
    ylabel('Latency ($\mu$s)', 'Interpreter', 'latex', 'FontSize', 20, 'FontName', fontName);
    grid on
    box on
    
    low = prctile(values, 1);
    high = prctile(values, 99);
    ylim([low high])

    h = findobj(gca, 'Tag', 'Box');
    colors = lines(numel(h));
    for j = 1:length(h)
        patch(get(h(j), 'XData'), get(h(j), 'YData'), colors(j,:), 'FaceAlpha', 0.3);
    end


end
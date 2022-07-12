% Run simulation and assign outputs
disp('Running mocap simulation');
simOut = sim('mocap.slx');
outputs = simOut.yout;

% Get position data
position_data = outputs.get('position').Values.Data;
surge_x = position_data(:, 1);
sway_y = position_data(:, 2);
heave_z = position_data(:, 3);
roll_x = position_data(:, 4);
pitch_y = position_data(:, 5);
yaw_z = position_data(:, 6);

% Plot surge, sway, heave
figure
hold on
plot(surge_x)
plot(sway_y)
plot(heave_z)
legend('surge (x)', 'sway (y)', 'heave (z)')
hold off

% Plot roll, pitch, yaw
figure
plot(roll_x)
plot(pitch_y)
plot(yaw_z)
legend('roll (x)', 'pitch (y)', 'yaw (z)')
hold off

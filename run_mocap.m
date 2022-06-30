disp('Running mocap simulation');
simOut = sim('mocap.slx');
outputs = simOut.yout;

% Get position data
position = outputs.get('position').Values;

plot(position)
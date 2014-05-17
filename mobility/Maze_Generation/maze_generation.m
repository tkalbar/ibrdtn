outfileID = fopen('Maze_traces_propspeed_20000.txt', 'w');

time = 20000;


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%Maze

start_x = [0; 6600; 1800; 1800; 1800; ... 
    1800; 1800; 4200; 1800; 6600; ...
    4200; 11400; 6600; 6600; 6600; ...
    11400; 13800; 13800; 16200; 16200; ...
    13800; 18600; 13800; 21000; 21000; ...
    21000; 23400];

start_y = [6600; 4200; 4200; 1800; 1800; ...
    6600; 16200; 13200; 11400; 9000; ...
    9000; 11400; 16800; 13200; 13800; ...
    13800; 4200; 4200; 4200; 9000; ...
    11400; 11400; 16200; 1800; 1800; ...
    13800; 7800];

end_x = [7200; 6600; 6600; 1800; 9600; ...
    1800; 4200; 4200; 11400; 6600; ...
    6600; 11400; 11400; 6600; 9000; ...
    16200; 13800; 16200; 16200; 21000; ...
    18600; 18600; 18600; 24000; 21000; ...
    24000; 23400];

end_y = [6600; 6600; 4200; 4200; 1800; ...
    16800; 16200; 16200; 11400; 11400; ...
    9000; 16800; 16800; 16800; 13800; ...
    13800; 13800; 4200; 9000; 9000; ...
    11400; 16800; 16200; 1800; 16800; ...
    13800; 16800];

%speed = 20*ones(length(start_x), 1);

speed = [120; 48; 48; 48; 78; 204; 24; 60; 114; 48; 24; 108; 48; 72; 24; 36; 192; 24; 96; 192; 48; 108; 48; 30; 300; 96; 180];

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%Ladder

% start_x = [0;0;0;0;0;0;5000;5000;5000;5000;5000;10000;9900];
% 
% start_y = [8000;2000;2000;4000;6000;8000;2000;2000;4000;6000;8000;2000;2000];
% 
% end_x = [100;0;5000;5000;5000;5000;5000;10000;10000;10000;10000;10000;10000];
% 
% end_y =[8000;8000;2000;4000;6000;8000;8000;2000;4000;6000;8000;8000;2000];
%
% speed = 20*ones(length(start_x), 1);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%Line

% start_x = [0; 500; 1000; 1500; 2000; 2500];
% 
% start_y = [1000; 1000; 1000; 1000; 1000; 1000];
% 
% end_x = [500; 1000; 1500; 2000; 2500; 3000];
% 
% end_y = [1000; 1000; 1000; 1000; 1000; 1000];
% 
% speed = [10, 20, 30, 5, 15, 25];
% 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    
nodes = length(start_x);
    
    


final_output_x = start_x;
final_output_y = start_y;

end_temp_x = end_x;
end_temp_y = end_y;


for t = 2:time
    for i = 1:nodes
        
        new_x = final_output_x(i,t-1);
        new_y = final_output_y(i,t-1);
        
        if (start_x(i)==end_x(i))
            direction(i) = sign(end_temp_y(i)-new_y);
            if (direction(i) ==0)
                if (direction_old(i) > 0)
                    end_temp_y(i) = start_y(i);
                else
                    end_temp_y(i) = end_y(i);
                end
                direction(i) = -direction_old(i);
            end
            direction_old(i) = direction(i);
            
            new_y = new_y + (speed(i)*direction(i));
            if (direction(i)<0 && new_y<end_temp_y(i))
                end_temp_y(i) = end_y(i);
                new_y = start_y(i) + abs(start_y(i)-new_y);
            end
            if (direction(i)>0 && new_y>end_temp_y(i))
                end_temp_y(i) = start_y(i);
                new_y = end_y(i) - abs(new_y-end_y(i));
            end  
        end
        if (start_y(i)==end_y(i))
            direction(i) = sign(end_temp_x(i)-new_x);
            if (direction(i) ==0)
                if (direction_old(i) > 0)
                    end_temp_x(i) = start_x(i);
                else
                    end_temp_x(i) = end_x(i);
                end
                direction(i) = -direction_old(i);
            end
            direction_old(i) = direction(i);
            new_x = new_x + (speed(i)*direction(i));
            if (direction(i)<0 && new_x<end_temp_x(i))
                end_temp_x(i) = end_x(i);
                new_x = start_x(i) + abs(start_x(i)-new_x);
            end
            if (direction(i)>0 && new_x>end_temp_x(i))
                end_temp_x(i) = start_x(i);
                new_x = end_x(i) - abs(new_x-end_x(i));
            end    
        end
        final_output_x(i,t) = new_x; 
        final_output_y(i,t) = new_y;
        
    end
end



fprintf(outfileID, 'Settings === Total time = %d, Number of Nodes = %d, Speed = %d\n', [time, nodes, speed(1)]);
fprintf(outfileID, 'Time\tNode\tPos_X\tPos_Y\n');

for t = 1:time
    for i = 1:nodes
        fprintf(outfileID, '%d\t%d\t%d\t%d\n', [t, i, final_output_x(i,t), final_output_y(i,t)]);
    end
end
fclose(outfileID);

figure(1);
set(gcf,'Renderer','OpenGL'); 
h = plot(final_output_x(:,1)', final_output_y(:,1)','o', 'MarkerSize', 16, 'MarkerEdgeColor', 'r', 'MarkerFaceColor', 'y');
set(h,'EraseMode','normal');
xlim([0,25200]);
ylim([0,18000]);

% xlim([0,12000]);
% ylim([0,10000]);
% 
% xlim([0,4000]);
% ylim([0,2000]);

%Animation Loop
for t = 2:time
    set(h,'XData',final_output_x(:,t));
    set(h,'YData',final_output_y(:,t));
    drawnow;
end



outfileID = fopen('Maze_traces_5000.txt', 'w');

time = 5000;

start_x = [0; 6600; 1800; 1800; 1800; ... 
    1800; 1800; 4200; 1800; 6600; ...
    4200; 11400; 6600; 6600; 6600; ...
    11400; 13800; 13800; 16200; 16200; ...
    13800; 18600; 13800; 21000; 21000; ...
    21000; 23400];

start_y = [6600; 4200; 4200; 1800; 1800; ...
    8400; 16200; 13200; 11400; 9000; ...
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
    
nodes = length(start_x);
    
    

%start_x = [0; 2];

%start_y = [0; 2];

%end_x = [5; 2];

%end_y = [0; 7];

speed = 20*ones(nodes, 1);


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
fprintf(outfileID, 'Time\t Node\t Pos_X\t Pos_Y\n');

for t = 1:time
    for i = 1:nodes
        fprintf(outfileID, '%d \t %d \t %d \t %d\n', [t, i, final_output_x(i,t), final_output_y(i,t)]);
    end
end
fclose(outfileID);

figure(1);
set(gcf,'Renderer','OpenGL'); 
h = plot(final_output_x(:,1)', final_output_y(:,1)','o', 'MarkerSize', 10, 'MarkerEdgeColor', 'r', 'MarkerFaceColor', 'y');
set(h,'EraseMode','normal');
xlim([0,25200]);
ylim([0,18000]);

% Animation Loop
for t = 2:time
    set(h,'XData',final_output_x(:,t));
    set(h,'YData',final_output_y(:,t));
    drawnow;
end



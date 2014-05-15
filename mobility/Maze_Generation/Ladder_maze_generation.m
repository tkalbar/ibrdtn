outfileID = fopen('Ladder_traces_10000.txt', 'w');



%Total running time
time = 10000;


%Starting and ending (x,y) coordinates
start_x = [0; 0; 0; 0; 0; ... 
    0; 5000; 5000; 5000; 5000; ...
    5000; 10000; 9900];

start_y = [8000; 2000; 2000; 4000; 6000; ...
    8000; 2000; 2000; 4000; 6000; ...
    8000; 2000; 2000];


end_x = [0; 0; 4000; 4000; 4000; ... 
    4000; 5000; 9000; 9000; 9000; ...
    9000; 10000; 9900];

end_y = [8000; 8000; 2000; 4000; 6000; ...
    8000; 8000; 2000; 4000; 6000; ...
    8000; 2000; 2000];
    
nodes = length(start_x);
    
    

%start_x = [0; 2];

%start_y = [0; 2];

%end_x = [5; 2];

%end_y = [0; 7];


%Generate speed vector. Each node can have its own speed
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


%%%%%%%%%%%
%Output file Generation

fprintf(outfileID, 'Settings === Total time = %d, Number of Nodes = %d, Speed = %d\n', [time, nodes, speed(1)]);
fprintf(outfileID, 'Time\t Node\t Pos_X\t Pos_Y\n');

for t = 1:time
    for i = 1:nodes
        fprintf(outfileID, '%d \t %d \t %d \t %d\n', [t, i, final_output_x(i,t), final_output_y(i,t)]);
    end
end
fclose(outfileID);
%%%%%%%%%%%%%%%



%%%%%%%%%%%%%%
%Create visualization
%Comment out if only interested in file generation

% figure(1);
% set(gcf,'Renderer','OpenGL'); 
% h = plot(final_output_x(:,1)', final_output_y(:,1)','o', 'MarkerSize', 10, 'MarkerEdgeColor', 'r', 'MarkerFaceColor', 'y');
% set(h,'EraseMode','normal');
% xlim([0,25200]);
% ylim([0,18000]);
% 
% % Animation Loop
% for t = 2:time
%     set(h,'XData',final_output_x(:,t));
%     set(h,'YData',final_output_y(:,t));
%     drawnow;
% end

%%%%%%%%%%%%%%%%

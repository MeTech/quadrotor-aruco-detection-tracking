%rosinit;
%rosshutdown;
path = rossubscriber('/trajectory');
pathData= receive(path,2);
pathData.Poses(1,1).Pose.Position.X;
[m,n] = size(pathData.Poses);
x=[];
y=[];
z=[];



for i=1:m
x(i)= pathData.Poses(i,1).Pose.Position.X;
y(i)= pathData.Poses(i,1).Pose.Position.Y;
z(i)= pathData.Poses(i,1).Pose.Position.Z;
end

figure(1)
%subplot(1,3,1)
plot3(x,y,z)
%title('Quadrotor 3-D Path')
xlabel('x') % x-axis label
ylabel('y') % y-axis label
zlabel('z') % z-axis label
xlim([-0.2 2.2])
ylim([-0.2 2.2])
zlim([-0.2 2.2])
hold on
plot3(2,2,0,'r*')

figure(2)
%subplot(1,3,2)
plot(x,y)
%title('Quadrotor XY Path')
xlabel('x') % x-axis label
ylabel('y') % y-axis label
xlim([-0.2 2.2])
ylim([-0.2 2.2])

hold on
plot(2,2,'r*')

%time olayı düzgün olmuyor
figure(3)
%subplot(1,3,3)
t=0:0.2:25.2;
plot(t,z)
ylim([0 2.2])
%title('Quadrotor Z Path')
xlabel('t') % x-axis label
ylabel('z') % x-axis label


%x=x+ones(1,size(x))*0.15
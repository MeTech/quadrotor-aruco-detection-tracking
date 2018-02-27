figure

plot(2,2,'bo')
hold on
plot(1.996,1.993,'r*')
hold on
plot(1.992,2.001,'r*')
hold on
plot(1.995,2.016,'r*')
hold on
plot(2.007,2.019,'r*')
hold on
plot(2.020,2.012,'r*')
xlabel('x(m)') % x-axis label
ylabel('y(m)') % y-axis label
xlim([1.96 2.039])
ylim([1.96 2.039])
hold on
plot(2.019,2.002,'r*')
hold on
plot(2.017,1.995,'r*')
hold on
plot(2.008,1.983,'r*')
hold on
viscircles([2 2],0.016,'LineStyle','--','LineWidth',1,'Color','black')


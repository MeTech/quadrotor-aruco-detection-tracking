figure

plot(2,2,'bo')
hold on
plot(1.978,1.970,'r*')
hold on
plot(1.965,1.997,'r*')
hold on
plot(1.971,2.042,'r*')
hold on
plot(2.015,2.046,'r*')
hold on
plot(2.042,2.024,'r*')
xlabel('x(m)') % x-axis label
ylabel('y(m)') % y-axis label
xlim([1.90 2.1])
ylim([1.90 2.1])
hold on
plot(2.044,1.995,'r*')
hold on
plot(2.037,1.973,'r*')
hold on
plot(2.010,1.967,'r*')
hold on
legend('Hedef','(0,0) Kalkış','(2,0) Kalkış','(4,0) Kalkış','(4,2) Kalkış','(4,4) Kalkış','(2,4) Kalkış','(0,4) Kalkış','(0,2) Kalkış')

viscircles([2 2],0.05,'LineStyle','--','LineWidth',1,'Color','black')

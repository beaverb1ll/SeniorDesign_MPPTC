EXECUTABLE = mpptc

CC      = gcc
CFLAGS  = -Wall  
LFLAGS  =
C_SRCS	= $(EXECUTABLE).c

# gcc iemcd.c -Wall -L/usr/lib64/mysql -L/usr/lib -o iemcd  -lmysqlclient -l'hidapi-hidraw'

all: $(EXECUTABLE)

$(EXECUTABLE):
	$(CC) $(C_SRCS) $(CFLAGS) -o $(EXECUTABLE) $(LFLAGS)

install:
	install $(EXECUTABLE) /usr/bin/$(EXECUTABLE)
	install ./systemd/$(EXECUTABLE).service /usr/lib/systemd/system/$(EXECUTABLE).service
	install ./systemd/$(EXECUTABLE).conf /etc/$(EXECUTABLE).conf

	install ./systemd/BBB-GPIO.service /usr/lib/systemd/system/BBB-GPIO.service
	install ./systemd/enable-BBB-GPIO.sh /usr/bin/enable-BBB-GPIO.sh
	
	install ./systemd/BBB-PWM.service /usr/lib/systemd/system/BBB-PWM.service
	install ./systemd/enable-BBB-PWM.sh /usr/bin/enable-BBB-PWM.sh

	install ./systemd/BBB-ADC.service /usr/lib/systemd/system/BBB-ADC.service
	install ./systemd/enable-BBB-ADC.sh /usr/bin/enable-BBB-ADC.sh

	systemctl daemon-reload
	systemctl restart $(EXECUTABLE).service

clean:
	rm -f $(EXECUTABLE)

.PHONY: clean libs

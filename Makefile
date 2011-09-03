OBJ=ghctrl

ghctrl: src/ghctrl.c src/debug.h src/print.h src/print.c
	$(CC) -Wall -Wextra -W -g3 $? -o ghctrl -lcwiimote -lasound

clean:
	rm ghctrl

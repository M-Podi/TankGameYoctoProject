CFLAGS += -Wall -Wextra
LDFLAGS += -lncurses -lpthread

TARGET = tank-game
SRC = tank-game.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

install:
	mkdir -p $(DESTDIR)/usr/bin
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/

clean:
	rm -f $(TARGET)
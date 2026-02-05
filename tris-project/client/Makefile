CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread
TARGET = client
OBJS = main.o connessione.o gioco.o interfaccia.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c strutture.h connessione.h gioco.h interfaccia.h
connessione.o: connessione.c connessione.h strutture.h
gioco.o: gioco.c gioco.h strutture.h interfaccia.h connessione.h
interfaccia.o: interfaccia.c interfaccia.h strutture.h

clean:
	rm -f $(OBJS) $(TARGET)

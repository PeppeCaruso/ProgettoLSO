# Immagine base Ubuntu
FROM ubuntu:latest

# Installa compilatore e make
RUN apt-get update && apt-get install -y gcc make && rm -rf /var/lib/apt/lists/*

# Directory di lavoro
WORKDIR /app

# Copia tutti i file nel container
COPY . .

# Compila
RUN make

# Esegue il client
CMD ["./client"]

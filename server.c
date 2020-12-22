#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PORT htons(21212)

char *command[3];

void readCommand(char path[])
{
    int count = 0;
    int i = 0, n = strlen(path);
    while(i <= n)
    {
        if(path[i] == '\0' || path[i] == ' ')
            count++;
        i++;
    }
    i = 0;
    command[0] = strtok(path, " \n");
    i = 1;
    command[count] = NULL;
    while(i < count)
    {
        command[i++] = strtok(NULL, " \r\n");
    }
}

void handleUpload(int gn, char path[]){
    char file_name[sizeof(command[1])], *ptr, realPath[512];
    FILE* file;
    unsigned char bufor[1024];
    long length, data, totalData = 0;
    int wczytano = 0;

    printf("Klient chce wstawić plik: %s\n", command[1]);
    strcpy(file_name, command[1]);
    memset(path, 0, 512);
    if (recv(gn, path, 512, 0) <= 0)
    {
        printf("Blad przy odczycie dlugosci\n");
        return;
    }
    length = strtol(path, &ptr, 10);

    mkdir("data",0777);

    sprintf(realPath, "./data/%s", file_name);

    file = fopen(realPath, "w");

    while (totalData < length)
    {
        memset(bufor, 0, 1025);
        data = recv(gn, bufor, 1024, 0);
        if (data < 0) break;
        totalData += data;
        wczytano = fwrite(bufor, sizeof(char), data, file);
    }
    if (totalData == length)
    {

        printf("Plik odebrany poprawnie\n");
    }
    else
        printf("Blad przy odbieraniu pliku\n");
    fclose(file);
}

void handleDownload(int gn, char path[]){
    long length, data, totalData = 0, przeczytano;
    struct stat fileinfo;
    FILE* file;
    unsigned char bufor[1024];
    char realPath[512];

    printf("Klient chce plika: %s\n", command[1]);
    sprintf(realPath, "./data/%s", command[1]);
    if (stat(realPath, &fileinfo) < 0)
    {
        printf("Nie moge pobrac informacji o pliku\n");
        return;
    }

    if (fileinfo.st_size == 0)
    {
        printf("Rozmiar pliku 0\n");
        return;
    }

    printf("Dlugosc pliku: %ld\n", fileinfo.st_size);

    length = fileinfo.st_size;

    file = fopen(realPath, "rb");
    if (file == NULL)
    {
        printf("Blad przy otwarciu pliku\n");
        return;
    }

    while (totalData < length)
    {
        memset(bufor, 0, 1024);
        przeczytano = fread(bufor, 1, 1024, file);
        data = write(gn, bufor, przeczytano);
        if (przeczytano != data)
        {
            break;
        }
        totalData += data;
    }
    if (totalData == length)
    {

        printf("Plik wyslany poprawnie\n");
    }
    else
        printf("Blad przy wysylaniu pliku\n");
    fclose(file);
}

void handleClient(int gn)
{
    while(1)
    {
        char path[512];
        memset(path, 0, 512);
        printf("\nOczekuje na polecenie...\n");
        if (recv(gn, path, 512, 0) <= 0)
        {
            printf("Blad przy odczycie sciezki\n");
            return;
        }

        readCommand(path);

        //***UPLOAD
        if(strcmp(command[0], "upload") == 0 ){
            handleUpload(gn, path);
        }
        //***DOWNLOAD
        else if(strcmp(command[0], "download") == 0)
        {
            handleDownload(gn, path);
        }
        //***END
        else if(strcmp(command[0], "end") == 0)
        {
            printf("Koncze proces\n");
            return;
        }
        else
        {
            printf("Nie znam takiego polecenia\nProsze wpisać poprawne polecenie!\n");
        }
    }
}


int main(void)
{
    int gn_nasluch, gn_klienta;
    struct sockaddr_in adr;
    socklen_t dladr = sizeof(struct sockaddr_in);

    gn_nasluch = socket(PF_INET, SOCK_STREAM, 0);
    adr.sin_family = AF_INET;
    adr.sin_port = PORT;
    adr.sin_addr.s_addr = INADDR_ANY;
    memset(adr.sin_zero, 0, sizeof(adr.sin_zero));

    if (bind(gn_nasluch, (struct sockaddr*) &adr, dladr) < 0)
    {
        printf("Bind nie powiodl sie\n");
        return 1;
    }

    listen(gn_nasluch, 10);

    while(1)
    {
        dladr = sizeof(struct sockaddr_in);
        gn_klienta = accept(gn_nasluch, (struct sockaddr*) &adr, &dladr);
        if (gn_klienta < 0)
        {
            printf("Accept zwrocil blad\n");
            continue;
        }
        printf("Polaczenie od %s:%u\n",
               inet_ntoa(adr.sin_addr),
               ntohs(adr.sin_port)
              );
        if (fork() == 0)
        {
            /* proces potomny */
            printf("Zaczynam obsluge klienta\n");
            handleClient(gn_klienta);
            printf("Zamykam gniazdo\n");
            close(gn_klienta);
            exit(0);
        }
    }
    return 0;
}

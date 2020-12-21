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

void handleClient(int gn)
{

    long length, data, totalData, przeczytano, odebrano_razem;
    struct stat fileinfo;
    FILE* file;
    unsigned char bufor[1024];
    struct stat st = {0};

    while(1)
    {
        char path[512];
        memset(path, 0, 512);
        printf("\nOczekuje na polecenie...\n");
        if (recv(gn, path, 512, 0) <= 0)
        {
            printf("Path w if: %s, wielkosc: %ld\n", path, strlen(path));
            printf("Potomny: blad przy odczycie sciezki\n");
            return;
        }

        printf("Path: %s\n", path);
        readCommand(path);

        //***UPLOAD
        if(strcmp(command[0], "upload") == 0 )
        {
            printf("Potomny: klient chce wstawić plik: %s\n", command[1]);
            char file_name[sizeof(command[1])];
            char test[sizeof(command[1])];

            strcpy(file_name, command[1]);
            memset(path, 0, 512);
            if (recv(gn, path, 512, 0) <= 0)
            {
                printf("Blad przy odczycie dlugosci\n");
                return;
            }

            printf("Length: %s\n", path);
            char *ptr;
            length = strtol(path, &ptr, 10);

            totalData = 0;
            mkdir("data",0777);

            char realPath[512];
            strcpy(test,file_name);
            sprintf(realPath, "./data/%s", test);

            file = fopen(realPath, "w");

            int wczytano = 0;
            while (totalData < length)
            {

                memset(bufor, 0, 1025);
                data = recv(gn, bufor, 1024, 0);
                if (data < 0)
                    break;
                totalData += data;
                wczytano = fwrite(bufor, sizeof(char), data, file);
            }
            fclose(file);

        }
        //***DOWNLOAD
        else if(strcmp(command[0], "download") == 0)
        {
            printf("Potomny: klient chce plik %s\n", command[1]);
            char realPath[512];
            sprintf(realPath, "./data/%s", command[1]);
            printf("realPath: %s\n", realPath);
            if (stat(realPath, &fileinfo) < 0)
            {
                printf("Potomny: nie moge pobrac informacji o pliku\n");
                return;
            }

            if (fileinfo.st_size == 0)
            {
                printf("Potomny: rozmiar pliku 0\n");
                return;
            }

            printf("Potomny: dlugosc pliku: %ld\n", fileinfo.st_size);

            length = fileinfo.st_size;
            totalData = 0;

            file = fopen(realPath, "rb");
            if (file == NULL)
            {
                printf("Potomny: blad przy otwarciu pliku\n");
                return;
            }

            while (1)
            {
                if(totalData >= length)
                {
                    break;
                }
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

                printf("Potomny: plik wyslany poprawnie\n");
            }
            else
                printf("Potomny: blad przy wysylaniu pliku\n");
            fclose(file);

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
        printf("Glowny: bind nie powiodl sie\n");
        return 1;
    }

    listen(gn_nasluch, 10);

    while(1)
    {
        dladr = sizeof(struct sockaddr_in);
        gn_klienta = accept(gn_nasluch, (struct sockaddr*) &adr, &dladr);
        if (gn_klienta < 0)
        {
            printf("Glowny: accept zwrocil blad\n");
            continue;
        }
        printf("Glowny: polaczenie od %s:%u\n",
               inet_ntoa(adr.sin_addr),
               ntohs(adr.sin_port)
              );
        printf("Glowny: tworze proces potomny\n");
        if (fork() == 0)
        {
            /* proces potomny */
            printf("Potomny: zaczynam obsluge\n");
            handleClient(gn_klienta);
            printf("Potomny: zamykam gniazdo\n");
            close(gn_klienta);
            printf("Potomny: koncze proces\n");
            exit(0);
        }
        else
        {
            /* proces macierzysty */
            printf("Glowny: wracam do nasluchu\n");
            continue;
        }
    }
    return 0;
}

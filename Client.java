

import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.concurrent.TimeUnit;

public class Client {

    private static ArrayList<String> divideCommand(String line){
        ArrayList<String> commands = new ArrayList<>();
        if(line.contains(" ")) {
            commands.add(line.substring(0, line.indexOf(' ')));
            commands.add(line.substring(line.indexOf(' ') + 1));
            System.out.printf("f: %s, s: %s%n",
                    commands.get(0), commands.get(1));
        } else {
            commands.add(line);
        }
        return commands;
    }

    private static void download(DataOutputStream dos, byte[] data, Socket socket, ArrayList<String> command, byte[] bytes) throws IOException {
        int count;
        dos.write(data);
        dos.flush();

        try {
            FileOutputStream fos = new FileOutputStream(command.get(1));
            PrintWriter writerObj = new PrintWriter(fos);
            InputStream in = socket.getInputStream();


            System.out.println("Plik:");

            while ((count = in.read(bytes)) > 0){
                //System.out.println(count);
                fos.write(bytes, 0, count);
                if(count<1024) {
                    fos.close();
                    writerObj.close();
                    System.out.println("Zrobione");
                    break;
                }
            }

        }
        catch(Exception e){
            System.out.println("Odebrano plik po 10s (wielokrotnosc 1024)");
        }
    }

    private static void upload(DataOutputStream dos, byte[] data, ArrayList<String> command, byte[] bytes, String line) throws IOException, InterruptedException {

        int count;

        File file = new File(command.get(1));
        while(!file.isFile()) {
            System.out.println("Nie ma takiego pliku. Wpisz nazwę ponownie: ");
            Scanner scannerUpload = new Scanner(System.in);
            line = scannerUpload.nextLine();
            scannerUpload.close();
            command.set(1, line);
            file = new File(command.get(1));
        }
        line = command.get(0) + " " + command.get(1);
        data = line.getBytes(StandardCharsets.UTF_8);

        dos.write(data);
        dos.flush();
        TimeUnit.SECONDS.sleep(1);
        // Get the size of the file
        long length = file.length();
        System.out.println("lenght: " +length);
        String string_len = String.valueOf(length);

        byte[] file_lenght = string_len.getBytes(StandardCharsets.UTF_8);

        dos.write(file_lenght);
        dos.flush();

        InputStream in = new FileInputStream(file);

        while ((count = in.read(bytes)) != -1) {
            dos.write(bytes, 0, count);

        }
        in.close();
    }

    public static void main(String[] args) {
        ArrayList<String> command;
        String line;
        byte[] bytes = new byte[1024];

        Scanner scanner = new Scanner(System.in);
        String host = "127.0.0.1";
        String port2 = "21212";
        //System.out.println("Podaj host:");
        //String host = scanner.nextLine();
        //System.out.println("Podaj port:");
        //String port2 = scanner.nextLine();

        try {
            // ustal adres serwera
            InetAddress addr = InetAddress.getByName(host);

            // ustal port
            int port = Integer.parseInt(port2);

            // utworz gniazdo i od razu podlacz je
            // do addr:port
            Socket socket = new Socket(addr, port);
            socket.setSoTimeout(10000);

            // pobierz strumienie i zbuduj na nich
            // "lepsze" strumienie
            DataOutputStream dos = new DataOutputStream(
                    socket.getOutputStream());
            DataInputStream dis = new DataInputStream(
                    socket.getInputStream());

            while(true) {

                System.out.println("\nPodaj wiadomosc");

                line = scanner.nextLine();
                //dzielenie komendy
                command = divideCommand(line);
                byte[] data = line.getBytes(StandardCharsets.UTF_8);
                if (command.get(0).equals("end")){
                    dos.write(data);
                    dos.flush();
                    break;
                }


                switch(command.get(0)) {

                    case "download":

                        download(dos, data, socket, command, bytes);
                        break;

                    case "upload":

                        upload(dos, data, command, bytes, line);
                        break;

                    case "test":
                        System.out.println("Testowanko");
                        break;

                    default:
                        System.out.println("Nie rozpoznano komendy");

                }

                command.clear();

            }
            // koniec rozmowy
            dis.close();
            dos.close();
            scanner.close();
            socket.close();

            // moga byc wyjatki dot. gniazd,
            // getByName, parseInt i strumieni
        } catch (Exception e) {
            e.printStackTrace();
        }
        System.out.println("Klient zakonczyl dzialanie");
    }
}

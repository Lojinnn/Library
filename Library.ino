#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <SPI.h>
#include <MFRC522.h>

// --- НАСТРОЙКИ СЕТИ ---
const char* ssid = "YOUR_WIFI_SSID";       
const char* password = "YOUR_WIFI_PASSWORD"; 

// --- ПИНЫ ДВУХ RC522 ---
#define SS_PIN_USER  12
#define RST_PIN_USER 22
#define SS_PIN_BOOK  15
#define RST_PIN_BOOK 21

// Создаем два объекта считывателей
MFRC522 mfrc522User(SS_PIN_USER, RST_PIN_USER);
MFRC522 mfrc522Book(SS_PIN_BOOK, RST_PIN_BOOK);

String lastUserUID = "";
String lastBookUID = "";

WebServer server(80);
const char* usersFile = "/users.csv";
const char* booksFile = "/books.csv";

// HTML и CSS шаблон (Черный / Белый / Красный)
const char* htmlTemplate = R"rawliteral(
<!DOCTYPE html>
<html lang='ru'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <title>Библиотека | Терминал</title>
    <style>
        :root {
            --black: #111111;
            --dark-gray: #1e1e1e;
            --red: #e63946;
            --red-hover: #d62828;
            --white: #ffffff;
            --bg: #f4f4f5;
            --border: #e0e0e0;
        }
        body { font-family: 'Segoe UI', system-ui, sans-serif; background: var(--bg); color: var(--black); margin: 0; padding-bottom: 40px; }
        
        /* Шапка */
        header { background: var(--black); color: var(--white); padding: 20px; text-align: center; border-bottom: 4px solid var(--red); }
        header h1 { margin: 0; font-size: 24px; text-transform: uppercase; letter-spacing: 2px; }
        header span { color: var(--red); }

        .container { max-width: 1000px; margin: 30px auto; padding: 0 20px; }
        
        /* Сетка карточек */
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 25px; margin-bottom: 30px; }
        
        .card { background: var(--white); border-radius: 8px; box-shadow: 0 10px 20px rgba(0,0,0,0.05); overflow: hidden; border: 1px solid var(--border); display: flex; flex-direction: column; }
        .card-header { background: var(--black); color: var(--white); padding: 15px 20px; font-weight: bold; font-size: 18px; display: flex; justify-content: space-between; align-items: center; }
        .card-body { padding: 20px; flex: 1; }

        /* Формы */
        label { display: block; margin-top: 15px; font-weight: 600; font-size: 14px; color: #555; }
        input { width: 100%; padding: 12px; margin-top: 5px; box-sizing: border-box; border: 1px solid var(--border); border-radius: 4px; font-size: 15px; background: #fafafa; transition: 0.2s; }
        input:focus { outline: none; border-color: var(--red); background: var(--white); box-shadow: 0 0 0 3px rgba(230, 57, 70, 0.1); }
        input[readonly] { font-family: monospace; font-weight: bold; color: var(--red); }

        /* Кнопки */
        .btn { border: none; padding: 12px; border-radius: 4px; cursor: pointer; font-size: 15px; font-weight: bold; transition: 0.2s; width: 100%; margin-top: 20px; text-transform: uppercase; letter-spacing: 1px; }
        .btn-black { background: var(--black); color: var(--white); }
        .btn-black:hover { background: #333; }
        
        .btn-red { background: var(--red); color: var(--white); margin-top: 5px; padding: 10px; }
        .btn-red:hover { background: var(--red-hover); }

        /* Таблицы */
        .table-wrap { overflow-x: auto; }
        table { width: 100%; border-collapse: collapse; font-size: 14px; }
        th { background: var(--black); color: var(--white); text-align: left; padding: 12px; }
        td { padding: 12px; border-bottom: 1px solid var(--border); }
        tr:hover { background: #fff5f5; }
        .uid { font-family: monospace; color: var(--red); font-weight: bold; }
        .empty { text-align: center; color: #999; padding: 30px; }
    </style>
</head>
<body>

<header>
    <h1>СИСТЕМА <span>БИБЛИОТЕКА</span></h1>
</header>

<div class='container'>
    <div class='grid'>
        <!-- ФОРМА ЧИТАТЕЛЯ -->
        <div class='card'>
            <div class='card-header'>РЕГИСТРАЦИЯ ЧИТАТЕЛЯ</div>
            <div class='card-body'>
                <form action='/saveuser' method='POST'>
                    <label>Карта читателя (Приложите к левому модулю):</label>
                    <input type='text' id='user_uid' name='uid' placeholder='Ожидание метки...' required readonly>
                    <button type='button' class='btn btn-red' onclick='scanUser()'>Считать карту</button>

                    <label>ФИО:</label>
                    <input type='text' name='fio' placeholder='Иванов Иван' required>
                    
                    <label>Телефон:</label>
                    <input type='tel' name='phone' placeholder='+7 (999)...' required>
                    
                    <label>Прописка:</label>
                    <input type='text' name='address' placeholder='Адрес...' required>
                    
                    <button type='submit' class='btn btn-black'>Сохранить читателя</button>
                </form>
            </div>
        </div>

        <!-- ФОРМА КНИГИ -->
        <div class='card'>
            <div class='card-header'>ДОБАВЛЕНИЕ КНИГИ</div>
            <div class='card-body'>
                <form action='/savebook' method='POST'>
                    <label>Метка книги (Приложите к правому модулю):</label>
                    <input type='text' id='book_uid' name='uid' placeholder='Ожидание метки...' required readonly>
                    <button type='button' class='btn btn-red' onclick='scanBook()'>Считать метку</button>

                    <label>Название:</label>
                    <input type='text' name='title' placeholder='Название книги' required>
                    
                    <label>Автор:</label>
                    <input type='text' name='author' placeholder='Автор' required>
                    
                    <button type='submit' class='btn btn-black'>Сохранить книгу</button>
                </form>
            </div>
        </div>
    </div>

    <!-- ТАБЛИЦЫ -->
    <div class='card' style='margin-bottom: 25px;'>
        <div class='card-header'>БАЗА ЧИТАТЕЛЕЙ</div>
        <div class='table-wrap'>
            <table>
                <tr><th>UID Карты</th><th>ФИО</th><th>Телефон</th><th>Прописка</th></tr>
                <!-- USERS_TABLE -->
            </table>
        </div>
    </div>

    <div class='card'>
        <div class='card-header'>БАЗА КНИГ</div>
        <div class='table-wrap'>
            <table>
                <tr><th>UID Метки</th><th>Название</th><th>Автор</th></tr>
                <!-- BOOKS_TABLE -->
            </table>
        </div>
    </div>
</div>

<script>
    // Функция запроса к ESP32
    function fetchUID(endpoint, inputId, btnText) {
        const input = document.getElementById(inputId);
        fetch(endpoint)
            .then(response => response.text())
            .then(data => {
                if(data && data.trim() !== "") {
                    input.value = data.trim();
                    input.style.borderColor = "var(--red)";
                } else {
                    alert("Карта не найдена. Приложите её к считывателю и нажмите кнопку снова.");
                }
            });
    }

    function scanUser() { fetchUID('/getuseruid', 'user_uid', 'Считать карту'); }
    function scanBook() { fetchUID('/getbookuid', 'book_uid', 'Считать метку'); }
</script>

</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    if (!LittleFS.begin(true)) return;

    // Инициализация SPI (общая шина)
    SPI.begin();
    
    // Инициализация обоих модулей
    mfrc522User.PCD_Init();
    mfrc522Book.PCD_Init();
    
    // Можно выставить усиление антенны, если читают плохо
    // mfrc522User.PCD_SetAntennaGain(mfrc522User.RxGain_max);
    // mfrc522Book.PCD_SetAntennaGain(mfrc522Book.RxGain_max);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(500);
    Serial.println("\nIP: " + WiFi.localIP().toString());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/saveuser", HTTP_POST, handleSaveUser);
    server.on("/savebook", HTTP_POST, handleSaveBook);
    server.on("/getuseruid", HTTP_GET, []() { server.send(200, "text/plain", lastUserUID); });
    server.on("/getbookuid", HTTP_GET, []() { server.send(200, "text/plain", lastBookUID); });
    
    server.begin();
}

void loop() {
    server.handleClient();

    // Читаем модуль читателя
    String u = readCard(&mfrc522User);
    if (u != "") {
        lastUserUID = u;
        Serial.println("ЧИТАТЕЛЬ: " + u);
    }

    // Читаем модуль книг
    String b = readCard(&mfrc522Book);
    if (b != "") {
        lastBookUID = b;
        Serial.println("КНИГА: " + b);
    }
}

// Универсальная функция чтения карты
String readCard(MFRC522 *reader) {
    if (reader->PICC_IsNewCardPresent() && reader->PICC_ReadCardSerial()) {
        String uid = "";
        for (byte i = 0; i < reader->uid.size; i++) {
            uid += String(reader->uid.uidByte[i] < 0x10 ? "0" : "");
            uid += String(reader->uid.uidByte[i], HEX);
        }
        uid.toUpperCase();
        reader->PICC_HaltA(); // Останавливаем карту
        return uid;
    }
    return "";
}

// --- WEB ОБРАБОТЧИКИ ---

void handleRoot() {
    String html = String(htmlTemplate);
    
    // Таблица пользователей (4 колонки: UID, ФИО, Телефон, Адрес)
    html.replace("<!-- USERS_TABLE -->", generateTable(usersFile, 4));
    
    // Таблица книг (3 колонки: UID, Название, Автор)
    html.replace("<!-- BOOKS_TABLE -->", generateTable(booksFile, 3));
    
    server.send(200, "text/html", html);
}

void handleSaveUser() {
    if (server.hasArg("uid") && server.hasArg("fio") && server.hasArg("phone") && server.hasArg("address")) {
        File file = LittleFS.open(usersFile, "a");
        if (file) {
            file.print(server.arg("uid")); file.print(";");
            file.print(server.arg("fio")); file.print(";");
            file.print(server.arg("phone")); file.print(";");
            file.println(server.arg("address"));
            file.close();
        }
    }
    server.sendHeader("Location", "/");
    server.send(303);
}

void handleSaveBook() {
    if (server.hasArg("uid") && server.hasArg("title") && server.hasArg("author")) {
        File file = LittleFS.open(booksFile, "a");
        if (file) {
            file.print(server.arg("uid")); file.print(";");
            file.print(server.arg("title")); file.print(";");
            file.println(server.arg("author"));
            file.close();
        }
    }
    server.sendHeader("Location", "/");
    server.send(303);
}

// Парсер CSV в HTML таблицу
String generateTable(const char* filename, int cols) {
    String rows = "";
    File file = LittleFS.open(filename, "r");
    if (file) {
        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
                rows += "<tr>";
                int start = 0;
                for (int i = 0; i < cols - 1; i++) {
                    int sep = line.indexOf(';', start);
                    if (sep > 0) {
                        String cell = line.substring(start, sep);
                        // Если это первая колонка (UID), красим её в красный и моноширинный шрифт
                        if (i == 0) rows += "<td class='uid'>" + cell + "</td>";
                        else rows += "<td>" + cell + "</td>";
                        start = sep + 1;
                    }
                }
                rows += "<td>" + line.substring(start) + "</td></tr>";
            }
        }
        file.close();
    }
    if (rows == "") rows = "<tr><td colspan='" + String(cols) + "' class='empty'>База данных пуста</td></tr>";
    return rows;
}

#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>
#include <ctime>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

using namespace boost::asio;

std::string get_current_date() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm *tm = std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d");
    return oss.str();
}

std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm *tm = std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(tm, "%H:%M:%S");
    return oss.str();
}

int get_current_milliseconds() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    return static_cast<int>(ms.count());
}

// Remove invalid characters while keeping numeric format
std::string clean_number(const std::string &input) {
    std::string result;
    for (char c : input) {
        if (isdigit(c) || c == '.' || c == '+' || c == '-' || c == 'E')
            result += c;
    }
    return result;
}

int main() {
    std::string porta;
    std::string nome_arquivo;

    std::cout << "COM port (e.g. COM5 or /dev/ttyUSB0): ";
    std::cin >> porta;

    auto now_nome = std::chrono::system_clock::now();
    std::time_t t_nome = std::chrono::system_clock::to_time_t(now_nome);
    std::tm *tm_nome = std::localtime(&t_nome);

    std::ostringstream nome_auto;
    nome_auto << "log_fluke_" << porta << "_" << std::put_time(tm_nome, "%Y-%m-%d_%H-%M-%S") << ".csv";
    nome_arquivo = nome_auto.str();
    
    #ifdef _WIN32
    _mkdir("output");
    #else
    mkdir("output", 0755);
    #endif
    nome_arquivo = "output/" + nome_auto.str();

    std::cout << "Automatic CSV file: " << nome_arquivo << std::endl;

    if (nome_arquivo.find(".csv") == std::string::npos)
        nome_arquivo += ".csv";

    try {
        boost::asio::io_context io;
        serial_port serial(io, porta);

        serial.set_option(serial_port_base::baud_rate(9600));
        serial.set_option(serial_port_base::character_size(8));
        serial.set_option(serial_port_base::parity(serial_port_base::parity::none));
        serial.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));

        std::ofstream file(nome_arquivo, std::ios::app);

        if (file.tellp() == 0) {
            file << "sep=;\n";
            file << "Date;Time;Milliseconds;Voltage (V);Current (A)\n";
        }

        std::cout << "[READING] " << nome_arquivo << " from " << porta << std::endl;

        streambuf buffer;

        const std::chrono::milliseconds intervalo(1000);

        while (true) {
            auto start = std::chrono::steady_clock::now();

            // Send command
            boost::asio::write(serial, boost::asio::buffer("MEAS?\r\n"));

            // Read line
            read_until(serial, buffer, '\n');
            std::istream is(&buffer);
            std::string linha;
            std::getline(is, linha);

            if (!linha.empty() && linha.back() == '\r')
                linha.pop_back();

            if (linha.find("=>") != std::string::npos)
                continue;

            if (!linha.empty()) {
                std::string data = get_current_date();
                std::string hora = get_current_time();
                int ms = get_current_milliseconds();

                std::stringstream ss(linha);
                std::string item1, item2;

                if (std::getline(ss, item1, ',') && std::getline(ss, item2)) {

                    item1.erase(0, item1.find_first_not_of(" "));
                    item2.erase(0, item2.find_first_not_of(" "));

                    std::string corrente = clean_number(item1);
                    std::string tensao   = clean_number(item2);

                    try {
                        float corrente_f = std::stof(corrente);
                        float tensao_f   = std::stof(tensao);

                        file << data << ";" << hora << ";" << ms << ";"
                             << tensao_f << ";" << corrente_f << "\n";

                        // std::cout << "[" << data << " " << hora << "."
                        //           << std::setfill('0') << std::setw(3) << ms << "] "
                        //           << "V: " << tensao_f
                        //           << " | A: " << corrente_f << std::endl;
                    } catch (...) {
                        std::cout << "[" << hora << "."
                                  << std::setfill('0') << std::setw(3) << ms
                                  << "] Parse error: " << linha << std::endl;
                    }

                } else {
                    file << data << ";" << hora << ";" << ms << ";" << linha << ";---\n";
                    // std::cout << "[" << data << " " << hora << "."
                    //           << std::setfill('0') << std::setw(3) << ms
                    //           << "] Single reading: " << linha << std::endl;
                }

                file.flush();
            }

            // Precise timing control (compensates execution time)
            auto end = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            if (elapsed < intervalo) {
                std::this_thread::sleep_for(intervalo - elapsed);
            }
        }

    } catch (std::exception &e) {
        std::cerr << "Erro: " << e.what() << std::endl;
        std::cout << "\nPressione ENTER para fechar...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
    }

    return 0;
}
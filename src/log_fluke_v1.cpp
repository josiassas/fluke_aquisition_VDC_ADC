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

/**
 * @brief Get the current date as a formatted string.
 * @return std::string Current date in format "YYYY-MM-DD"
 */
std::string get_current_date()
{
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm *tm = std::localtime(&t);

  std::ostringstream oss;
  oss << std::put_time(tm, "%Y-%m-%d");
  return oss.str();
}

/**
 * @brief Get the current milliseconds component of the current time.
 * @return int Milliseconds (0-999) component of the current system time
 */
int get_current_milliseconds()
{
  auto now = std::chrono::system_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  return static_cast<int>(ms.count());
}

/**
 * @brief Get the current time as a formatted string with millisecond precision.
 * @return std::string Current time in format "HH:MM:SS:mmm" where mmm is milliseconds
 */
std::string get_current_time()
{
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm *tm = std::localtime(&t);

  std::ostringstream oss;
  oss << std::put_time(tm, "%H:%M:%S") << ":" << std::setfill('0') << std::setw(3) << get_current_milliseconds();
  return oss.str();
}

/**
 * @brief Clean a numeric string by removing invalid characters.
 * @param input The input string to clean
 * @return std::string Cleaned numeric string containing only digits, decimal points, signs, and exponent notation
 */
std::string clean_number(const std::string &input)
{
  std::string result;
  for (char c : input)
  {
    if (isdigit(c) || c == '.' || c == '+' || c == '-' || c == 'E')
      result += c;
  }
  return result;
}

int main()
{
  std::string port;
  std::string file_name;

  std::cout << "COM port (e.g. COM5 or /dev/ttyUSB0): ";
  std::cin >> port;

  auto now_name = std::chrono::system_clock::now();
  std::time_t t_nome = std::chrono::system_clock::to_time_t(now_name);
  std::tm *tm_nome = std::localtime(&t_nome);

  std::ostringstream auto_name;
  auto_name << "log_fluke_" << port << "_" << std::put_time(tm_nome, "%Y-%m-%d_%H-%M-%S") << ".csv";
  file_name = auto_name.str();

#ifdef _WIN32
  _mkdir("output");
#else
  mkdir("output", 0755);
#endif
  file_name = "output/" + auto_name.str();

  std::cout << "Automatic CSV file: " << file_name << std::endl;

  if (file_name.find(".csv") == std::string::npos)
    file_name += ".csv";

  try
  {
    boost::asio::io_context io;
    serial_port serial(io, port);

    serial.set_option(serial_port_base::baud_rate(9600));
    serial.set_option(serial_port_base::character_size(8));
    serial.set_option(serial_port_base::parity(serial_port_base::parity::none));
    serial.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));

    std::ofstream file(file_name, std::ios::app);

    if (file.tellp() == 0)
    {
      file << "sep=;\n";
      file << "Date;Time;Voltage (V);Current (A)\n";
    }

    std::cout << "[READING] " << file_name << " from " << port << std::endl;

    streambuf buffer;

    const std::chrono::milliseconds interval(1000);
    std::string last_day = get_current_date();

    while (true)
    {
      auto start = std::chrono::steady_clock::now();

      // Check if it's 07:00 and the day has changed
      std::string day_current = get_current_date();
      std::string hour_current = get_current_time();

      if (day_current != last_day && hour_current.substr(0, 2) == "07")
      {
        file.close();

        auto now_name = std::chrono::system_clock::now();
        std::time_t t_nome = std::chrono::system_clock::to_time_t(now_name);
        std::tm *tm_nome = std::localtime(&t_nome);

        std::ostringstream auto_name;
        auto_name << "log_fluke_" << port << "_" << std::put_time(tm_nome, "%Y-%m-%d_%H-%M-%S") << ".csv";
        file_name = "output/" + auto_name.str();

        file.open(file_name, std::ios::app);
        file << "sep=;\n";
        file << "Date;Time;Voltage (V);Current (A)\n";

        std::cout << "[NEW FILE] " << file_name << std::endl;
        last_day = day_current;
      }

      try
      {
        // Send command
        boost::asio::write(serial, boost::asio::buffer("MEAS?\r\n"));

        read_until(serial, buffer, '\n');
      }
      catch (const std::exception &e)
      {
        std::cerr << "[" << get_current_date() << " " << get_current_time()
                  << "." << std::setfill('0') << std::setw(3) << get_current_milliseconds()
                  << "] *** USB DESCONECTADO *** " << std::endl;

        // Keep file open and try to reconnect in a loop

        try
        {
          if (serial.is_open())
            serial.close();
        }
        catch (...)
        {
        }

        // Clear any buffered date
        try
        {
          buffer.consume(buffer.size());
        }
        catch (...)
        {
        }

        // Reconnection attempts
        while (true)
        {
          std::cout << "[" << get_current_date() << " " << get_current_time() << "."
                    << std::setfill('0') << std::setw(3) << get_current_milliseconds()
                    << "[RECONNECTING] Tentando reconectar a " << port << " ..." << std::endl;
          std::this_thread::sleep_for(std::chrono::seconds(5));

          try
          {
            serial.open(port);
            serial.set_option(serial_port_base::baud_rate(9600));
            serial.set_option(serial_port_base::character_size(8));
            serial.set_option(serial_port_base::parity(serial_port_base::parity::none));
            serial.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));

            // file remains open during USB disconnect; no need to reopen

            std::cout << "[" << get_current_date() << " " << get_current_time() << "."
                      << std::setfill('0') << std::setw(3) << get_current_milliseconds()
                      << "] *** USB RECONECTADO *** " << std::endl;

            // reset last_day and buffer
            last_day = get_current_date();
            try
            {
              buffer.consume(buffer.size());
            }
            catch (...)
            {
            }

            break; // reconnected
          }
          catch (const std::exception &e2)
          {
            std::cerr << "Falha ao reconectar: " << e2.what() << std::endl;
          }
        }
        // continue main loop after reconnection
        continue;
      }

      // Read line
      std::istream is(&buffer);
      std::string line;
      std::getline(is, line);

      if (!line.empty() && line.back() == '\r')
        line.pop_back();

      if (line.find("=>") != std::string::npos)
        continue;

      if (!line.empty())
      {
        std::string date = get_current_date();
        std::string hour = get_current_time();
        int ms = get_current_milliseconds();

        std::stringstream ss(line);
        std::string item1, item2;

        if (std::getline(ss, item1, ',') && std::getline(ss, item2))
        {

          item1.erase(0, item1.find_first_not_of(" "));
          item2.erase(0, item2.find_first_not_of(" "));

          std::string current = clean_number(item1);
          std::string voltage = clean_number(item2);

          try
          {
            float current_f = std::stof(current);
            float voltage_f = std::stof(voltage);

            file << date << ";" << hour << ";"
                 << voltage_f << ";" << current_f << "\n";
          }
          catch (...)
          {
            std::cout << "[" << get_current_date() << " " << get_current_time() << "."
                      << std::setfill('0') << std::setw(3) << get_current_milliseconds()
                      << "] Parse error: " << line << std::endl;
          }
        }
        else
        {
          file << date << ";" << hour << ";" << line << ";---\n";
          std::cout << "[" << get_current_date() << " " << get_current_time() << "."
                    << std::setfill('0') << std::setw(3) << get_current_milliseconds()
                    << "] Single reading: " << line << std::endl;
        }

        file.flush();
      }

      // Precise timing control (compensates execution time)
      auto end = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

      if (elapsed < interval)
      {
        std::this_thread::sleep_for(interval - elapsed);
      }
    }
  }
  catch (std::exception &e)
  {
    std::cerr << "Erro: " << e.what() << std::endl;
    std::cout << "\nPressione ENTER para fechar...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
  }

  return 0;
}
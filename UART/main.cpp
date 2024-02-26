#include <iostream>
#include <boost/asio.hpp>

int main() {
    boost::asio::io_service io;
    boost::asio::serial_port serial(io, "/dev/ttyS0");

    serial.set_option(boost::asio::serial_port_base::baud_rate(115200));
    serial.set_option(boost::asio::serial_port_base::character_size(8));
    serial.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
    serial.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    serial.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

    // Send data
    std::string data_to_send = "Hello, Raspberry Pi!\n";
    boost::asio::write(serial, boost::asio::buffer(data_to_send));

    // Get Data
    //char read_data[128];
    //size_t bytes_read = boost::asio::read(serial, boost::asio::buffer(read_data, sizeof(read_data)));
    //std::string received_data(read_data, bytes_read);

    //std::cout << "Received data: " << received_data;

    serial.close();

    return 0;
}